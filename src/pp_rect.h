#ifndef PP_RECT_H
#define PP_RECT_H

#include "../include/glm/vec2.hpp"

struct Rect {
    glm::vec2 pos;
    glm::vec2 dim;

    float angle;
};

bool rect_are_colliding(Rect* r1, Rect* r2);

#endif