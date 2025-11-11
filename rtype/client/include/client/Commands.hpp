#pragma once
#include <vector>

enum class PlayerAction {
    NONE = 0,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    SHOOT,
    CHANGE_WEAPON
};

using CommandList = std::vector<PlayerAction>;