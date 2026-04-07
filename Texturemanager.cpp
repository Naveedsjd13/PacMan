#ifndef TEXTUREMANAGER_CPP
#define TEXTUREMANAGER_CPP

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>
#include <filesystem>

class TextureManager
{
private:
    std::map<std::string, sf::Texture> textures;

public:
    bool loadTexture(const std::string& name, const std::string& filepath)
    {
        std::cout << "TEXTURE MANAGER IS RUNNING" << std::endl;
        std::filesystem::path absPath = std::filesystem::absolute(filepath);
        std::cout << "[TextureManager] Looking for: " << absPath << std::endl;

        if (!std::filesystem::exists(absPath))
        {
            std::cerr << "[TextureManager] ERROR: File does not exist: " << absPath << std::endl;
            std::cerr << "                 Make sure " << filepath 
                      << " is in the same folder as pacman.exe" << std::endl;
            return false;
        }

        sf::Texture texture;
        if (!texture.loadFromFile(filepath))
        {
            std::cerr << "[TextureManager] ERROR: File found but failed to load: " << absPath << std::endl;
            std::cerr << "                 Make sure it is a valid PNG or JPG image." << std::endl;
            return false;
        }

        sf::Vector2u size = texture.getSize();
        std::cout << "[TextureManager] OK: Loaded '" << name 
                  << "' (" << size.x << "x" << size.y << "px)" << std::endl;

        textures[name] = std::move(texture);
        return true;
    }

    sf::Texture* getTexture(const std::string& name)
    {
        auto it = textures.find(name);
        if (it != textures.end())
            return &it->second;

        std::cerr << "[TextureManager] ERROR: Texture '" << name 
                  << "' not found in cache. Was it loaded?" << std::endl;
        return nullptr;
    }

    void loadAllTextures()
    {
        std::cout << "[TextureManager] Loading textures..." << std::endl;
        std::cout << "[TextureManager] Working directory: " 
                  << std::filesystem::current_path() << std::endl;

        loadTexture("pacman", "pacman.png");
        loadTexture("ghost",  "ghost.png");

        std::cout << "[TextureManager] Done. Loaded " 
                  << textures.size() << "/2 textures." << std::endl;

        if (textures.size() < 2)
        {
            std::cerr << std::endl;
            std::cerr << "================================================" << std::endl;
            std::cerr << " WARNING: Some textures failed to load." << std::endl;
            std::cerr << " The game will use fallback shapes instead." << std::endl;
            std::cerr << " Place pacman.png and ghost.png next to pacman.exe" << std::endl;
            std::cerr << "================================================" << std::endl;
            std::cerr << std::endl;
        }
    }
};

#endif