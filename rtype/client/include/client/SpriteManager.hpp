#pragma once
#include "raylib.h"
#include <map>
#include <string>

class SpriteManager {
public:
    void loadAssets();
    Texture2D getSprite(const std::string& name) const;
    void unload();

private:
    std::map<std::string, Texture2D> sprites;
};
