#include <array>
#include <chrono>
#include <string>
#include <iostream>
#include <mpi.h>
#include <SFML/Graphics.hpp>

#include "Global.cpp"
#include "MPIBridge.cpp"
#include "TextureManager.cpp"
#include "Ghostmanager.cpp"
#include "DrawMap.cpp"
#include "ConvertSketch.cpp"

std::array<std::string, MAP_HEIGHT> map_sketch = {
    "#####################",
    "#.........#.........#",
    "#.###.###.#.###.###.#",
    "#o# #.# #.#.# #.# #o#",
    "#.###.###.#.###.###.#",
    "#...................#",
    "#.###.#.#####.#.###.#",
    "#.....#...#...#.....#",
    "#####.### # ###.#####",
    "____#.#   0   #.#____",
    "#####.# ##=## #.#####",
    "______  #123#  ______",
    "#####.# ##### #.#####",
    "____#.#       #.#____",
    "#####.# ##### #.#####",
    "#.........#.........#",
    "#.###.###.#.###.###.#",
    "#o..#.....P.....#..o#",
    "###.#.#.#####.#.#.###",
    "#.......#####.......#",
    "#####################"
};

// ─────────────────────────────────────────────────────────────────────────────
//  RANK 0 — DISPLAY PROCESS
// ─────────────────────────────────────────────────────────────────────────────
void run_display_process()
{
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   MPI RANK 0 - DISPLAY PROCESS       ║\n";
    std::cout << "║   Handles: Window, Pacman, Drawing    ║\n";
    std::cout << "║   Sends:   Pacman pos to Rank 1       ║\n";
    std::cout << "║   Receives: Ghost pos from Rank 1     ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";

    bool game_won = false;
    unsigned lag  = 0;
    unsigned char level = 0;
    unsigned int score  = 0;
    unsigned int frame_count    = 0;
    unsigned int messages_sent  = 0;
    unsigned int messages_recv  = 0;

    std::chrono::time_point<std::chrono::steady_clock> previous_time;
    std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> map{};

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(
            (unsigned)(CELL_SIZE * MAP_WIDTH  * SCREEN_RESIZE),
            (unsigned)(CELL_SIZE * MAP_HEIGHT * SCREEN_RESIZE))),
        "ParaMan - MPI + OpenMP + CUDA");

    window.setView(sf::View(sf::FloatRect(
        sf::Vector2f(0.f, 0.f),
        sf::Vector2f(
            (float)(CELL_SIZE * MAP_WIDTH),
            (float)(CELL_SIZE * MAP_HEIGHT)))));

    TextureManager texManager;
    texManager.loadAllTextures();

    Pacman pacman;
    pacman.setTexture(texManager.getTexture("pacman"));

    // Rank 0 ghost manager is DISPLAY ONLY - no AI, no CUDA
    GhostManager ghost_manager;
    ghost_manager.setTextures(texManager.getTexture("ghost"));

    map = convert_sketch(map_sketch, pacman);
    ghost_manager.reset(GHOST_START_POSITIONS);

    GameStateData state{ true, false };
    mpi_send_game_state(state);
    std::cout << "[Rank 0] Sent START signal to Rank 1\n";

    previous_time = std::chrono::steady_clock::now();

    while (window.isOpen())
    {
        unsigned delta_time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - previous_time).count();
        lag += delta_time;
        previous_time += std::chrono::microseconds(delta_time);

        while (FRAME_DURATION <= lag)
        {
            lag -= FRAME_DURATION;
            frame_count++;

            while (auto ev = window.pollEvent())
            {
                if (ev->is<sf::Event::Closed>())
                {
                    GameStateData quit{ false, false };
                    mpi_send_game_state(quit);
                    PacmanData dummy{0, 0, false};
                    mpi_send_pacman(dummy);
                    std::cout << "[Rank 0] Sent QUIT to Rank 1\n";
                    std::cout << "[Rank 0] Total MPI messages sent:     " << messages_sent << "\n";
                    std::cout << "[Rank 0] Total MPI messages received: " << messages_recv << "\n";
                    window.close();
                }
            }

            if (!window.isOpen()) break;

            if (!game_won && !pacman.get_dead())
            {
                game_won = true;

                pacman.update(level, map);

                // Send Pacman pos to Rank 1
                PacmanData pd;
                pd.x         = pacman.get_position().x;
                pd.y         = pacman.get_position().y;
                pd.energized = pacman.get_energizer_timer() > 0;
                mpi_send_pacman(pd);
                messages_sent++;

                // Receive ghost positions from Rank 1
                GhostPositions gp = mpi_recv_ghost_positions();
                ghost_manager.set_all_positions(gp.x, gp.y);
                messages_recv++;

                if (frame_count % 60 == 0)
                {
                    std::cout << "[Rank 0] Frame " << frame_count
                              << " | Pacman: (" << pd.x << "," << pd.y << ")"
                              << " | Ghost0: (" << gp.x[0] << "," << gp.y[0] << ")"
                              << " | MPI sent=" << messages_sent
                              << " recv=" << messages_recv << "\n";
                }

                if (ghost_manager.check_pacman_collision(pacman.get_position()))
                {
                    if (pacman.get_energizer_timer() == 0) pacman.set_dead(true);
                    else score += 200;
                }

                for (const auto& column : map)
                {
                    for (const Cell& cell : column)
                        if (Cell::Pellet == cell) { game_won = false; break; }
                    if (!game_won) break;
                }
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter))
            {
                game_won = false;
                if (!pacman.get_dead()) { level++; score += 1000; }
                else { score = 0; level = 0; }

                map = convert_sketch(map_sketch, pacman);
                pacman.reset();
                ghost_manager.reset(GHOST_START_POSITIONS);

                GameStateData rst{ true, true };
                mpi_send_game_state(rst);
                std::cout << "[Rank 0] Sent RESET to Rank 1\n";
            }

            if (FRAME_DURATION > lag)
            {
                window.clear(sf::Color::Black);
                draw_map(map, window);
                ghost_manager.draw(window, pacman.get_energizer_timer());
                pacman.draw(game_won, window);

                std::string title = "ParaMan [MPI+OMP+CUDA] | Level: " +
                    std::to_string(1 + level) + " | Score: " + std::to_string(score);
                if (pacman.get_dead())                     title += " | GAME OVER! Press ENTER";
                else if (game_won)                         title += " | LEVEL COMPLETE! Press ENTER";
                else if (pacman.get_energizer_timer() > 0) title += " | POWER UP!";

                window.setTitle(title);
                window.display();
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  RANK 1 — GHOST AI PROCESS (OpenMP + CUDA)
// ─────────────────────────────────────────────────────────────────────────────
void run_ghost_ai_process()
{
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   MPI RANK 1 - GHOST AI PROCESS      ║\n";
    std::cout << "║   Pathfinding: CUDA (GPU)             ║\n";
    std::cout << "║   Movement:    OpenMP (CPU threads)   ║\n";
    std::cout << "║   Receives: Pacman pos from Rank 0    ║\n";
    std::cout << "║   Sends:    Ghost pos to Rank 0       ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";

    std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> map{};
    GhostManager ghost_manager;
    ghost_manager.reset(GHOST_START_POSITIONS);

    Pacman dummy_pacman;
    map = convert_sketch(map_sketch, dummy_pacman);

    // ── Init CUDA pathfinder with map ─────────────────────────────────────
    ghost_manager.init_cuda(map);

    unsigned int frame_count   = 0;
    unsigned int messages_sent = 0;
    unsigned int messages_recv = 0;

    std::cout << "[Rank 1] Waiting for START signal from Rank 0...\n";
    GameStateData state = mpi_recv_game_state();
    std::cout << "[Rank 1] START received! Running with:\n";
    std::cout << "[Rank 1]   OpenMP threads: " << omp_get_max_threads() << "\n";
    std::cout << "[Rank 1]   CUDA pathfinding: ENABLED\n";
    std::cout << "[Rank 1]   Ghosts will CHASE Pacman via GPU!\n\n";

    while (state.running)
    {
        if (state.reset)
        {
            ghost_manager.reset(GHOST_START_POSITIONS);
            map = convert_sketch(map_sketch, dummy_pacman);
            ghost_manager.init_cuda(map);
            std::cout << "[Rank 1] RESET received - ghosts repositioned\n";
        }

        // Receive Pacman position from Rank 0
        PacmanData pd = mpi_recv_pacman();
        messages_recv++;
        frame_count++;

        // CUDA picks directions + OpenMP applies movement
        ghost_manager.update(map, pd.x, pd.y);

        // Send ghost positions back to Rank 0
        GhostPositions gp{};
        ghost_manager.get_all_positions(gp.x, gp.y);
        mpi_send_ghost_positions(gp);
        messages_sent++;

        if (frame_count % 60 == 0)
        {
            std::cout << "[Rank 1] Frame " << frame_count
                      << " | Pacman: (" << pd.x << "," << pd.y << ")"
                      << " | Ghost0: (" << gp.x[0] << "," << gp.y[0] << ")"
                      << " | CUDA+OMP active"
                      << " | MPI sent=" << messages_sent
                      << " recv=" << messages_recv << "\n";
        }

        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(0, TAG_GAME_STATE, MPI_COMM_WORLD, &flag, &status);
        if (flag) state = mpi_recv_game_state();
    }

    ghost_manager.cleanup_cuda();

    std::cout << "\n[Rank 1] QUIT received. Shutting down.\n";
    std::cout << "[Rank 1] Total frames processed:      " << frame_count   << "\n";
    std::cout << "[Rank 1] Total MPI messages sent:     " << messages_sent << "\n";
    std::cout << "[Rank 1] Total MPI messages received: " << messages_recv << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "[MPI] Process launched - Rank " << rank
              << " of " << size << " total processes\n";

    if (size < 2)
    {
        if (rank == 0)
        {
            std::cout << "\nERROR: Need 2 MPI processes!\n";
            std::cout << "Run with: mpiexec -n 2 pacman.exe\n\n";
        }
        MPI_Finalize();
        return 1;
    }

    if      (rank == 0) run_display_process();
    else if (rank == 1) run_ghost_ai_process();

    MPI_Finalize();
    return 0;
}