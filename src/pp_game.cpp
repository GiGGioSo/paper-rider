#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_quad_renderer.h"
#include "pp_rect.h"

#include <cassert>
#include <math.h>
#include <iostream>

// TODO:
//  - Proper wind
//  - Find textures
//  - Animation system
//  - Particle system
//  - Sound system
//  - Level creation system
//  |
// #Mechanics
//  - Make changing angle more or less difficult
//  - Control the rider when jumped out from the plane
//  - Make the plane change angle alone when the rider jumps off

// TODO: RESEARCH
// - flags you probably want to google: -MF -MMD (clang & gcc)
// - https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

float debug_buffer[32];
int debug_index;

// TODO: Will this always be some arbitrary numbers that kind of just works?
#define GRAVITY 9.81f * 50.f

#define VELOCITY_LIMIT 1000.f

void apply_air_resistances(PP::Plane* p);

void game_update(float dt) {

    fps_counter++;
    time_from_last_fps_update += dt;
    if (time_from_last_fps_update > 1.f) {
        fps_to_display = fps_counter;
        fps_counter = 0;
        time_from_last_fps_update -= 1.f;

        std::cout << "FPS: " << fps_to_display << std::endl;
    }


    PP::WinInfo *win = &glob->window;
    PP::Plane *p = &glob->plane;
    PP::Camera *cam = &glob->cam;
    PP::Rider *rid = &glob->rider;
    InputController *input = &glob->input;

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    // NOTE: Update input
    input_controller_update(win->glfw_win, &glob->input);

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;
    rid->acc *= 0;

    apply_air_resistances(p);

    // Wind effect (more like a propulsion)
    /* if (p->pos.y > win->h * 0.6f) { */
    /*     float wind = 10.f; */
    /*     p->acc.x += wind; */
    /* } */

    // Propulsion
    // TODO: Could this be a "powerup" or something?
    if (glob->input.boost) {
        float propulsion = 8.f;
        p->acc.x += propulsion * cos(glm::radians(p->body.angle));
        p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
        /* std::cout << "PROP.x: " << propulsion * cos(glm::radians(p->body.angle)) */
        /*             << ",  PROP.y: " << propulsion * -sin(glm::radians(p->body.angle)) << "\n"; */
    }

    // NOTE: Remember to divide all the forces applied by the mass,
    //          the mass depends on wether the rider is on the plane
    if (rid->attached) p->acc *= 1.f/(p->mass + rid->mass);
    else p->acc *= 1.f/p->mass;

    // NOTE: Gravity is already an accelleration so it doesn't need to be divided
    // Apply gravity based on the mass
    p->acc.y += GRAVITY;

    // NOTE: Motion of the plane
    p->vel += p->acc * dt;
    p->body.pos += p->vel * dt + p->acc * POW2(dt) * 0.5f;

    cam->pos.x = lerp(cam->pos.x, p->body.pos.x, dt * cam->speed_multiplier);

    if (rid->attached) {
        rid->body.pos = p->body.pos + p->body.dim * 0.5f - rid->body.dim *0.5f;
        rid->body.angle = p->body.angle;
    } else {
        // NOTE: Motion of the rider, if not attached
        rid->acc.y += GRAVITY;
        rid->vel += rid->acc * dt;
        rid->body.pos += rid->vel * dt + rid->acc * POW2(dt) * 0.5f;

        rid->jump_time_elapsed += dt;
    }

    // NOTE: If rider not attached, check if recollided
    if (!rid->attached &&
        rect_are_colliding(&p->body, &rid->body) &&
        rid->jump_time_elapsed > 0.5f) {

        rid->attached = true;
        rid->vel *= 0.f;
    }


    // NOTE: Changing angle based on the input
    if (input->vertical) {
        p->body.angle += 150.f * input->vertical * dt;
    }

    // NOTE: If the rider is attached, make it jump
    if (rid->attached &&
        input->jump) {
        rid->attached = false;
        rid->vel.x = p->vel.x;
        rid->vel.y = -400.f;
        rid->jump_time_elapsed = 0.f;
    }


    // NOTE: Limiting the angle
    if (p->body.angle > 360.f) {
        p->body.angle -= 360.f;
    }
    if (p->body.angle < -360) {
        p->body.angle += 360.f;
    }

    // NOTE: Checking collision with obstacles
    for (int obs_index = 0; obs_index < ARRAY_LENGTH(glob->obstacles); obs_index++) {

        Rect *obs = &glob->obstacles[obs_index];

        if (p->body.pos.x + p->body.dim.x < obs->pos.x ||
            p->body.pos.x > obs->pos.x + obs->dim.x) continue;

        if (rect_are_colliding(&p->body, obs)) {
            std::cout << "Colliding with " << obs_index << std::endl;
        }
    }

    // NOTE: Limit velocities
    if (glm::length(p->vel) > VELOCITY_LIMIT) {
        p->vel *= VELOCITY_LIMIT / glm::length(p->vel);
    }
    if (!rid->attached && glm::length(rid->vel) > VELOCITY_LIMIT) {
        rid->vel *= VELOCITY_LIMIT / glm::length(rid->vel);
    }

    // NOTE: Loop over window edged pacman style,
    //       but only on the top and bottom
    if (p->body.pos.y + p->body.dim.y/2 > win->h) {
        p->body.pos.y -= win->h;
    }
    if (p->body.pos.y + p->body.dim.y/2 < 0) {
        p->body.pos.y += win->h;
    }

    if (!rid->attached) {
        if (rid->body.pos.y + rid->body.dim.y * 0.5f > win->h) {
            rid->body.pos.y -= win->h;
        }
        if (rid->body.pos.y + rid->body.dim.y * 0.5f < 0) {
            rid->body.pos.y += win->h;
        }
    }
}

void apply_air_resistances(PP::Plane* p) {

    float vertical_alar_surface = p->alar_surface * cos(glm::radians(p->body.angle));

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_lift = vertical_alar_surface *
                            POW2(p->vel.y) * glob->air.density *
                            vertical_lift_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `vertical_lift`
    vertical_lift = glm::abs(vertical_lift) *
                    glm::sign(-p->vel.y);

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_drag = vertical_alar_surface *
                            POW2(p->vel.y) * glob->air.density *
                            vertical_drag_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `vertical_drag`
    if ((0 < p->body.angle && p->body.angle <= 90) || // 1o quadrante
        (180 < p->body.angle && p->body.angle <= 270) || // 3o quadrante
        (-360 < p->body.angle && p->body.angle <= -270) || // 1o quadrante
        (-180 < p->body.angle && p->body.angle <= -90))  { // 3o quadrante
        vertical_drag = -glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    } else {
        vertical_drag = glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    }

    p->acc.y += vertical_lift;
    p->acc.x += vertical_drag;

    float horizontal_alar_surface = p->alar_surface * sin(glm::radians(p->body.angle));

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float horizontal_lift = horizontal_alar_surface *
                            POW2(p->vel.x) * glob->air.density *
                            horizontal_lift_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `horizontal_lift`
    if ((0 < p->body.angle && p->body.angle <= 90) ||
        (180 < p->body.angle && p->body.angle <= 270) ||
        (-360 < p->body.angle && p->body.angle <= -270) ||
        (-180 < p->body.angle && p->body.angle <= -90))  {
        horizontal_lift = glm::abs(horizontal_lift) *
                            -glm::sign(p->vel.x);
    } else {
        horizontal_lift = glm::abs(horizontal_lift) *
                            glm::sign(p->vel.x);
    }

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float horizontal_drag = horizontal_alar_surface *
                            POW2(p->vel.x) * glob->air.density *
                            horizontal_drag_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `horizontal_drag`
    horizontal_drag = glm::abs(horizontal_drag) *
                        glm::sign(-p->vel.x);

    p->acc.y += horizontal_lift;
    p->acc.x += horizontal_drag;

    /* std::cout << "VL: " << vertical_lift << */
    /*             ",  VD: " << vertical_drag << */
    /*             ",  HL: " << horizontal_lift << */
    /*             ",  HD: " << horizontal_drag << std::endl; */
}

void game_draw(void) {
    PP::Plane *p = &glob->plane;
    PP::WinInfo *win = &glob->window;
    PP::Camera *cam = &glob->cam;
    PP::Rider *rid = &glob->rider;


    // High wind zone
    /* quad_render_add_queue(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false); */

    // NOTE: Rendering the obstacles
    for(int obs_index = 0; obs_index < ARRAY_LENGTH(glob->obstacles); obs_index++) {

        Rect *obs = &glob->obstacles[obs_index];

        Rect obs_in_cam_pos = rect_in_camera_space(*obs, cam);

        if (obs_in_cam_pos.pos.x < -((float)win->w) ||
            obs_in_cam_pos.pos.x > win->w * 2.f) continue;

        quad_render_add_queue(obs_in_cam_pos, glm::vec4(1.f, 0.2f, 0.2f, 1.f), false);

    }

    // NOTE: Rendering the plane
    quad_render_add_queue(rect_in_camera_space(p->body, cam), glm::vec4(1.0f, 1.0f, 1.0f, 1.f), false);

    glm::vec2 p_cam_pos = p->body.pos - cam->pos + glm::vec2(win->w*0.5f, win->h*0.5f);
    if (glob->input.boost) {
        float bx = p_cam_pos.x + p->body.dim.x*0.5f -
                    (p->body.dim.x-p->body.dim.y)*0.5f * cos(glm::radians(p->body.angle));
        float by = p_cam_pos.y + p->body.dim.y*0.5f +
                    (p->body.dim.x-p->body.dim.y)*0.5f * sin(glm::radians(p->body.angle));

        // NOTE: Rendering the boost of the plane
        quad_render_add_queue(bx, by,
                              p->body.dim.y, p->body.dim.y,
                              p->body.angle,
                              glm::vec4(1.0f, 0.0f, 0.0f, 1.f),
                              true);
    }

    quad_render_add_queue(rect_in_camera_space(rid->body, cam), glm::vec4(0.0f, 0.0f, 1.0f, .5f), false);

    quad_render_draw(glob->rend.shaders[0]);
}
