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
//  - Tidy up the rider movement variables and stuff
//  |
// #Tweaks
//  - Rider movement that feels good

// TODO: RESEARCH
// - flags you probably want to google: -MF -MMD (clang & gcc)
// - https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

// TODO: Will this always be some arbitrary numbers that kind of just works?
#define GRAVITY 9.81f * 50.f

#define VELOCITY_LIMIT 800.f

void apply_air_resistances(PR::Plane* p);

// Rider things
float rider_x_vel_limit = 500.f;
float rider_x_min_limit = 500.f;
float rider_y_vel_limit = 500.f;
float rider_x_movement_acc = 8000.f;
float air_friction = 1.5f;

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

float animation_frame = 0.f;
float animation_countdown = 0.f;

void game_update(float dt) {

    fps_counter++;
    time_from_last_fps_update += dt;
    if (time_from_last_fps_update > 1.f) {
        fps_to_display = fps_counter;
        fps_counter = 0;
        time_from_last_fps_update -= 1.f;

        std::cout << "FPS: " << fps_to_display << std::endl;
    }

    PR::WinInfo *win = &glob->window;
    PR::Plane *p = &glob->plane;
    PR::Camera *cam = &glob->cam;
    PR::Rider *rid = &glob->rider;
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

    // NOTE: The mass is greater if the rider is attached
    if (rid->attached) p->acc *= 1.f/(p->mass + rid->mass);
    else p->acc *= 1.f/p->mass;

    // NOTE: Gravity is already an accelleration so it doesn't need to be divided
    // Apply gravity based on the mass
    p->acc.y += GRAVITY;

    // NOTE: Motion of the plane
    p->vel += p->acc * dt;
    p->body.pos += p->vel * dt + p->acc * POW2(dt) * 0.5f;


    if (rid->attached) {

        cam->pos.x = lerp(cam->pos.x, p->render_zone.pos.x+p->render_zone.dim.x*0.5f, dt * cam->speed_multiplier);

        // NOTE: Changing plane angle based on the input
        if (input->left_right) {
            p->body.angle += 150.f * -input->left_right * dt;
        }

        rid->body.angle = p->render_zone.angle;
        rid->body.pos.x = p->render_zone.pos.x + (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
                    (p->render_zone.dim.y + rid->body.dim.y)*0.5f * sin(glm::radians(rid->body.angle)) -
                    (p->render_zone.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
        rid->body.pos.y = p->render_zone.pos.y + (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
                    (p->render_zone.dim.y + rid->body.dim.y)*0.5f * cos(glm::radians(rid->body.angle)) +
                    (p->render_zone.dim.x*0.2f) * sin(glm::radians(rid->body.angle));


        // NOTE: If the rider is attached, make it jump
        if (input->jump) {
            rid->attached = false;
            rid->vel.x = p->vel.x * 0.9f;
            rider_x_vel_limit = rid->vel.x;
            rid->vel.y = -400.f;
            rid->jump_time_elapsed = 0.f;
        }
    } else {
        cam->pos.x = lerp(cam->pos.x, rid->render_zone.pos.x+rid->render_zone.dim.x*0.5f, dt * cam->speed_multiplier);

        rid->acc.y += GRAVITY;

        if (input->left_right) {
            rid->acc.x += rider_x_movement_acc * input->left_right;
        } else {
            // NOTE: Make the player stop
            rid->acc.x += -rid->vel.x * air_friction;
        }

        if (rider_x_vel_limit < rider_x_min_limit) rider_x_vel_limit = rider_x_min_limit;

        rid->vel += rid->acc * dt;
        if (glm::abs(rid->vel.x) > rider_x_vel_limit)
            rid->vel.x = rider_x_vel_limit * glm::sign(rid->vel.x);

        if (glm::abs(rid->vel.y) > rider_y_vel_limit)
            rid->vel.y = rider_y_vel_limit * glm::sign(rid->vel.y);

        rider_x_vel_limit = rid->vel.x;

        rid->body.pos += rid->vel * dt + rid->acc * POW2(dt) * 0.5f;

        rid->jump_time_elapsed += dt;

        // NOTE: If rider not attached, check if recollided
        if (rect_are_colliding(&p->body, &rid->body) &&
            rid->jump_time_elapsed > 0.5f) {
            rid->attached = true;
            p->vel += (rid->vel - p->vel) * 0.5f;
            rid->vel *= 0.f;
        }

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

        if (rect_are_colliding(&p->body, obs)) {
            // NOTE: Plane colliding with an obstacle

            std::cout << "Plane collided with " << obs_index << std::endl;
        }
        if (rect_are_colliding(&rid->body, obs)) {
            // NOTE: Rider colliding with an obstacle

            std::cout << "Rider collided with " << obs_index << std::endl;
        }
    }

    // NOTE: Limit velocities
    if (glm::length(p->vel) > VELOCITY_LIMIT) {
        p->vel *= VELOCITY_LIMIT / glm::length(p->vel);
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

    // NOTE: Update the `render_zone`s based on the `body`s
    p->render_zone.pos = p->body.pos + (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;

    rid->render_zone.pos = rid->body.pos + (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->render_zone.angle = rid->body.angle;

    // NOTE: Animation based on the accelleration
    if (animation_countdown < 0) {
        if (glm::abs(p->acc.y) < 20.f) {
            animation_frame = 0.f;
            animation_countdown = 0.25f;
        } else {
            if (animation_frame == 1.f)
                animation_frame = (p->acc.y > 0) ? 0.f : 1.f;
            else if (animation_frame == 0.f)
                animation_frame = (p->acc.y > 0) ? 2.f : 1.f;
            else if (animation_frame == 2.f)
                animation_frame = (p->acc.y > 0) ? 2.f : 0.f;

            animation_countdown = 0.25f;
        }
    } else {
        animation_countdown -= dt;
    }
}

void apply_air_resistances(PR::Plane* p) {

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
    PR::Plane *p = &glob->plane;
    PR::WinInfo *win = &glob->window;
    PR::Camera *cam = &glob->cam;
    PR::Rider *rid = &glob->rider;


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

    // TODO: Implement the boost as an actual thing
    glm::vec2 p_cam_pos = rect_in_camera_space(p->render_zone, cam).pos;
    if (glob->input.boost) {
        float bx = p_cam_pos.x + p->render_zone.dim.x*0.5f -
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f * cos(glm::radians(p->render_zone.angle));
        float by = p_cam_pos.y + p->render_zone.dim.y*0.5f +
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f * sin(glm::radians(p->render_zone.angle));

        // NOTE: Rendering the boost of the plane
        quad_render_add_queue(bx, by,
                              p->render_zone.dim.y, p->render_zone.dim.y,
                              p->render_zone.angle,
                              glm::vec4(1.0f, 0.0f, 0.0f, 1.f),
                              true);
    }

    quad_render_add_queue(rect_in_camera_space(rid->render_zone, cam), glm::vec4(0.0f, 0.0f, 1.0f, 1.f), false);

    quad_render_draw(glob->rend.shaders[0]);

    // NOTE: Rendering plane texture
    quad_render_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                              texcoords_in_texture_space(animation_frame * 32.f, 0.f,
                                                         32.f, 8.f,
                                                         &glob->rend.global_sprite));

    quad_render_draw_tex(glob->rend.shaders[1], &glob->rend.global_sprite);

}
