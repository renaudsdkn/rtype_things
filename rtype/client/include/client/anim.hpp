/*
 *
 *
 *
 *
 */

#ifndef ANIM_H_
    #define ANIM_H_
    #include <vector>
    #include <raylib.h>

#include "../include/ecs/engine.hpp"

typedef enum {
    REPEATING = 1,
    ONESHOT
} AnimType;

Component::Position::operator Vector2() {
    return (Vector2){(*this).x, (*this).y};
}

typedef struct {
    int first;
    int last;
    int cur;
    float rotateAngl { 0.0f };
    float ObjWidth;      // frame width
    float ObjHeight;     // frame height
    float speed;         // duration per frame in seconds
    float duration_left;
    // Vector2 pos;
    Component::Position &pos;         // position on screen
    Vector2 scale;       // display scale (width, height)
    AnimType type;
    Texture2D Obj;       // Changed from reference to value
} Animobj;

#endif /* */
