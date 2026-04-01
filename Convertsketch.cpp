#ifndef CONVERTSKETCH_CPP
#define CONVERTSKETCH_CPP

#include <array>
#include <string>
#include "Global.cpp"
#include "Pacman.cpp"

std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> convert_sketch(const std::array<std::string, MAP_HEIGHT>& i_map_sketch, Pacman& i_pacman)
{
    std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> output_map{};

    for (unsigned char a = 0; a < MAP_HEIGHT; a++)
    {
        for (unsigned char b = 0; b < MAP_WIDTH; b++)
        {
            output_map[b][a] = Cell::Empty;

            // SAFETY: Check if the character exists in the string before reading it
            if (b < i_map_sketch[a].length())
            {
                switch (i_map_sketch[a][b])
                {
                    case '#': output_map[b][a] = Cell::Wall;      break;
                    case '=': output_map[b][a] = Cell::Door;      break;
                    case '.': output_map[b][a] = Cell::Pellet;    break;
                    case 'o': output_map[b][a] = Cell::Energizer; break;
                    case 'P': i_pacman.set_position(CELL_SIZE * b, CELL_SIZE * a); break;
                    case ' ': 
                    case '_': output_map[b][a] = Cell::Empty;     break;
                    default: break;
                }
            }
        }
    }

    return output_map;
}

#endif