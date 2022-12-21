#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_quad_renderer.h"

#include <math.h>
#include <iostream>

#define GRAVITY 9.81f * 40.f

// TODO: May want to implement this
#define VELOCITY_LIMIT 2000.f

void game_update(float dt) {

    PP::WinInfo* win = &glob->window;
    PP::Plane* p = &glob->plane;

    float adj_angle = (int)p->angle % 360;

    // Reset accelleration to recalculate it
    p->acc *= 0;


    // TODO: Implement in another way the wind, like this it doesn't work
    glm::vec2 wind_speed{};
    if (p->pos.y > win->h * 4.5f/5.f) {
        wind_speed.x = 0.0f;
        wind_speed.y = 0.0f;
    }
    wind_speed.y = 0.0f;

    // NOTE: This is the vertical resistance of the wind, when the plane goes down.
    //       This force makes it so the force of gravity gets balanced by the air resistance
    //       IT IS NOT THE LIFT
    float vertical_alar_surface = p->alar_surface * cos(glm::radians(adj_angle));
    float total_speed_y = -p->vel.y + wind_speed.y;
    float vertical_lift = vertical_alar_surface *
                            POW2(total_speed_y) * glob->air.density *
                            vertical_lift_coefficient(adj_angle) * 0.5f;
    vertical_lift = glm::abs(vertical_lift) *
                    glm::sign(-p->vel.y);
    vertical_lift *= (vertical_lift > 0) ? 1.f : 1.0f;
    float vertical_drag = vertical_alar_surface *
                            POW2(total_speed_y) * glob->air.density *
                            vertical_drag_coefficient(adj_angle) * 0.5f;
    if ((0 < adj_angle && adj_angle <= 90) || // 1o quadrante
        (180 < adj_angle && adj_angle <= 270) || // 3o quadrante
        (-360 < adj_angle && adj_angle <= -270) || // 1o quadrante
        (-180 < adj_angle && adj_angle <= -90))  { // 3o quadrante
        vertical_drag = -glm::abs(vertical_drag);
    } else {
        vertical_drag = glm::abs(vertical_drag);
    }
    p->acc.y += vertical_lift;
    p->acc.x += vertical_drag;

    float horizontal_alar_surface = p->alar_surface * sin(glm::radians(adj_angle));
    float total_speed_x = -p->vel.x + wind_speed.x;
    float horizontal_lift = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_lift_coefficient(adj_angle) * 0.5f;
    if ((0 < adj_angle && adj_angle <= 90) ||
        (180 < adj_angle && adj_angle <= 270) ||
        (-360 < adj_angle && adj_angle <= -270) ||
        (-180 < adj_angle && adj_angle <= -90))  {
        horizontal_lift = glm::abs(horizontal_lift) *
                            -glm::sign(p->vel.x);
    } else {
        horizontal_lift = glm::abs(horizontal_lift) *
                            glm::sign(p->vel.x);
    }
    float horizontal_drag = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_drag_coefficient(adj_angle) * 0.5f;
    horizontal_drag = glm::abs(horizontal_drag) *
                        glm::sign(total_speed_x);
    p->acc.y += horizontal_lift;
    p->acc.x += horizontal_drag;

    // Propulsion
    if (glob->input.boost) {
        float propulsion = 8.f;
        p->acc.x += propulsion * cos(glm::radians(adj_angle));
        p->acc.y += propulsion * -sin(glm::radians(adj_angle));
        std::cout << "PROP.x: " << propulsion * cos(glm::radians(adj_angle))
                    << ",  PROP.y: " << propulsion * -sin(glm::radians(adj_angle)) << "\n";
    }

    // NOTE: Remember to divide the force by the mass
    p->acc *= 1.f/p->mass;

    // NOTE: Gravity is already an accelleration so it doesn't need to be divided
    // Apply gravity based on the mass
    p->acc.y += GRAVITY;

    p->vel += p->acc * dt;

    p->pos += p->vel * dt + p->acc * dt * dt * 0.5f;

    std::cout << "SPEED X: " << total_speed_x << ",  SPEED Y: " << total_speed_y << std::endl;
    std::cout << "VL: " << vertical_lift <<
                ",  VD: " << vertical_drag <<
                ",  HL: " << horizontal_lift <<
                ",  HD: " << horizontal_drag << std::endl;

    // Input
    input_controller_update(win->glfw_win, &glob->input);

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


    // Plot twist
    /* if ((p->vel.x > 400 && p->angle < 0) || */
    /*     (p->vel.x < -400 && p->angle > 0)) { */
    /*     std::cout << "CHANGE" << std::endl; */
    /*     p->angle *= -1; */
    /* } */

    // Loop over window edged, pacman style
    if (p->pos.y > win->h) {
        p->pos.y -= win->h;
    }
    if (p->pos.y < 0) {
        p->pos.y += win->h;
    }
    if (p->pos.x > win->w) {
        p->pos.x -= win->w;
    }
    if (p->pos.x < 0) {
        p->pos.x += win->w;
    }

}

void game_draw(void) {
    PP::Plane* p = &glob->plane;
    PP::WinInfo* win = &glob->window;

    // High wind zone
    quad_render_add_queue(0.f, win->h * 4.5f/5.f, win->w, win->h * 0.5f/5.f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false);

    quad_render_add_queue(p->pos.x, p->pos.y, p->dim.x, p->dim.y, p->angle, glm::vec3(1.0f, 1.0f, 1.0f), true);

    if (glob->input.boost) {
        float bx = p->pos.x - p->dim.x/2.f * cos(glm::radians(p->angle));
        float by = p->pos.y + p->dim.x/2.f * sin(glm::radians(p->angle));
        quad_render_add_queue(bx, by, p->dim.y, p->dim.y, p->angle, glm::vec3(1.0f, 0.0f, 0.0f), true);
    }

    quad_render_draw(glob->rend.shaders[0]);
}

