#pragma once
#include "raylib.h"

typedef enum {
    REPEATING = 1,
    ONESHOT
} AnimType;

typedef struct {
    int first;
    int last;
    int cur;
    float ObjWidth;
    float ObjHeight;
    float speed;
    float duration_left;
    Vector2 pos;
    Vector2 scale;
    AnimType type;
    Texture2D Obj;
} Animobj;
