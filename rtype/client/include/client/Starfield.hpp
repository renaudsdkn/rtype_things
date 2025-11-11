#pragma once

#include "raylib.h"
#include <vector>

struct Star {
    Vector2 position;
    float speed;
};

class Starfield {
public:
    Starfield(int screenWidth, int screenHeight);
    void updateAndDraw();

private:
    int screenWidth;
    int screenHeight;
    std::vector<Star> stars;
};


