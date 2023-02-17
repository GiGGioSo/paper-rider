#ifndef PP_RECT_H
#define PP_RECT_H

#include "../include/glm/vec2.hpp"

struct Rect {
    glm::vec2 pos;
    glm::vec2 dim;

    float angle;
    // lmao.
    bool triangle;
};

bool rect_contains_point(const Rect *rec, float px, float py, bool centered);

bool rect_are_colliding(const Rect* r1, const Rect* r2, float *cx, float *cy);

#endif
