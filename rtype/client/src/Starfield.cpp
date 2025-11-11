#include "../include/client/Starfield.hpp"

Starfield::Starfield(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight) {
    for (int i = 0; i < 100; ++i) {
        stars.push_back({
            {(float)GetRandomValue(0, screenWidth), (float)GetRandomValue(0, screenHeight)},
            (float)GetRandomValue(1, 3)
        });
    }
}

void Starfield::updateAndDraw() {
    for (auto& star : stars) {
        star.position.x -= star.speed;
        if (star.position.x < 0) {
            star.position.x = (float)screenWidth;
            star.position.y = (float)GetRandomValue(0, screenHeight);
        }
        DrawPixelV(star.position, WHITE);
    }
}


