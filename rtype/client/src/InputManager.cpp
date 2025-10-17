#include "../include/client/InputManager.hpp"
#include "raylib.h"

CommandList InputManager::getCommands() const {
    CommandList commands;

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        commands.push_back(PlayerAction::MOVE_UP);
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        commands.push_back(PlayerAction::MOVE_DOWN);
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        commands.push_back(PlayerAction::MOVE_LEFT);
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        commands.push_back(PlayerAction::MOVE_RIGHT);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        commands.push_back(PlayerAction::SHOOT);
    }

    if (IsKeyPressed(KEY_C)) {
        commands.push_back(PlayerAction::CHANGE_WEAPON);
    }

    return commands;
}