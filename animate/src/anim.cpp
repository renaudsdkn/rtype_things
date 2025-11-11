/*
 *
 *
 *
 *
 */

#include "../include/anim.hpp"
// #include <raylib.h>

// typedef enum {
//     REPEATING = 1,
//     ONESHOT
// } AnimType;

// typedef struct {
// int first;
// int last;
// int cur;
// float ObjWidth;      // frame width
// float ObjHeight;     // frame height
// float speed;         // duration per frame in seconds
// float duration_left;
// Vector2 pos;         // position on screen
// Vector2 scale;       // display scale (width, height)
// AnimType type;
// Texture2D Obj;       // Changed from reference to value
// } Animobj;
/*
 * Fixed Animation Code
 */

// #include "../include/anim.hpp"

class Anima {
private:
    std::vector<Animobj> _arr;

public:
    Anima() {};
    ~Anima() {};

    Animobj &get_obj(std::size_t pos) { return _arr[pos]; }
    void modify_obj(std::size_t pos, Animobj &info) { _arr[pos] = info; }
    void add_obj(const Animobj &info) { _arr.push_back(info); }
    void remove_obj(std::size_t pos) {
        UnloadTexture(_arr[pos].Obj);
        _arr.erase(_arr.begin() + pos);
    }
    std::size_t size() const { return _arr.size(); }

    void animate(Animobj &info, int numFramesPerRow = 1) {
        float dt = GetFrameTime();

        // Update animation timer
        info.duration_left -= dt;
        if (info.duration_left <= 0) {
            info.duration_left = info.speed;
            info.cur++;

            // Check if we've exceeded the last frame
            if (info.cur > info.last) {
                switch (info.type) {
                    case REPEATING:
                        info.cur = info.first;
                        break;
                    case ONESHOT:
                        info.cur = info.last;
                        break;
                }
            }
        }

        // Calculate source rectangle from spritesheet
        int x = (info.cur % numFramesPerRow) * (int)info.ObjWidth;
        int y = (info.cur / numFramesPerRow) * (int)info.ObjHeight;

        Rectangle source = {
            .x = (float)x,
            .y = (float)y,
            .width = info.ObjWidth,
            .height = info.ObjHeight
        };

        Rectangle dest = {
            .x = info.pos.x,
            .y = info.pos.y,
            .width = info.scale.x,
            .height = info.scale.y
        };

        DrawTexturePro(info.Obj, source, dest, {0, 0}, info.rotateAngl, WHITE);
    }

    void animate(int numFramesPerRow = 1) {
        float dt = GetFrameTime(); // Call ONCE per frame, not per object

        for (auto &i : _arr) {
            // Update animation timer
            i.duration_left -= dt;
            if (i.duration_left <= 0) {
                i.duration_left = i.speed;
                i.cur++;

                // Check if we've exceeded the last frame
                if (i.cur > i.last) {
                    switch (i.type) {
                        case REPEATING:
                            i.cur = i.first;
                            break;
                        case ONESHOT:
                            i.cur = i.last;
                            break;
                    }
                }
            }

            // Calculate source rectangle from spritesheet
            int x = (i.cur % numFramesPerRow) * (int)i.ObjWidth;
            int y = (i.cur / numFramesPerRow) * (int)i.ObjHeight;

            Rectangle source = {
                .x = (float)x,
                .y = (float)y,
                .width = i.ObjWidth,
                .height = i.ObjHeight
            };

            Rectangle dest = {
                .x = i.pos.x,
                .y = i.pos.y,
                .width = i.scale.x,
                .height = i.scale.y
            };

            DrawTexturePro(i.Obj, source, dest, {0, 0}, i.rotateAngl, WHITE);
        }
    }
};

int main() {
    Anima lola;

    const int screenWidth = 1366;
    const int screenHeight = 768;
    InitWindow(screenWidth, screenHeight, "TestAnim"); // FIXED: was screenWidth twice
    SetTargetFPS(60);

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 7,
        .cur = 0,
        .ObjWidth = 192,
        .ObjHeight = 192,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {50, 50},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("Tiny Swords (Free Pack)/Units/Black Units/Warrior/Warrior_Idle.png")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 12,
        .cur = 0,
        .ObjWidth = 17,
        .ObjHeight = 18,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {250, 250},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy_1.png")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 18,
        .cur = 0,
        .ObjWidth = 32,
        .ObjHeight = 36,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {700, 0},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy_2.png")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 5,
        .cur = 0,
        // .rotateAngl = 180.f,
        .ObjWidth = 66,
        .ObjHeight = 66,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy_3.png")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 7,
        .cur = 0,
        .ObjWidth = 33,
        .ObjHeight = 33,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {700, 200},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy4.gif")
    });
    */
    // Check if texture loaded successfully

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 3,
        .cur = 0,
        .ObjWidth = 50,
        .ObjHeight = 51,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy.gif")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 1,
        .cur = 0,
        .ObjWidth = 64,
        .ObjHeight = 51,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy2.gif")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 2,
        .cur = 0,
        .ObjWidth = 33,
        .ObjHeight = 32,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy3.gif")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 7,
        .cur = 0,
        .ObjWidth = 33,
        .ObjHeight = 33,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy5.gif")
    });*/

    /*lola.add_obj((Animobj) {
        .first = 0,
        .last = 3,
        .cur = 0,
        .ObjWidth = 67,
        .ObjHeight = 54,
        .speed = 0.1f,        // 0.1 seconds per frame = 10 FPS animation
        .duration_left = 0.1f,
        .pos = {500, 500},
        .scale = {192, 192},  // Display at original size+
        .type = REPEATING,
        .Obj = LoadTexture("enemy6.gif")
    });*/

    if (lola.get_obj(0).Obj.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture! Check file path.");
        CloseWindow();
        return -1;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{65, 105, 225, 255}); // Fixed alpha to 255

        // Pass 8 because your sprite sheet has 8 frames in one row
        lola.animate(8);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
