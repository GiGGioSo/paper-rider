#include "pr_polygon.h"
#include "pr_renderer.h"
#include "pr_camera.h"
#include "pr_common.h"

#include "pr_globals.h"

// ###############
// ### SETTERS ###
// ###############
void boostpad_set_option_buttons(PR_Button *buttons) {
    // NOTE: Set up options buttons for the selected boost
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_BOOST_OPTIONS;
        ++option_button_index) {

        assert((option_button_index <
                    SELECTED_MAX_OPTIONS)
                && "Selected options out of bounds for boosts");

        PR_Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_BOOST_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH / (SELECTED_BOOST_OPTIONS+2);
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
                              strlen("BOOST_ANGLE")+1,
                              "BOOST_ANGLE");
                break;
            case 5:
                snprintf(button->text,
                              strlen("BOOST_POWER")+1,
                              "BOOST_POWER");
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

// ##############
// ### CREATE ###
// ##############
void boostpad_init(PR_BoostPad *pad, vec2f pos, vec2f dim, float angle) {
    pad->body.pos = pos;
    pad->body.dim = dim;
    pad->body.angle = angle;
    pad->body.triangle = false;
    pad->boost_angle = 0.f;
    pad->boost_power = 0.f;
}

// ##################
// ### COLLISIONS ###
// ##################
bool boostpad_contains_point(PR_BoostPad *pad, vec2f p) {
    return rect_contains_point(pad->body, p.x, p.y, false);
}
bool boostpad_collides_with_plane(PR_BoostPad *pad, PR_Plane *plane, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            plane->body, pad->body,
            crash_pos_x, crash_pos_y);
}
bool boostpad_collides_with_rider(PR_BoostPad *pad, PR_Rider *rid, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            rid->body, pad->body,
            crash_pos_x, crash_pos_y);
}

// #################
// ### RENDERING ###
// #################
void boostpad_render(PR_BoostPad *pad) {
    PR_Camera *cam = &glob->current_level.camera;

    PR_Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

    if (ABS(pad_in_cam_pos.pos.x) >
            ABS(pad_in_cam_pos.dim.x) + ABS(pad_in_cam_pos.dim.y) +
            ABS(GAME_WIDTH) + ABS(GAME_HEIGHT)
     || ABS(pad_in_cam_pos.pos.y) >
            ABS(pad_in_cam_pos.dim.x) + ABS(pad_in_cam_pos.dim.y) +
            ABS(GAME_WIDTH) + ABS(GAME_HEIGHT)
    ) return;

    renderer_add_queue_uni_rect(pad_in_cam_pos,
                          _vec4f(0.f, 1.f, 0.f, 1.f),
                          false);
    
    if (pad->body.triangle) {
        pad_in_cam_pos.pos.x += pad_in_cam_pos.dim.x * 0.25f;
        pad_in_cam_pos.pos.y += pad_in_cam_pos.dim.y * 0.5f;
        pad_in_cam_pos.dim = vec2f_mult(pad_in_cam_pos.dim, 0.4f);
    } else  {
        pad_in_cam_pos.pos = vec2f_sum(
                pad_in_cam_pos.pos,
                vec2f_mult(pad_in_cam_pos.dim, 0.5f));
        pad_in_cam_pos.dim = vec2f_mult(pad_in_cam_pos.dim, 0.5f);
    }
    pad_in_cam_pos.angle = pad->boost_angle;
    renderer_add_queue_tex_rect(pad_in_cam_pos,
                           texcoords_in_texture_space(
                               0, 8, 96, 32,
                               glob->rend_res.global_sprite, false),
                           true);
}

void boostpad_render_info(PR_BoostPad *boost, float tx, float ty) {
    char buffer[99];
    memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    sprintf(buffer, "BOOST INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "pos: (%f, %f)",
            (boost)->body.pos.x, (boost)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "dim: (%f, %f)",
            (boost)->body.dim.x, (boost)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "angle: %f",
            (boost)->body.angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "boost_angle: %f",
            (boost)->boost_angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "boost_power: %f",
            (boost)->boost_power);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}


// ##############
// ### MODIFY ###
// ##############
void boostpad_translate(PR_BoostPad *pad, vec2f move) {
    pad->body.pos = vec2f_sum(pad->body.pos, move);
}

void boostpad_rotate(PR_BoostPad *pad, float angle) {
    pad->body.angle += angle;
}

void boostpad_set_size(PR_BoostPad *pad, vec2f size) {
    PR_ASSERT(size.x > 0 && size.y > 0);
    pad->body.dim = size;
}
void boostpad_resize(PR_BoostPad *pad, vec2f delta) {
    vec2f new_size = vec2f_sum(pad->body.dim, delta);
    PR_ASSERT(new_size.x > 0 && new_size.y > 0);
    pad->body.dim = new_size;
}
void boostpad_scale(PR_BoostPad *pad, vec2f factor) {
    PR_ASSERT(factor.x > 0 && factor.y > 0);
    pad->body.dim.x *= factor.x;
    pad->body.dim.y *= factor.y;
}
