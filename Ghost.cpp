#ifndef GHOST_CPP
#define GHOST_CPP

#include <vector>
#include <array>
#include <omp.h>
#include <SFML/Graphics.hpp>
#include "Global.cpp"
#include "MapCollision.cpp"

// Thread-safe random: works on Windows, Linux, and Mac.
inline unsigned int thread_safe_rand(unsigned int& seed)
{
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

class Ghost
{
    unsigned char id;
    unsigned char direction;
    unsigned char status;       // 0: Waiting, 1: Exiting House, 2: Normal Navigation
    Position position;

    unsigned char animation_frame;
    unsigned char frame_counter;

    // Each ghost waits a different number of frames before exiting
    unsigned short exit_delay;
    unsigned short exit_timer;

    sf::Texture* texture;

public:
    Ghost(unsigned char i_id) : id(i_id), direction(0), status(0),
                                 position({0, 0}), animation_frame(0),
                                 frame_counter(0), texture(nullptr),
                                 exit_delay(i_id * 120), // 0, 120, 240, 360 frames apart
                                 exit_timer(0) {}

    void setTexture(sf::Texture* tex) { texture = tex; }

    void set_position(short i_x, short i_y)
    {
        position  = {i_x, i_y};
        status    = 0;
        exit_timer = 0;
    }

    Position get_position() { return position; }

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
                position.y + CELL_SIZE / 2.f
            ));

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
                static_cast<float>(position.y)
            ));

            if (i_frightened)
                body.setFillColor(sf::Color(0, 0, 139));
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

            if (!i_frightened)
            {
                i_window.draw(pupil1);
                i_window.draw(pupil2);
            }
        }
    }

    void update(std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& i_map)
    {
        short speed = GHOST_SPEED;

        // --- PHASE 0: WAITING IN HOUSE ---
        // Each ghost waits a different amount before starting to exit.
        // Ghost 0: exits immediately (delay=0)
        // Ghost 1: waits 120 frames (~2 sec)
        // Ghost 2: waits 240 frames (~4 sec)
        // Ghost 3: waits 360 frames (~6 sec)
        if (status == 0)
        {
            exit_timer++;
            if (exit_timer >= exit_delay)
                status = 1; // Start exiting
            return;
        }

        // --- PHASE 1: EXITING THE HOUSE ---
        if (status == 1)
        {
            // Move toward center column (tile 10)
            if      (position.x < 10 * CELL_SIZE) position.x += speed;
            else if (position.x > 10 * CELL_SIZE) position.x -= speed;

            // Move UP toward exit
            position.y -= speed;

            if (position.y <= 7 * CELL_SIZE)
            {
                position.y = 7 * CELL_SIZE;
                status    = 2;

                // Each ghost gets a FIXED starting direction based on id
                // so they spread out immediately instead of all going the same way
                // 0=Right, 1=Up, 2=Left, 3=Down
                if      (id == 0) direction = 2; // Red   → goes Left
                else if (id == 1) direction = 0; // Pink  → goes Right
                else if (id == 2) direction = 2; // Cyan  → goes Left
                else              direction = 0; // Orange→ goes Right
            }
            return;
        }

        // --- PHASE 2: NORMAL NAVIGATION ---
        if (position.x % CELL_SIZE == 0 && position.y % CELL_SIZE == 0)
        {
            std::vector<unsigned char> available_directions;

            for (unsigned char a = 0; a < 4; a++)
            {
                if (a == (direction + 2) % 4) continue; // No 180s

                bool coll = false;
                if      (a == 0) coll = map_collision(0, 1, position.x + speed, position.y,         i_map);
                else if (a == 1) coll = map_collision(0, 1, position.x,         position.y - speed, i_map);
                else if (a == 2) coll = map_collision(0, 1, position.x - speed, position.y,         i_map);
                else if (a == 3) coll = map_collision(0, 1, position.x,         position.y + speed, i_map);

                if (!coll) available_directions.push_back(a);
            }

            if (!available_directions.empty())
            {
                // Unique seed per ghost per position — guaranteed different choices
                unsigned int seed = static_cast<unsigned int>(
                    id * 73856093u ^ 
                    (unsigned int)position.x * 19349663u ^ 
                    (unsigned int)position.y * 83492791u
                );
                direction = available_directions[thread_safe_rand(seed) % available_directions.size()];
            }
            else
                direction = (direction + 2) % 4;
        }

        // Apply movement
        if      (direction == 0) position.x += speed;
        else if (direction == 1) position.y -= speed;
        else if (direction == 2) position.x -= speed;
        else if (direction == 3) position.y += speed;

        // Tunnel Wrap
        if      (position.x < -CELL_SIZE)            position.x = CELL_SIZE * MAP_WIDTH;
        else if (position.x > CELL_SIZE * MAP_WIDTH)  position.x = -CELL_SIZE;
    }
};

#endif