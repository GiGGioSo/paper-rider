#ifndef PP_GAME_H
#define PP_GAME_H

#include "../include/glm/trigonometric.hpp"
#include "pp_globals.h"

void game_update(float dt);

void apply_air_resistances(PP::Plane* p);

void game_draw(void);

#define POW2(x) x * x

inline float lerp(float x1, float x2, float t) {
    float result = (1.f - t) * x1 + t * x2;
    return result;
}

inline glm::vec2 lerp_v2(glm::vec2 x1, glm::vec2 x2, float t) {
    glm::vec2 result;
    result.x = x1.x + t * (x2.x - x1.x);
    result.y = x1.y + t * (x2.y - x1.y);

    return result;
}

// NOTE: Vertical lift and horizontal drag are identical,
//       because falling is just like moving right.

// TODO: Modify these coefficient so that feels nice

inline float vertical_lift_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(180.f - 2.f * angle));
    return result;
}

inline float vertical_drag_coefficient(float angle) {
    float result = (float) sin(glm::radians(180.f - 2.f * angle));
    return result;
}

inline float horizontal_lift_coefficient(float angle) {
    float result = (float) sin(glm::radians(2.f * angle));
    return result;
}

inline float horizontal_drag_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(2.f * angle));
    return result;
}

#endif
