#ifndef _PR_CAMERA_H_
#define _PR_CAMERA_H_

#include "pr_rect.h"
#include "pr_mathy.h"

typedef struct PR_Camera {
    vec2f pos;
    float speed_multiplier;
} PR_Camera;

PR_Rect rect_in_camera_space(PR_Rect r, const PR_Camera *cam);

vec2f screen_to_world(vec2f p_screen, const PR_Camera *cam);

#endif
