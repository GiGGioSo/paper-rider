#ifndef PP_RECT_H
#define PP_RECT_H

#include "pr_mathy.h"

typedef struct PR_Rect {
    vec2f pos;
    vec2f dim;

    float angle;
    // lmao.
    bool triangle;
} PR_Rect;

bool
rect_contains_point(const PR_Rect rec, float px, float py, bool centered);

bool
rect_are_colliding(const PR_Rect r1, const PR_Rect r2, float *cx, float *cy);

#endif
