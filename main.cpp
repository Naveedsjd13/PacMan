#include <array>
#include <chrono>
#include <string>
#include <SFML/Graphics.hpp>

#include "Global.cpp"
#include "TextureManager.cpp"
#include "GhostManager.cpp"
#include "DrawMap.cpp"
#include "ConvertSketch.cpp"

int main()
{
	bool game_won = 0;
	unsigned lag = 0;
	unsigned char level = 0;
	unsigned int score = 0;

	std::chrono::time_point<std::chrono::steady_clock> previous_time;
	std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> map{};

	// The classic Pac-Man maze
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
		"#####################"};

	// SFML 3.x fix: VideoMode takes sf::Vector2u, not two unsigned ints
	sf::RenderWindow window(
		sf::VideoMode(sf::Vector2u(
			(unsigned)(CELL_SIZE * MAP_WIDTH  * SCREEN_RESIZE),
			(unsigned)(CELL_SIZE * MAP_HEIGHT * SCREEN_RESIZE))),
		"PAC-MAN - Enhanced Edition");

	// SFML 3.x fix: sf::View constructor takes sf::FloatRect correctly
	window.setView(sf::View(sf::FloatRect(
		sf::Vector2f(0.f, 0.f),
		sf::Vector2f(
			(float)(CELL_SIZE * MAP_WIDTH),
			(float)(CELL_SIZE * MAP_HEIGHT)))));

	// Initialize texture manager and load sprites
	TextureManager texManager;
	texManager.loadAllTextures();

	// Initialize Pac-Man
	Pacman pacman;
	pacman.setTexture(texManager.getTexture("pacman"));

	// Initialize Ghost Manager
	GhostManager ghost_manager;
	ghost_manager.setTextures(texManager.getTexture("ghost"));

	// Setup initial game state
	map = convert_sketch(map_sketch, pacman);
	ghost_manager.reset(GHOST_START_POSITIONS);

	previous_time = std::chrono::steady_clock::now();

	// Main game loop
	while (window.isOpen())
	{
		unsigned delta_time = std::chrono::duration_cast<std::chrono::microseconds>(
								  std::chrono::steady_clock::now() - previous_time)
								  .count();
		lag += delta_time;
		previous_time += std::chrono::microseconds(delta_time);

		while (FRAME_DURATION <= lag)
		{
			lag -= FRAME_DURATION;

			// SFML 3.x fix: event handling uses pollEvent returning optional
			while (auto ev = window.pollEvent())
			{
				if (ev->is<sf::Event::Closed>())
					window.close();
			}

			// Game logic
			if (!game_won && !pacman.get_dead())
			{
				game_won = 1;
				
				// Update game entities
				pacman.update(level, map);
				ghost_manager.update(map);

				// Check collision with ghosts
				if (ghost_manager.check_pacman_collision(pacman.get_position()))
				{
					// Only die if not powered up
					if (pacman.get_energizer_timer() == 0)
					{
						pacman.set_dead(true);
					}
					else
					{
						// Eating a ghost while powered up
						score += 200;
					}
				}

				// Check if any pellets are left
				for (const auto &column : map)
				{
					for (const Cell &cell : column)
					{
						if (Cell::Pellet == cell)
						{
							game_won = 0;
							break;
						}
					}
					if (!game_won)
						break;
				}
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter))
			{
				// Restart or Next Level
				game_won = 0;
				if (!pacman.get_dead())
				{
					level++;
					score += 1000; // Level completion bonus
				}
				else
				{
					// Reset everything on death
					score = 0;
					level = 0;
				}
				map = convert_sketch(map_sketch, pacman);
				pacman.reset();
				ghost_manager.reset(GHOST_START_POSITIONS);
			}

			// Render
			if (FRAME_DURATION > lag)
			{
				window.clear(sf::Color::Black);
				
				// Draw game elements
				draw_map(map, window);
				ghost_manager.draw(window, pacman.get_energizer_timer());
				pacman.draw(game_won, window);

				// Update window title with game state
				std::string title = "PAC-MAN | Level: " + std::to_string(1 + level) + 
				                   " | Score: " + std::to_string(score);
				
				if (pacman.get_dead())
					title += " | GAME OVER! Press ENTER to Restart";
				else if (game_won)
					title += " | LEVEL COMPLETE! Press ENTER for Next Level";
				else if (pacman.get_energizer_timer() > 0)
					title += " | POWER UP!";

				window.setTitle(title);
				window.display();
			}
		}
	}
	return 0;
}