#include "../include/client/SpriteManager.hpp"
void SpriteManager::loadAssets() {
    sprites["player"] = LoadTexture("assets/ship.png");
    sprites["enemy"] = LoadTexture("assets/enemy.png");
    sprites["missile"] = LoadTexture("assets/fireball.png");
}

Texture2D SpriteManager::getSprite(const std::string& name) const {
    auto it = sprites.find(name);
    if (it != sprites.end()) return it->second;
    return Texture2D{}; // Texture vide si non trouvé
}

void SpriteManager::unload() {
    for (auto& pair : sprites) {
        UnloadTexture(pair.second);
    }
    sprites.clear();
}
