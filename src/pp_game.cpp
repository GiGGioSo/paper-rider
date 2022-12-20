#include "pp_game.h"
#include "pp_globals.h"

#include <math.h>
#include <iostream>

#define GRAVITY 9.81f

void game_update(float dt) {

    PP::WinInfo* win = &glob->window;
    PP::Plane* p = &glob->plane;

    // Reset accelleration to recalculate it
    p->acc *= 0;

    // Apply gravity based on the mass
    p->acc.y += GRAVITY * p->mass;

    // TODO: Sign of the wind ?
    glm::vec2 wind_speed = glm::vec2(0.0f, 0.0f);

    // NOTE: This is the vertical resistance of the wind, when the plane goes down.
    //       This force makes it so the force of gravity gets balanced by the air resistance
    //       IT IS NOT THE LIFT
    // TODO: Remove this if
    if (p->vel.y > 0) {
        float vertical_alar_surface = p->alar_surface * cos(glm::radians(p->angle));
        float total_speed_y = p->vel.y + wind_speed.y;
        float vertical_lift = vertical_alar_surface *
                                POW2(total_speed_y) * glob->air.density *
                                vertical_lift_coefficient(p->angle) * 0.5f *
                                glm::sign(-p->vel.y);
        // TODO: figure out the sign of this thing based on the angle
        float vertical_drag = vertical_alar_surface *
                                POW2(total_speed_y) * glob->air.density *
                                vertical_drag_coefficient(p->angle) * 0.5f;
        vertical_drag = glm::abs(vertical_drag) *
                        glm::sign(cos(glm::radians(90.f + p->angle)));
        p->acc.y += vertical_lift;
        p->acc.x += vertical_drag;
    }

    // NOTE: This is the lift
    float horizontal_alar_surface = p->alar_surface * sin(glm::radians(p->angle));
    float total_speed_x = p->vel.x + wind_speed.x;
    float horizontal_lift = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_lift_coefficient(p->angle) * 0.5f;
    horizontal_lift = glm::abs(horizontal_lift) *
                        glm::sign(p->vel.x * cos(glm::radians(90.f + p->angle)));
    float horizontal_drag = horizontal_alar_surface *
                            POW2(total_speed_x) * glob->air.density *
                            horizontal_drag_coefficient(p->angle) * 0.5f;
    horizontal_drag = glm::abs(horizontal_drag) *
                        glm::sign(-p->vel.x);
    p->acc.y += horizontal_lift;
    p->acc.x += horizontal_drag;

    p->vel += p->acc * dt;
    p->pos += p->vel * dt + p->acc * dt * dt * 0.5f;

    std::cout << "VEL Y: " << p->vel.y << ",  HL: " << horizontal_drag << std::endl;

    // Plot twist
    if ((p->vel.x > 400 && p->angle < 0) ||
        (p->vel.x < -400 && p->angle > 0)) {
        std::cout << "CHANGE" << std::endl;
        p->angle *= -1;
    }

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
    quad_render_add_queue(p->pos.x, p->pos.y, p->dim.x, p->dim.y, p->angle, glm::vec3(1.0f, 1.0f, 1.0f), true);

    quad_render_draw(glob->rend.shaders[0]);
}

