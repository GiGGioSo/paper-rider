#include "pr_polygon.h"
#include "pr_renderer.h"
#include "pr_camera.h"
#include "pr_common.h"

#include "pr_globals.h"

// ###############
// ### SETTERS ###
// ###############
void portal_set_option_buttons(PR_Button *buttons) {
    // NOTE: Set up options buttons for the selected portal
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_PORTAL_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for portals");

        PR_Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_PORTAL_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH /
                             (SELECTED_PORTAL_OPTIONS+2);
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
                              strlen("TYPE")+1,
                              "TYPE");
                break;
            case 3:
                snprintf(button->text,
                              strlen("ENABLE_EFFECT")+1,
                              "ENABLE_EFFECT");
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
vec4f get_portal_color(PR_Portal *portal) {
    switch(portal->type) {
        case PR_INVERSE:
        {
            return _vec4f(0.f, 0.f, 0.f, 1.f);
            break;
        }
        case PR_SHUFFLE_COLORS:
        {
            return _diag_vec4f(1.f);
            break;
        }
    }
}

// ##############
// ### CREATE ###
// ##############
void
portal_init(PR_Portal *portal, vec2f pos, vec2f dim, float angle) {
    portal->body.pos = pos;
    portal->body.dim = dim;
    portal->body.angle = angle;
    portal->body.triangle = false;
    portal->type = PR_SHUFFLE_COLORS;
    portal->enable_effect = true;
}

// ##################
// ### COLLISIONS ###
// ##################
bool portal_contains_point(PR_Portal *portal, vec2f p) {
    return rect_contains_point(portal->body, p.x, p.y, false);
}
bool
portal_collides_with_plane(PR_Portal *portal, PR_Plane *plane, vec2f *crash_pos);
bool
portal_collides_with_rider(PR_Portal *portal, PR_Rider *rid, vec2f *crash_pos);

// #################
// ### RENDERING ###
// #################
void portal_render(PR_Portal *portal) {
    PR_Camera *cam = &glob->current_level.camera;
    if (portal->type == PR_SHUFFLE_COLORS) {
        PR_Rect b = portal->body;

        PR_Rect q1;
        q1.angle = 0.f;
        q1.triangle = false;
        q1.pos = b.pos;
        q1.dim = vec2f_mult(b.dim, 0.5f);
        renderer_add_queue_uni_rect(
            rect_in_camera_space(q1, cam),
            glob->colors[glob->current_level.current_gray],
            false);

        PR_Rect q2;
        q2.angle = 0.f;
        q2.triangle = false;
        q2.pos.x = b.pos.x + b.dim.x*0.5f;
        q2.pos.y = b.pos.y;
        q2.dim = vec2f_mult(b.dim, 0.5f);
        renderer_add_queue_uni_rect(
            rect_in_camera_space(q2, cam),
            glob->colors[glob->current_level.current_white],
            false);

        PR_Rect q3;
        q3.angle = 0.f;
        q3.triangle = false;
        q3.pos.x = b.pos.x;
        q3.pos.y = b.pos.y + b.dim.y*0.5f;
        q3.dim = vec2f_mult(b.dim, 0.5f);
        renderer_add_queue_uni_rect(
            rect_in_camera_space(q3, cam),
            glob->colors[glob->current_level.current_blue],
            false);

        PR_Rect q4;
        q4.angle = 0.f;
        q4.triangle = false;
        q4.pos.x = b.pos.x + b.dim.x*0.5f;
        q4.pos.y = b.pos.y + b.dim.y*0.5f;
        q4.dim = vec2f_mult(b.dim, 0.5f);
        renderer_add_queue_uni_rect(
            rect_in_camera_space(q4, cam),
            glob->colors[glob->current_level.current_red],
            false);
    } else {
        renderer_add_queue_uni_rect(rect_in_camera_space(portal->body, cam),
                               get_portal_color(portal),
                               false);
    }
}

void portal_render_info(PR_Portal *portal, float tx, float ty) {
    char buffer[99];
    memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    sprintf(buffer, "PORTAL INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "pos: (%f, %f)",
                 (portal)->body.pos.x, (portal)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "dim: (%f, %f)",
                 (portal)->body.dim.x, (portal)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "type: %s",
                 get_portal_type_name((portal)->type));
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    sprintf(buffer, "enable_effect: %s",
                 (portal)->enable_effect ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, _diag_vec4f(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

// ##############
// ### MODIFY ###
// ##############
void
portal_translate(PR_Portal *obs, vec2f move);

void
portal_rotate(PR_Portal *obs, float angle);

void
portal_set_size(PR_Portal *obs, vec2f size);
void
portal_resize(PR_Portal *obs, vec2f delta);
void
portal_scale(PR_Portal *obs, vec2f factor);

#endif//_PR_PORTAL_H_
