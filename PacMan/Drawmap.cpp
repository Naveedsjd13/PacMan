#ifndef DRAWMAP_CPP
#define DRAWMAP_CPP
#include <array>
#include <SFML/Graphics.hpp>
#include "Global.cpp"

void draw_map(const std::array<std::array<Cell, MAP_HEIGHT>, MAP_WIDTH>& i_map, sf::RenderWindow& i_window)
{
	// Enhanced wall rendering with outline
	sf::RectangleShape wall_shape({(float)CELL_SIZE, (float)CELL_SIZE});
	wall_shape.setFillColor(sf::Color(33, 33, 222)); // Brighter blue
	wall_shape.setOutlineColor(sf::Color(66, 66, 255)); // Lighter blue outline
	wall_shape.setOutlineThickness(1.f);

	// Enhanced pellets
	sf::CircleShape pellet_shape(2.5f);
	pellet_shape.setFillColor(sf::Color(255, 184, 174)); // Peachy color
	
	// Enhanced energizers with glow effect
	sf::CircleShape energizer_shape(6.f);
	energizer_shape.setFillColor(sf::Color(255, 255, 255)); // White center
	
	sf::CircleShape energizer_glow(7.f);
	energizer_glow.setFillColor(sf::Color(255, 200, 100, 128)); // Glowing effect

	// Door (ghost house entrance)
	sf::RectangleShape door_shape({(float)CELL_SIZE, 2.f});
	door_shape.setFillColor(sf::Color(255, 192, 203)); // Pink door

	for (unsigned char a = 0; a < MAP_WIDTH; a++)
	{
		for (unsigned char b = 0; b < MAP_HEIGHT; b++)
		{
			float x = static_cast<float>(CELL_SIZE * a);
			float y = static_cast<float>(CELL_SIZE * b);

			switch (i_map[a][b])
			{
				case Cell::Wall:
					wall_shape.setPosition({x, y});
					i_window.draw(wall_shape);
					break;
					
				case Cell::Door:
					door_shape.setPosition({x, y + CELL_SIZE / 2.f - 1.f});
					i_window.draw(door_shape);
					break;
					
				case Cell::Pellet:
					pellet_shape.setPosition({x + CELL_SIZE / 2.f - 2.5f, y + CELL_SIZE / 2.f - 2.5f});
					i_window.draw(pellet_shape);
					break;
					
				case Cell::Energizer:
					// Draw glow first
					energizer_glow.setPosition({x + CELL_SIZE / 2.f - 7.f, y + CELL_SIZE / 2.f - 7.f});
					i_window.draw(energizer_glow);
					// Then the energizer
					energizer_shape.setPosition({x + CELL_SIZE / 2.f - 6.f, y + CELL_SIZE / 2.f - 6.f});
					i_window.draw(energizer_shape);
					break;
					
				default: break;
			}
		}
	}
}
#endif