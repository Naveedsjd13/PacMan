#ifndef MAP_COLLISION_CPP
#define MAP_COLLISION_CPP

#include <array>
#include <cmath>
#include "Global.cpp"

bool map_collision(bool i_collect, bool i_ghost, short i_x, short i_y, std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& i_map)
{
	bool collision = false;

	// --- THE CRASH FIX: BOUNDARY CHECK ---
	// If the coordinates are outside the map (like in the tunnel), 
	// we return "no collision" so they can pass through, 
	// but we STOP the code from checking i_map[invalid_index].
	if (i_x < 0 || i_x >= CELL_SIZE * MAP_WIDTH || i_y < 0 || i_y >= CELL_SIZE * MAP_HEIGHT)
	{
		return false; 
	}
	// -------------------------------------

	float cell_x = i_x / (float)CELL_SIZE;
	float cell_y = i_y / (float)CELL_SIZE;

	// Check the 4 corners of the entity to see if they hit a wall
	for (unsigned char a = 0; a < 4; a++)
	{
		short x = 0;
		short y = 0;

		switch (a)
		{
			case 0: // Top Left
				x = static_cast<short>(floor(cell_x));
				y = static_cast<short>(floor(cell_y));
				break;
			case 1: // Top Right
				x = static_cast<short>(ceil(cell_x));
				y = static_cast<short>(floor(cell_y));
				break;
			case 2: // Bottom Left
				x = static_cast<short>(floor(cell_x));
				y = static_cast<short>(ceil(cell_y));
				break;
			case 3: // Bottom Right
				x = static_cast<short>(ceil(cell_x));
				y = static_cast<short>(ceil(cell_y));
				break;
		}

		// Final safety check before accessing the array
		if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
		{
			if (i_ghost) // Ghost logic
			{
				if (Cell::Wall == i_map[x][y]) collision = true;
			}
			else // Pacman logic
			{
				if (Cell::Wall == i_map[x][y] || Cell::Door == i_map[x][y]) collision = true;
			}

			// Pellet eating logic
			if (i_collect && Cell::Wall != i_map[x][y])
			{
				if (Cell::Pellet == i_map[x][y])
				{
					i_map[x][y] = Cell::Empty;
				}
				else if (Cell::Energizer == i_map[x][y])
				{
					i_map[x][y] = Cell::Empty;
					return true; // Signal that an energizer was eaten
				}
			}
		}
	}

	return collision;
}

#endif