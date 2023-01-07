#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_quad_renderer.h"

#include <cassert>
#include <math.h>
#include <iostream>

// TODO:
//  - 2D collision - obstacles
//  - Proper wind
//  - Find textures
//  - Animation system
//  - Particle system
//  - Sound system


// TODO: Will this always be some arbitrary number that kind of just works?
#define GRAVITY 9.81f * 50.f

#define VELOCITY_LIMIT 800.f

void game_update(float dt) {

    PP::WinInfo* win = &glob->window;
    PP::Plane* p = &glob->plane;
    PP::Camera* cam = &glob->cam;

    assert(-360.f <= p->angle && p->angle <= 360.f);

    // Input
    input_controller_update(win->glfw_win, &glob->input);

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;

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
        p->acc.x += propulsion * cos(glm::radians(p->angle));
        p->acc.y += propulsion * -sin(glm::radians(p->angle));
        std::cout << "PROP.x: " << propulsion * cos(glm::radians(p->angle))
                    << ",  PROP.y: " << propulsion * -sin(glm::radians(p->angle)) << "\n";
    }

    // NOTE: Remember to divide all the forces applied by the mass
    p->acc *= 1.f/p->mass;

    // NOTE: Gravity is already an accelleration so it doesn't need to be divided
    // Apply gravity based on the mass
    p->acc.y += GRAVITY;

    // NOTE: Motion of the plane
    p->vel += p->acc * dt;
    p->pos += p->vel * dt + p->acc * dt * dt * 0.5f;

    cam->pos.x = lerp(cam->pos.x, p->pos.x, dt * cam->speed_multiplier);

    std::cout << "SPEED X: " << p->vel.x
            << ",  SPEED Y: " << p->vel.y
            << ",  ANGLE: " << p->angle
            << std::endl;

    float angle_boost = 150.f;
    if (glm::abs(glob->input.vertical) > 0.2f) {
        angle_boost = angle_boost * glm::abs(glob->input.vertical);
    }

    if (glob->input.move_up) {
        p->angle += angle_boost * dt;
    } else if (glob->input.move_down) {
        p->angle -= angle_boost * dt;
    }

    // Limitations
    if (p->angle > 360.f) {
        p->angle -= 360.f;
    }
    if (p->angle < -360) {
        p->angle += 360.f;
    }
    // Limit velocity
    if (POW2(p->vel.x) + POW2(p->vel.y) > POW2(VELOCITY_LIMIT)) {
        std::cout << "Limited!!\n";
        float vel_angle = atan(p->vel.y / p->vel.x);
        p->vel.x = glm::abs(VELOCITY_LIMIT * cos(vel_angle)) *
                    glm::sign(p->vel.x);
        p->vel.y = glm::abs(VELOCITY_LIMIT * sin(vel_angle)) *
                    glm::sign(p->vel.y);
    }

    // Loop over window edged, pacman style,
    // but only on the top and bottom
    if (p->pos.y > win->h) {
        p->pos.y -= win->h;
    }
    if (p->pos.y < 0) {
        p->pos.y += win->h;
    }
}

void apply_air_resistances(PP::Plane* p) {

    float vertical_alar_surface = p->alar_surface * cos(glm::radians(p->angle));

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_lift = vertical_alar_surface *
                            POW2(p->vel.y) * glob->air.density *
                            vertical_lift_coefficient(p->angle) * 0.5f;
    vertical_lift = glm::abs(vertical_lift) *
                    glm::sign(-p->vel.y);

    // NOTE: This is done so the plane doesn't instantly brake when going up
    /* vertical_lift = (vertical_lift > 0 && p->vel.y < 0) ? */
    /*                     vertical_lift * 0.5f : */
    /*                     vertical_lift; */

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_drag = vertical_alar_surface *
                            POW2(p->vel.y) * glob->air.density *
                            vertical_drag_coefficient(p->angle) * 0.5f;
    if ((0 < p->angle && p->angle <= 90) || // 1o quadrante
        (180 < p->angle && p->angle <= 270) || // 3o quadrante
        (-360 < p->angle && p->angle <= -270) || // 1o quadrante
        (-180 < p->angle && p->angle <= -90))  { // 3o quadrante
        vertical_drag = -glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    } else {
        vertical_drag = glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    }

    p->acc.y += vertical_lift;
    p->acc.x += vertical_drag;

    float horizontal_alar_surface = p->alar_surface * sin(glm::radians(p->angle));
    float total_speed_x = -p->vel.x;
    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)

    // TODO: Divide by some value when the value is positive,
    //       like it's done for the `vertical_lift`
    float horizontal_lift = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_lift_coefficient(p->angle) * 0.5f;
    if ((0 < p->angle && p->angle <= 90) ||
        (180 < p->angle && p->angle <= 270) ||
        (-360 < p->angle && p->angle <= -270) ||
        (-180 < p->angle && p->angle <= -90))  {
        horizontal_lift = glm::abs(horizontal_lift) *
                            -glm::sign(p->vel.x);
    } else {
        horizontal_lift = glm::abs(horizontal_lift) *
                            glm::sign(p->vel.x);
    }

    /* horizontal_lift = (horizontal_lift > 0) ? horizontal_lift * 0.5f : horizontal_lift; */

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float horizontal_drag = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_drag_coefficient(p->angle) * 0.5f;
    horizontal_drag = glm::abs(horizontal_drag) *
                        glm::sign(total_speed_x);

    // NOTE: This is done so the plane doesn't instantly brake when going up
    /* horizontal_drag = (glm::sign(horizontal_drag) != glm::sign(p->vel.x) && */
    /*                     p->vel.y < 0) ? */
    /*                     horizontal_drag * 0.5f : */
    /*                     horizontal_drag; */

    p->acc.y += horizontal_lift;
    p->acc.x += horizontal_drag;

    std::cout << "VL: " << vertical_lift <<
                ",  VD: " << vertical_drag <<
                ",  HL: " << horizontal_lift <<
                ",  HD: " << horizontal_drag << std::endl;
}

void game_draw(void) {
    PP::Plane* p = &glob->plane;
    PP::WinInfo* win = &glob->window;
    PP::Camera* cam = &glob->cam;

    glm::vec2 p_cam_pos = p->pos - cam->pos + glm::vec2(win->w/2.f, win->h/2.f);

    // High wind zone
    quad_render_add_queue(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false);

    quad_render_add_queue(p_cam_pos.x, p_cam_pos.y, p->dim.x, p->dim.y, p->angle, glm::vec3(1.0f, 1.0f, 1.0f), true);

    if (glob->input.boost) {
        float bx = p_cam_pos.x - p->dim.x/2.f * cos(glm::radians(p->angle));
        float by = p_cam_pos.y + p->dim.x/2.f * sin(glm::radians(p->angle));
        quad_render_add_queue(bx, by, p->dim.y, p->dim.y, p->angle, glm::vec3(1.0f, 0.0f, 0.0f), true);
    }

    // "Obstacles"
    for(int i = 0; i < 400; i++) {

        float obstacle_x = i * win->w * 0.8f - cam->pos.x + win->w/2.f;
        float obstacle_y = win->h * 0.5f - cam->pos.y + win->h/2.f;

        quad_render_add_queue(obstacle_x, obstacle_y, win->w * 0.1f, win->h * 0.3f, 0.f, glm::vec3(1.f, 0.2f, 0.2f), false);

    }

    quad_render_draw(glob->rend.shaders[0]);
}

