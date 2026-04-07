#pragma once
#ifndef GLOBAL_CPP
#define GLOBAL_CPP

#include <array>

constexpr unsigned char CELL_SIZE = 16;
constexpr unsigned char MAP_HEIGHT = 21;
constexpr unsigned char MAP_WIDTH = 21;

// Separate speeds
constexpr unsigned char PACMAN_SPEED = 2;
constexpr unsigned char GHOST_SPEED = 1;

constexpr unsigned char SCREEN_RESIZE = 3;
constexpr unsigned short ENERGIZER_DURATION = 512;
constexpr unsigned short FRAME_DURATION = 16667;

enum Cell { Door, Empty, Energizer, Pellet, Wall };

struct Position
{
    short x;
    short y;
    bool operator==(const Position& i_position) const
    {
        return this->x == i_position.x && this->y == i_position.y;
    }
};

// Ghost house layout (from map_sketch in main.cpp):
//   Row  9: "____#.#   0   #.#____"   <- '0' = ghost 0 spawn (outside house, above door)
//   Row 10: "#####.# ##=## #.#####"   <- '=' = house door
//   Row 11: "______  #123#  ______"   <- '1','2','3' = ghosts 1/2/3 inside house
//
// Ghost 0 starts at row 9 col 10 — already outside, exits immediately.
// Ghosts 1/2/3 start inside the house and exit one by one.

const std::array<Position, 4> GHOST_START_POSITIONS = {
    Position{10 * CELL_SIZE, 9  * CELL_SIZE},  // Ghost 0 - Red    (above door, exits first)
    Position{9  * CELL_SIZE, 11 * CELL_SIZE},  // Ghost 1 - Pink   (inside house, left)
    Position{10 * CELL_SIZE, 11 * CELL_SIZE},  // Ghost 2 - Cyan   (inside house, center)
    Position{11 * CELL_SIZE, 11 * CELL_SIZE},  // Ghost 3 - Orange (inside house, right)
};

#endif