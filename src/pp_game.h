#ifndef PP_GAME_H
#define PP_GAME_H

#include "../include/glm/trigonometric.hpp"
#include "pp_globals.h"

int menu_prepare(PR::Menu *menu, PR::Level *level, const char* mapfile_path);
void menu_update(void);
void menu_draw(void);

int level_prepare(PR::Menu *menu, PR::Level *leve, const char* mapfile_path);
void level_update(void);

#define POW2(x) (x * x)

inline float lerp(float x1, float x2, float t) {
    float result = (1.f - t) * x1 + t * x2;
    return result;
}

inline glm::vec2 lerp_v2(glm::vec2 x1, glm::vec2 x2, float t) {
    glm::vec2 result;
    result.x = lerp(x1.x, x2.x, t);
    result.y = lerp(x1.y, x2.y, t);

    return result;
}

// NOTE: Vertical lift and horizontal drag are identical,
//       because falling is just like moving right.

// TODO: Maybe modify there coefficients to make them feel better?

inline
float vertical_lift_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(180.f - 2.f * angle));
    return result;
}

inline
float vertical_drag_coefficient(float angle) {
    float result = (float) sin(glm::radians(180.f - 2.f * angle));
    return result;
}

inline
float horizontal_lift_coefficient(float angle) {
    float result = (float) sin(glm::radians(2.f * angle));
    return result;
}

inline
float horizontal_drag_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(2.f * angle));
    return result;
}

inline
const char *get_case_name(PR::GameCase c) {
    switch (c) {
        case PR::MENU:
            return "MENU";
        case PR::LEVEL:
            return "LEVEL";
        default:
            return "UNKNOWN";
    }
    return "";
}

inline
const char *get_portal_type_name(PR::PortalType t) {
    switch (t) {
        case PR::INVERSE:
            return "INVERSE";
        case PR::SHUFFLE_COLORS:
            return "SHUFFLE_COLORS";
        default:
            return "UNKNOWN";
    }
    return "";
}

#endif
