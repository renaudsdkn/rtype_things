#include "../include/client/Anima.hpp"

Animobj& Anima::get_obj(std::size_t pos) { return _arr[pos]; }
void Anima::modify_obj(std::size_t pos, Animobj& info) { _arr[pos] = info; }
void Anima::add_obj(Animobj info) { _arr.push_back(info); }
void Anima::remove_obj(std::size_t pos) {
    UnloadTexture(_arr[pos].Obj);
    _arr.erase(_arr.begin() + pos);
}
std::size_t Anima::size() const { return _arr.size(); }

void Anima::animate(Animobj& info, int numFramesPerRow) {
    float dt = GetFrameTime();
    int x = (info.cur % numFramesPerRow) * info.ObjWidth;
    int y = (info.cur / numFramesPerRow) * info.ObjHeight;

    info.duration_left -= dt;
    if (info.duration_left <= 0) {
        info.duration_left = info.speed;
        info.cur++;
        if (info.cur > info.last) {
            info.cur = (info.type == REPEATING) ? info.first : info.last;
        }
    }

    Rectangle source = { (float)x, (float)y, info.ObjWidth, info.ObjHeight };
    Rectangle dest = { info.pos.x, info.pos.y, info.scale.x, info.scale.y };
    DrawTexturePro(info.Obj, source, dest, {0, 0}, 0.f, WHITE);
}

void Anima::animate(int numFramesPerRow) {
    for (auto& i : _arr) {
        animate(i, numFramesPerRow);
    }
}
