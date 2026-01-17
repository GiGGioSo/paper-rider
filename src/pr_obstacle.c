#include "pr_polygon.h"
#include "pr_renderer.h"
#include "pr_camera.h"
#include "pr_common.h"

#include "pr_globals.h"

// ###############
// ### SETTERS ###
// ###############
void obstacle_set_option_buttons(PR_Button *buttons) {
    // NOTE: Set up options buttons for the selected obstacle
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_OBSTACLE_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for obstacle");

        PR_Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_OBSTACLE_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH / (SELECTED_OBSTACLE_OPTIONS+2);
        button->body.dim.y = GAME_HEIGHT / 10;

        switch(option_button_index) {
            case 0:
                snprintf(button->text,
                              strlen("WIDTH")+1,
                              "WIDTH");
                break;
            case 1:
                snprintf(button->text,
                              strlen("HEIGHT")+1,
                              "HEIGHT");
                break;
            case 2:
                snprintf(button->text,
                              strlen("ANGLE")+1,
                              "ANGLE");
                break;
            case 3:
                snprintf(button->text,
                              strlen("TRIANGLE")+1,
                              "TRIANGLE");
                break;
            case 4:
                snprintf(button->text,
                              strlen("COLLIDE_PLANE")+1,
                              "COLLIDE_PLANE");
                break;
            case 5:
                snprintf(button->text,
                              strlen("COLLIDE_RIDER")+1,
                              "COLLIDE_RIDER");
                break;
            default:
                snprintf(button->text,
                              strlen("UNDEFINED")+1,
                              "UNDEFINED");
                break;
        }
        button->col = _vec4f(0.5f, 0.5f, 0.5f, 1.f);
    }
}

// ###############
// ### GETTERS ###
// ###############
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

// ##############
// ### CREATE ###
// ##############
void obstacle_init(PR_Obstacle *obs, vec2f pos, vec2f dim, float angle) {
    obs->body.pos = pos;
    obs->body.dim = dim;
    obs->body.angle = angle;
    obs->body.triangle = false;
    obs->collide_plane = false;
    obs->collide_rider = false;
}

// ##################
// ### COLLISIONS ###
// ##################
bool obstacle_contains_point(PR_Obstacle *o, vec2f p) {
    return rect_contains_point(o->body, p.x, p.y, false);
}

bool obstacle_collides_with_plane(PR_Obstacle *obs, PR_Plane *plane, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            plane->body, obs->body,
            crash_pos_x, crash_pos_y);
}

bool obstacle_collides_with_rider(PR_Obstacle *obs, PR_Rider *rid, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            rid->body, obs->body,
            crash_pos_x, crash_pos_y);
}

// #################
// ### RENDERING ###
// #################
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

// ##############
// ### MODIFY ###
// ##############
void obstacle_translate(PR_Obstacle *obs, vec2f move) {
    obs->body.pos = vec2f_sum(obs->body.pos, move);
}

void obstacle_rotate(PR_Obstacle *obs, float angle) {
    obs->body.angle += angle;
}

void obstacle_set_size(PR_Obstacle *obs, vec2f size) {
    PR_ASSERT(size.x > 0 && size.y > 0);
    obs->body.dim = size;
}
void obstacle_resize(PR_Obstacle *obs, vec2f delta) {
    vec2f new_size = vec2f_sum(obs->body.dim, delta);
    PR_ASSERT(new_size.x > 0 && new_size.y > 0);
    obs->body.dim = new_size;
}
void obstacle_scale(PR_Obstacle *obs, vec2f factor) {
    PR_ASSERT(factor.x > 0 && factor.y > 0);
    obs->body.dim.x *= factor.x;
    obs->body.dim.y *= factor.y;
}

