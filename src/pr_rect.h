#ifndef PP_RECT_H
#define PP_RECT_H

#include "glm/vec2.hpp"

typedef struct PR_Rect {
    glm::vec2 pos;
    glm::vec2 dim;

    float angle;
    // lmao.
    bool triangle;
} PR_Rect;

bool
rect_contains_point(const PR_Rect rec, float px, float py, bool centered);

bool
rect_are_colliding(const PR_Rect r1, const PR_Rect r2, float *cx, float *cy);

#endif
