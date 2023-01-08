#include "pp_rect.h"

#include "../include/glm/glm.hpp"

#include <math.h>
#include <iostream>

bool lines_are_colliding(float x0, float y0,
                         float x1, float y1,
                         float s0, float t0,
                         float s1, float t1);

// FIXME: Doens't work: t0 seems to be wrong, don't know why.
//          Also `lines_are_colliding` seems to be not working.
bool rect_are_colliding(Rect *r1, Rect *r2) {

    float center_x1 = r1->pos.x + r1->dim.x * 0.5f;
    float center_y1 = r1->pos.y + r1->dim.y * 0.5f;
    float r1_angle = glm::radians(r1->angle);

    float x0 = center_x1 +
                (r1->pos.x - center_x1) * cos(r1_angle) +
                (r1->pos.y - center_y1) * sin(r1_angle);
    float y0 = center_y1 +
                (r1->pos.x - center_x1) * sin(r1_angle) -
                (r1->pos.y - center_y1) * cos(r1_angle);
    float x1 = center_x1 +
                (r1->pos.x + r1->dim.x - center_x1) * cos(r1_angle) +
                (r1->pos.y + r1->dim.y - center_y1) * sin(r1_angle);
    float y1 = center_y1 +
                (r1->pos.x + r1->dim.x - center_x1) * sin(r1_angle) -
                (r1->pos.y + r1->dim.y - center_y1) * cos(r1_angle);


    float center_x2 = r2->pos.x + r2->dim.x * 0.5f;
    float center_y2 = r2->pos.y + r2->dim.y * 0.5f;
    float r2_angle = glm::radians(r2->angle);

    float s0 = center_x2 +
                (r2->pos.x - center_x2) * cos(r2_angle) +
                (r2->pos.y - center_y2) * sin(r2_angle);
    float t0 = center_y2 +
                (r2->pos.x - center_x2) * sin(r2_angle) -
                (r2->pos.y - center_y2) * cos(r2_angle);
    float s1 = center_x2 +
                (r2->pos.x + r2->dim.x - center_x2) * cos(r2_angle) +
                (r2->pos.y + r2->dim.y - center_y2) * sin(r2_angle);
    float t1 = center_y2 +
                (r2->pos.x + r2->dim.x - center_x2) * sin(r2_angle) -
                (r2->pos.y + r2->dim.y - center_y2) * cos(r2_angle);

    std::cout <<
        "s0  : " << s0 <<
        ", t0  : " << t0 <<
        ";  x0: " << x0 <<
        ", y0: " << y0 <<
        std::endl;

    // NOTE: line x0,y0:x1,y0 collision with all r2 lines
    if (lines_are_colliding(
            x0, y0,
            x1, y0,
            s0, t0,
            s1, t0
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y0,
            s1, t0,
            s1, t1
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y0,
            s0, t0,
            s0, t1
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y0,
            s0, t1,
            s1, t1
        )) return true;

    // NOTE: line x1,y0:x1,y1 collision with all r2 lines
    if (lines_are_colliding(
            x1, y0,
            x1, y1,
            s0, t0,
            s1, t0
        )) return true;
    if (lines_are_colliding(
            x1, y0,
            x1, y1,
            s1, t0,
            s1, t1
        )) return true;
    if (lines_are_colliding(
            x1, y0,
            x1, y1,
            s0, t0,
            s0, t1
        )) return true;
    if (lines_are_colliding(
            x1, y0,
            x1, y1,
            s0, t1,
            s1, t1
        )) return true;

    // NOTE: line x0,y0:x0,y1 collision with all r2 lines
    if (lines_are_colliding(
            x0, y0,
            x0, y1,
            s0, t0,
            s1, t0
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x0, y1,
            s1, t0,
            s1, t1
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x0, y1,
            s0, t0,
            s0, t1
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x0, y1,
            s0, t1,
            s1, t1
        )) return true;

    // NOTE: line x0,y1:x1,y1 collision with all r2 lines
    if (lines_are_colliding(
            x0, y1,
            x1, y1,
            s0, t0,
            s1, t0
        )) return true;
    if (lines_are_colliding(
            x0, y1,
            x1, y1,
            s1, t0,
            s1, t1
        )) return true;
    if (lines_are_colliding(
            x0, y1,
            x1, y1,
            s0, t0,
            s0, t1
        )) return true;
    if (lines_are_colliding(
            x0, y1,
            x1, y1,
            s0, t1,
            s1, t1
        )) return true;

    return false;
}

bool lines_are_colliding(float x0, float y0,
                         float x1, float y1,
                         float s0, float t0,
                         float s1, float t1) {

    // NOTE: The m (inclination) of the lines created by the points
    float m1 = (y1-y0) / (x1-x0);
    float m2 = (t1-t0) / (s1-s0);

    // NOTE: The q (offset at x=0) of the lines created by the points
    float q1 = y0 - x0 * m1;
    float q2 = t0 - s0 * m2;

    // NOTE: Coordinates of the resulting point
    float x = (q2-q1) / (m1-m2);
    float y = m1 * x + q1;

    // NOTE: The resulting point must be inside both lines
    bool result = (x0 <= x && x <= x1) &&
                    (y0 <= y && y <= y1) &&
                  (s0 <= x && x <= s1) &&
                    (t0 <= y && y <= t1);

    return result;
}
