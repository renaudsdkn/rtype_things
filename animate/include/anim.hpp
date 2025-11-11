/*
 *
 *
 *
 *
 */

#ifndef ANIM_H_
    #define ANIM_H_
    #include <SFML/System/Vector2.hpp>
#include <vector>
    #include <raylib.h>


    typedef enum {
        REPEATING = 1,
        ONESHOT
    } AnimType;

// (Vector2) Component::Position::operator() {
//     return (Vector2){(*this).x, (*this).y};
// }

typedef struct {
    int first;
    int last;
    int cur;
    float rotateAngl { 0.0f };
    float ObjWidth;      // frame width
    float ObjHeight;     // frame height
    float speed;         // duration per frame in seconds
    float duration_left;
    Vector2 pos;
    // Component::Position &pos;         // position on screen
    Vector2 scale;       // display scale (width, height)
    AnimType type;
    Texture2D Obj;       // Changed from reference to value
} Animobj;

#endif /* */
