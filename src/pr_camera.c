#include "pr_camera.h"

#include "pr_globals.h"

PR_Rect rect_in_camera_space(PR_Rect r, const PR_Camera *cam) {
    PR_Rect res;

    res.pos =
        vec2f_sum(
            vec2f_diff(
                r.pos,
                cam->pos),
            _vec2f(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f));

    res.dim = r.dim;
    res.angle = r.angle;
    res.triangle = r.triangle;

    return res;
}

vec2f screen_to_world(vec2f p_screen, const PR_Camera* cam) {
    vec2f screen_center = _vec2f(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f);

    return vec2f_sum(
               vec2f_diff(p_screen, screen_center),
               cam->pos);
}
