#ifndef PP_GAME_H
#define PP_GAME_H

#include "../include/glm/trigonometric.hpp"
#include "pp_globals.h"

int menu_prepare(PR::Level *level);
void menu_update(void);
void menu_draw(void);

int level1_prepare(PR::Level *level);
void level1_update(void);
void level1_draw(void);

int level2_prepare(PR::Level *level);
void level2_update(void);
void level2_draw(void);

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
Rect rect_in_camera_space(Rect r, PR::Camera *cam) {

    Rect res;

    res.pos = r.pos - cam->pos + glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);
    res.dim = r.dim;
    res.angle = r.angle;
    res.triangle = r.triangle;

    return res;
}

inline
TexCoords texcoords_in_texture_space(float x, float y,
                                     float w, float h,
                                     Texture *tex) {
    TexCoords res;

    res.tx = x / tex->width;
    res.ty = 1.f - (y + h) / tex->height;
    res.tw = w / tex->width;
    res.th = h / tex->height;

    return res;

}


#endif
