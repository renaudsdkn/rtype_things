#pragma once

#include "raylib.h"
#include <vector>
#include <string>

struct EntityState {
    std::string type;
    Vector2 position;
    int id;
};

struct MissileState {
    Vector2 position;
    float speed;
    int id;
};

struct EnemyState {
    Vector2 position;
    int id;
    float speed;
    int health;
    bool isAlive;
};

struct PlayerState {
    Vector2 position;
    int lives;
    int score;
    int id;
    bool isAlive;
};

struct GameState {
    enum State { MENU, PLAYING, GAME_OVER };
    State currentState = MENU;

    PlayerState player = {{100, 300}, 3, 0, 0, true};
    std::vector<EnemyState> enemies;
    std::vector<MissileState> missiles;

    float enemySpawnTimer = 0.0f;
    float enemySpawnInterval = 2.0f;

    void reset() {
        player.position = {100, 300};
        player.lives = 3;
        player.score = 0;
        player.isAlive = true;
        enemies.clear();
        missiles.clear();
        enemySpawnTimer = 0.0f;
        currentState = PLAYING;
    }
};


