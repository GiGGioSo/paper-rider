#include "pr_portal.h"

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
vec4f portal_get_color(PR_Portal *portal) {
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
const char *portal_get_type_name(PR_PortalType t) {
    switch (t) {
        case PR_INVERSE:
            return "INVERSE";
        case PR_SHUFFLE_COLORS:
            return "SHUFFLE_COLORS";
        default:
            return "UNKNOWN";
    }
    return "";
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
bool portal_collides_with_plane(PR_Portal *portal, PR_Plane *plane, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            plane->body, portal->body,
            crash_pos_x, crash_pos_y);
}
bool portal_collides_with_rider(PR_Portal *portal, PR_Rider *rid, vec2f *crash_pos) {
    float *crash_pos_x = NULL;
    float *crash_pos_y = NULL;
    if (crash_pos) {
        crash_pos_x = &crash_pos->x;
        crash_pos_y = &crash_pos->y;
    }
    return rect_are_colliding(
            rid->body, portal->body,
            crash_pos_x, crash_pos_y);
}

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
                               portal_get_color(portal),
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
                 portal_get_type_name((portal)->type));
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
void portal_translate(PR_Portal *portal, vec2f move) {
    portal->body.pos = vec2f_sum(portal->body.pos, move);
}

void portal_rotate(PR_Portal *portal, float angle) {
    portal->body.angle += angle;
}

void portal_set_size(PR_Portal *portal, vec2f size) {
    PR_ASSERT(size.x > 0 && size.y > 0);
    portal->body.dim = size;
}

void portal_resize(PR_Portal *portal, vec2f delta) {
    vec2f new_size = vec2f_sum(portal->body.dim, delta);
    PR_ASSERT(new_size.x > 0 && new_size.y > 0);
    portal->body.dim = new_size;
}

void portal_scale(PR_Portal *portal, vec2f factor) {
    PR_ASSERT(factor.x > 0 && factor.y > 0);
    portal->body.dim.x *= factor.x;
    portal->body.dim.y *= factor.y;
}
