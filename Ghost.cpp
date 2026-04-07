#ifndef GHOST_CPP
#define GHOST_CPP

#include <vector>
#include <array>
#include <omp.h>
#include <SFML/Graphics.hpp>
#include "Global.cpp"
#include "MapCollision.cpp"

// Thread-safe random (fallback only)
inline unsigned int thread_safe_rand(unsigned int& seed)
{
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

class Ghost
{
    unsigned char id;
    unsigned char direction;
    unsigned char status;  // 0=Waiting, 1=Exiting, 2=Normal

    Position position;
    unsigned char animation_frame;
    unsigned char frame_counter;

    unsigned short exit_delay;
    unsigned short exit_timer;

    sf::Texture* texture;

public:
    Ghost(unsigned char i_id) : id(i_id), direction(0), status(0),
                                 position({0, 0}), animation_frame(0),
                                 frame_counter(0), texture(nullptr),
                                 exit_delay(i_id * 120),
                                 exit_timer(0) {}

    void setTexture(sf::Texture* tex) { texture = tex; }

    void set_position(short i_x, short i_y)
    {
        position   = {i_x, i_y};
        status     = 0;
        exit_timer = 0;
    }

    // ── Getters for CUDA data packing ──────────────────────────────────────
    Position      get_position()  { return position;  }
    unsigned char get_direction() { return direction; }
    unsigned char get_status()    { return status;    }

    // ── CUDA sets the direction externally ─────────────────────────────────
    void set_direction(unsigned char d) { direction = d; }

    void draw(sf::RenderWindow& i_window, bool i_frightened)
    {
        frame_counter++;
        if (frame_counter >= 8)
        {
            frame_counter = 0;
            animation_frame = (animation_frame + 1) % 2;
        }

        if (texture && !i_frightened)
        {
            sf::Sprite sprite(*texture);
            sf::Vector2u texSize = texture->getSize();
            sprite.setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
            float scale = (CELL_SIZE) / (float)texSize.x;
            sprite.setScale(sf::Vector2f(scale, scale));
            sprite.setPosition(sf::Vector2f(
                position.x + CELL_SIZE / 2.f,
                position.y + CELL_SIZE / 2.f));

            if      (id == 0) sprite.setColor(sf::Color::Red);
            else if (id == 1) sprite.setColor(sf::Color::Magenta);
            else if (id == 2) sprite.setColor(sf::Color::Cyan);
            else              sprite.setColor(sf::Color(255, 165, 0));

            i_window.draw(sprite);
        }
        else
        {
            sf::CircleShape body(CELL_SIZE / 2.0f - 1.0f);
            body.setPosition(sf::Vector2f(
                static_cast<float>(position.x),
                static_cast<float>(position.y)));

            if (i_frightened) body.setFillColor(sf::Color(0, 0, 139));
            else
            {
                if      (id == 0) body.setFillColor(sf::Color::Red);
                else if (id == 1) body.setFillColor(sf::Color::Magenta);
                else if (id == 2) body.setFillColor(sf::Color::Cyan);
                else              body.setFillColor(sf::Color(255, 165, 0));
            }
            i_window.draw(body);

            sf::RectangleShape skirt1(sf::Vector2f(3.f, 4.f));
            sf::RectangleShape skirt2(sf::Vector2f(3.f, 4.f));
            sf::RectangleShape skirt3(sf::Vector2f(3.f, 4.f));
            skirt1.setPosition(sf::Vector2f(position.x + 2,  position.y + CELL_SIZE - 5));
            skirt2.setPosition(sf::Vector2f(position.x + 6,  position.y + CELL_SIZE - 5));
            skirt3.setPosition(sf::Vector2f(position.x + 10, position.y + CELL_SIZE - 5));
            skirt1.setFillColor(body.getFillColor());
            skirt2.setFillColor(body.getFillColor());
            skirt3.setFillColor(body.getFillColor());
            i_window.draw(skirt1);
            i_window.draw(skirt2);
            i_window.draw(skirt3);

            sf::CircleShape eye1(2.f), eye2(2.f);
            sf::CircleShape pupil1(1.f), pupil2(1.f);
            eye1.setFillColor(sf::Color::White);
            eye2.setFillColor(sf::Color::White);
            pupil1.setFillColor(sf::Color::Black);
            pupil2.setFillColor(sf::Color::Black);
            eye1.setPosition(sf::Vector2f(position.x + 3, position.y + 3));
            eye2.setPosition(sf::Vector2f(position.x + 9, position.y + 3));

            float pupilOffsetX = 0, pupilOffsetY = 0;
            if      (direction == 0) pupilOffsetX =  1;
            else if (direction == 1) pupilOffsetY = -1;
            else if (direction == 2) pupilOffsetX = -1;
            else if (direction == 3) pupilOffsetY =  1;

            pupil1.setPosition(sf::Vector2f(position.x + 4  + pupilOffsetX, position.y + 4 + pupilOffsetY));
            pupil2.setPosition(sf::Vector2f(position.x + 10 + pupilOffsetX, position.y + 4 + pupilOffsetY));
            i_window.draw(eye1);
            i_window.draw(eye2);
            if (!i_frightened) { i_window.draw(pupil1); i_window.draw(pupil2); }
        }
    }

    // ── update: direction is now SET BY CUDA before this is called ─────────
    // This function only handles: house exit logic + applying movement
    void update(std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& i_map)
    {
        short speed = GHOST_SPEED;

        // --- PHASE 0: WAITING ---
        if (status == 0)
        {
            exit_timer++;
            if (exit_timer >= exit_delay) status = 1;
            return;
        }

        // --- PHASE 1: EXITING HOUSE ---
        if (status == 1)
        {
            if      (position.x < 10 * CELL_SIZE) position.x += speed;
            else if (position.x > 10 * CELL_SIZE) position.x -= speed;
            position.y -= speed;

            if (position.y <= 7 * CELL_SIZE)
            {
                position.y = 7 * CELL_SIZE;
                status = 2;
                // Initial direction — CUDA will take over from next frame
                if      (id == 0) direction = 2;
                else if (id == 1) direction = 0;
                else if (id == 2) direction = 2;
                else              direction = 0;
            }
            return;
        }

        // --- PHASE 2: NORMAL MOVEMENT ---
        // Direction has already been set by CUDA in GhostManager
        // Just apply the movement here using OpenMP
        if (direction == 0) position.x += speed;
        else if (direction == 1) position.y -= speed;
        else if (direction == 2) position.x -= speed;
        else if (direction == 3) position.y += speed;

        // Tunnel wrap
        if      (position.x < -CELL_SIZE)            position.x = CELL_SIZE * MAP_WIDTH;
        else if (position.x > CELL_SIZE * MAP_WIDTH)  position.x = -CELL_SIZE;
    }
};

#endif