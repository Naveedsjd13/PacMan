#ifndef GHOST_MANAGER_CPP
#define GHOST_MANAGER_CPP

#include <array>
#include <vector>
#include <cmath>
#include <omp.h>
#include "Ghost.cpp"

class GhostManager
{
    std::vector<Ghost> ghosts;

public:
    GhostManager()
    {
        for (unsigned char i = 0; i < 4; i++)
            ghosts.push_back(Ghost(i));
    }

    void setTextures(sf::Texture *ghostTexture)
    {
        for (Ghost &ghost : ghosts)
            ghost.setTexture(ghostTexture);
    }

    void reset(const std::array<Position, 4> &i_start_positions)
    {
        for (unsigned char i = 0; i < 4; i++)
            ghosts[i].set_position(i_start_positions[i].x, i_start_positions[i].y);
    }

    void update(std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH> &i_map)
    {
// --- OpenMP: each ghost updates in parallel ---
    #pragma omp parallel for schedule(dynamic) num_threads(4)
        for (int i = 0; i < (int)ghosts.size(); i++)
            ghosts[i].update(i_map);
    }

    // SFML is NOT thread-safe for rendering, so draw stays sequential
    void draw(sf::RenderWindow &i_window, unsigned short i_energizer_timer)
    {
        bool is_frightened = (i_energizer_timer > 0);
        for (Ghost &ghost : ghosts)
            ghost.draw(i_window, is_frightened);
    }

    // Add inside GhostManager class (public section)
    void get_all_positions(short *ox, short *oy) const
    {
        for (int i = 0; i < 4; i++)
        {
            Position p = ghosts[i].get_position();
            ox[i] = p.x;
            oy[i] = p.y;
        }
    }

    void set_all_positions(const short *ix, const short *iy)
    {
        for (int i = 0; i < 4; i++)
            ghosts[i].set_position(ix[i], iy[i]);
    }

    // Check if any ghost is touching Pacman using OpenMP reduction
    bool check_pacman_collision(Position i_pacman_pos)
    {
        int hit = 0;
#pragma omp parallel for reduction(+ : hit) num_threads(4)
        for (int i = 0; i < (int)ghosts.size(); i++)
        {
            Position g_pos = ghosts[i].get_position();
            if (abs(g_pos.x - i_pacman_pos.x) < CELL_SIZE &&
                abs(g_pos.y - i_pacman_pos.y) < CELL_SIZE)
            {
                hit++;
            }
        }
        return hit > 0;
    }
};

#endif