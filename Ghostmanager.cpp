#ifndef GHOST_MANAGER_CPP
#define GHOST_MANAGER_CPP

#include <array>
#include <vector>
#include <cmath>
#include <cstdio>
#include <omp.h>
#include "Ghost.cpp"
// #include "GhostPathfinding.cuh"  // CUDA DISABLED FOR NOW

class GhostManager
{
    std::vector<Ghost> ghosts;
    // int  flat_map[GPU_MAP_W * GPU_MAP_H] = {};  // Not needed without CUDA
    // bool cuda_ready = false;  // Not needed without CUDA

public:
    GhostManager()
    {
        for (unsigned char i = 0; i < 4; i++)
            ghosts.push_back(Ghost(i));
    }

    void setTextures(sf::Texture* tex)
    {
        for (Ghost& g : ghosts) g.setTexture(tex);
    }

    void reset(const std::array<Position, 4>& positions)
    {
        for (int i = 0; i < 4; i++)
            ghosts[i].set_position(positions[i].x, positions[i].y);
    }

    // Dummy function - does nothing without CUDA
    void init_cuda(const std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& map)
    {
        printf("[CPU] Ghost pathfinding (CUDA disabled)\n\n");
    }

    // OpenMP only - no CUDA pathfinding
    void update(std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& map,
                short pacman_x, short pacman_y)
    {
        // Ghosts just move randomly without CUDA pathfinding
        // OpenMP: move all ghosts in parallel
        #pragma omp parallel for schedule(dynamic) num_threads(4)
        for (int i = 0; i < (int)ghosts.size(); i++)
            ghosts[i].update(map);
    }

    // Draw stays sequential (SFML not thread-safe)
    void draw(sf::RenderWindow& window, unsigned short energizer_timer)
    {
        bool frightened = energizer_timer > 0;
        for (Ghost& g : ghosts) g.draw(window, frightened);
    }

    // Collision check with OpenMP reduction
    bool check_pacman_collision(Position pacman_pos)
    {
        int hit = 0;
        #pragma omp parallel for reduction(+:hit) num_threads(4)
        for (int i = 0; i < (int)ghosts.size(); i++)
        {
            Position p = ghosts[i].get_position();
            if (abs(p.x - pacman_pos.x) < CELL_SIZE &&
                abs(p.y - pacman_pos.y) < CELL_SIZE)
                hit++;
        }
        return hit > 0;
    }

    // MPI helpers
    void get_all_positions(short* ox, short* oy) const
    {
        for (int i = 0; i < 4; i++)
        {
            Position p = const_cast<Ghost&>(ghosts[i]).get_position();
            ox[i] = p.x; oy[i] = p.y;
        }
    }

    void set_all_positions(const short* ix, const short* iy)
    {
        for (int i = 0; i < 4; i++)
            ghosts[i].set_position(ix[i], iy[i]);
    }

    void cleanup_cuda() { /* do nothing */ }
};

#endif