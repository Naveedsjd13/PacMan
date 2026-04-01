#ifndef PACMAN_CPP
#define PACMAN_CPP
#include <array>
#include <cmath>
#include <SFML/Graphics.hpp>
#include "Global.cpp"
#include "MapCollision.cpp"

class Pacman
{
    bool animation_over;
    bool dead;
    unsigned char direction;
    unsigned short animation_timer;
    unsigned short energizer_timer;
    Position position;
    bool moving;
    
    // Animation
    unsigned char mouth_frame;
    unsigned char frame_counter;

    sf::Texture* texture;

public:
    Pacman() : animation_over(0), dead(0), direction(0), animation_timer(0), 
               energizer_timer(0), position({0, 0}), moving(0), 
               mouth_frame(0), frame_counter(0), texture(nullptr) {}

    void setTexture(sf::Texture* tex) { texture = tex; }

    // getters
    bool get_animation_over() { return animation_over; }
    bool get_dead() { return dead; }
    unsigned short get_energizer_timer() { return energizer_timer; }
    Position get_position() { return position; }
    unsigned char get_direction() { return direction; }

    // setters
    void set_position(short i_x, short i_y) { position = {i_x, i_y}; }
    void set_animation_timer(unsigned short t) { animation_timer = t; }
    void set_dead(bool state) { dead = state; }

    void reset()
    {
        animation_over = 0;
        dead = 0;
        direction = 0;
        animation_timer = 0;
        energizer_timer = 0;
        moving = 0;
        mouth_frame = 0;
        frame_counter = 0;
    }

    void draw(bool i_victory, sf::RenderWindow& i_window)
    {
        if (texture && !dead)
        {
            // Animate mouth (3 frames: open, half, closed)
            frame_counter++;
            if (frame_counter >= 4)
            {
                frame_counter = 0;
                mouth_frame = (mouth_frame + 1) % 3;
            }

            // SFML 3.x fix: pass texture directly to constructor
            sf::Sprite sprite(*texture);
            
            // Calculate rotation based on direction
            float rotation = 0.f;
            switch (direction)
            {
                case 0: rotation =   0.f; break;  // Right
                case 1: rotation = -90.f; break;  // Up
                case 2: rotation = 180.f; break;  // Left
                case 3: rotation =  90.f; break;  // Down
            }

            // Set origin to center for proper rotation
            sf::Vector2u texSize = texture->getSize();
            sprite.setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
            
            // Scale to fit cell size
            float scale = (CELL_SIZE) / (float)texSize.x;
            sprite.setScale(sf::Vector2f(scale, scale));
            
            // Position at center of cell
            sprite.setPosition(sf::Vector2f(
                position.x + CELL_SIZE / 2.f,
                position.y + CELL_SIZE / 2.f
            ));
            sprite.setRotation(sf::degrees(rotation));

            // Power-up effect
            if (energizer_timer > 0)
                sprite.setColor(sf::Color(255, 200, 0));
            else
                sprite.setColor(sf::Color::White);

            i_window.draw(sprite);
        }
        else
        {
            // Fallback: classic circle rendering
            sf::CircleShape shape(CELL_SIZE / 2.f - 1.f);
            shape.setOrigin(sf::Vector2f(shape.getRadius(), shape.getRadius()));
            shape.setFillColor(energizer_timer > 0 ? sf::Color(255, 200, 0) : sf::Color::Yellow);
            shape.setPosition(sf::Vector2f(
                (float)position.x + CELL_SIZE / 2.f,
                (float)position.y + CELL_SIZE / 2.f
            ));
            i_window.draw(shape);
        }
    }

    void update(unsigned char i_level, std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& i_map)
    {
        std::array<bool, 4> walls{};
        walls[0] = map_collision(0, 0, PACMAN_SPEED + position.x, position.y,         i_map);
        walls[1] = map_collision(0, 0, position.x,                position.y - PACMAN_SPEED, i_map);
        walls[2] = map_collision(0, 0, position.x - PACMAN_SPEED, position.y,         i_map);
        walls[3] = map_collision(0, 0, position.x,                PACMAN_SPEED + position.y, i_map);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) && !walls[0]) { direction = 0; moving = 1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)    && !walls[1]) { direction = 1; moving = 1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)  && !walls[2]) { direction = 2; moving = 1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)  && !walls[3]) { direction = 3; moving = 1; }

        if (moving && !walls[direction])
        {
            switch (direction)
            {
                case 0: position.x += PACMAN_SPEED; break;
                case 1: position.y -= PACMAN_SPEED; break;
                case 2: position.x -= PACMAN_SPEED; break;
                case 3: position.y += PACMAN_SPEED; break;
            }
        }

        // Tunnel wrap
        if (-CELL_SIZE >= position.x)              position.x = CELL_SIZE * MAP_WIDTH - PACMAN_SPEED;
        else if (CELL_SIZE * MAP_WIDTH <= position.x) position.x = PACMAN_SPEED - CELL_SIZE;

        // Eat pellets/energizers
        if (map_collision(1, 0, position.x, position.y, i_map))
            energizer_timer = static_cast<unsigned short>(ENERGIZER_DURATION / pow(2, i_level));
        else
            energizer_timer = std::max(0, energizer_timer - 1);
    }
};

#endif