#pragma once
#include <vector>
#include "Animobj.hpp"

class Anima {
private:
    std::vector<Animobj> _arr;

public:
    Anima() {};
    ~Anima() {};

    Animobj& get_obj(std::size_t pos);
    void modify_obj(std::size_t pos, Animobj& info);
    void add_obj(Animobj info);
    void remove_obj(std::size_t pos);
    std::size_t size() const;

    void animate(Animobj& info, int numFramesPerRow = 1);
    void animate(int numFramesPerRow = 1);
};
