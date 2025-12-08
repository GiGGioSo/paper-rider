#include "pr_polygon.h"
#include "pr_renderer.h"
#include "pr_camera.h"

#include "pr_globals.h"

vec4f obstacle_get_color(PR_Obstacle *obs) {
    if (obs->collide_rider && obs->collide_plane) {
        return glob->colors[glob->current_level.current_red];
    } else
    if (obs->collide_rider) {
        return glob->colors[glob->current_level.current_white];
    } else
    if (obs->collide_plane) {
        return glob->colors[glob->current_level.current_blue];
    } else {
        return glob->colors[glob->current_level.current_gray];
    }
}

bool obstacle_contains_point(PR_Obstacle *o, vec2f p) {
    return rect_contains_point(o->body, p.x, p.y, false);
}

bool obstacle_collides_with_plane(PR_Obstacle *obs, PR_Plane *plane, vec2f *crash_pos) {
    return rect_are_colliding(
            plane->body, obs->body,
            &crash_pos->x, &crash_pos->y);
}

bool obstacle_collides_with_rider(PR_Obstacle *obs, PR_Rider *rid, vec2f *crash_pos) {
    return rect_are_colliding(
            rid->body, obs->body,
            &crash_pos->x, &crash_pos->y);
}

void obstacle_render(PR_Obstacle *obs) {
    PR_Camera *cam = &glob->current_level.camera;
    PR_Rect obs_in_cam_pos = rect_in_camera_space(obs->body, cam);

    if (ABS(obs_in_cam_pos.pos.x) >
            ABS(obs_in_cam_pos.dim.x) + ABS(obs_in_cam_pos.dim.y) +
            ABS(GAME_WIDTH) + ABS(GAME_HEIGHT)
     || ABS(obs_in_cam_pos.pos.y) >
            ABS(obs_in_cam_pos.dim.x) + ABS(obs_in_cam_pos.dim.y) +
            ABS(GAME_WIDTH) + ABS(GAME_HEIGHT)
    ) return;

    renderer_add_queue_uni_rect(obs_in_cam_pos,
                          obstacle_get_color(obs),
                          false);
}

void obstacle_render_info(PR_Obstacle *obstacle, float tx, float ty) {
    char buffer[99];
    memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    sprintf(buffer, "OBSTACLE INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "pos: (%f, %f)",
                 (obstacle)->body.pos.x, (obstacle)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "dim: (%f, %f)",
                 (obstacle)->body.dim.x, (obstacle)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "angle: %f",
                 (obstacle)->body.angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "collide_plane: %s",
                 (obstacle)->collide_plane ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "collide_rider: %s",
                 (obstacle)->collide_rider ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

