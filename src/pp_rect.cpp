#include "pp_rect.h"

#include "../include/glm/glm.hpp"

#include <math.h>
#include <iostream>

bool lines_are_colliding(float x0, float y0,
                         float x1, float y1,
                         float s0, float t0,
                         float s1, float t1,
                         float* x, float* y);

bool rect_are_colliding(Rect* r1, Rect* r2) {
    if (glm::abs(r1->pos.x - r2->pos.x) >
            r1->dim.x + r1->dim.y + r2->dim.x + r2->dim.y
     || glm::abs(r1->pos.y - r2->pos.y) >
            r1->dim.x + r1->dim.y + r2->dim.x + r2->dim.y
    ) return false;

    float center_x1 = r1->pos.x + r1->dim.x * 0.5f;
    float center_y1 = r1->pos.y + r1->dim.y * 0.5f;
    float r1_angle = glm::radians(-r1->angle);
    float cos_r1 = cos(r1_angle);
    float sin_r1 = sin(r1_angle);

    float x0 = center_x1 +
                (r1->pos.x - center_x1) * cos_r1 -
                (r1->pos.y - center_y1) * sin_r1;
    float y0 = center_y1 +
                (r1->pos.x - center_x1) * sin_r1 +
                (r1->pos.y - center_y1) * cos_r1;
    float x1 = center_x1 +
                (r1->pos.x + r1->dim.x - center_x1) * cos_r1 -
                (r1->pos.y - center_y1) * sin_r1;
    float y1 = center_y1 +
                (r1->pos.x + r1->dim.x - center_x1) * sin_r1 +
                (r1->pos.y - center_y1) * cos_r1;
    float x2 = center_x1 +
                (r1->pos.x + r1->dim.x - center_x1) * cos_r1 -
                (r1->pos.y + r1->dim.y - center_y1) * sin_r1;
    float y2 = center_y1 +
                (r1->pos.x + r1->dim.x - center_x1) * sin_r1 +
                (r1->pos.y + r1->dim.y - center_y1) * cos_r1;
    float x3 = center_x1 +
                (r1->pos.x - center_x1) * cos_r1 -
                (r1->pos.y + r1->dim.y - center_y1) * sin_r1;
    float y3 = center_y1 +
                (r1->pos.x - center_x1) * sin_r1 +
                (r1->pos.y + r1->dim.y - center_y1) * cos_r1;


    float center_x2 = r2->pos.x + r2->dim.x * 0.5f;
    float center_y2 = r2->pos.y + r2->dim.y * 0.5f;
    float r2_angle = glm::radians(-r2->angle);
    float cos_r2 = cos(r2_angle);
    float sin_r2 = sin(r2_angle);

    float s0 = center_x2 +
                (r2->pos.x - center_x2) * cos_r2 -
                (r2->pos.y - center_y2) * sin_r2;
    float t0 = center_y2 +
                (r2->pos.x - center_x2) * sin_r2 +
                (r2->pos.y - center_y2) * cos_r2;
    float s1 = center_x2 +
                (r2->pos.x + r2->dim.x - center_x2) * cos_r2 -
                (r2->pos.y - center_y2) * sin_r2;
    float t1 = center_y2 +
                (r2->pos.x + r2->dim.x - center_x2) * sin_r2 +
                (r2->pos.y - center_y2) * cos_r2;
    float s2 = center_x2 +
                (r2->pos.x + r2->dim.x - center_x2) * cos_r2 -
                (r2->pos.y + r2->dim.y - center_y2) * sin_r2;
    float t2 = center_y2 +
                (r2->pos.x + r2->dim.x - center_x2) * sin_r2 +
                (r2->pos.y + r2->dim.y - center_y2) * cos_r2;
    float s3 = center_x2 +
                (r2->pos.x - center_x2) * cos_r2 -
                (r2->pos.y + r2->dim.y - center_y2) * sin_r2;
    float t3 = center_y2 +
                (r2->pos.x - center_x2) * sin_r2 +
                (r2->pos.y + r2->dim.y - center_y2) * cos_r2;


    /* std::cout << "---------------------\n" */
    /*         << "x0: " << (int)x0 */
    /*         << ", y0: " << (int)y0 */
    /*         << ", x1: " << (int)x1 */
    /*         << ", y1: " << (int)y1 */
    /*         << "\ns0: " << (int)s0 */
    /*         << ", t0: " << (int)t0 */
    /*         << ", s1: " << (int)s1 */
    /*         << ", t1: " << (int)t1 */
    /*         << std::endl; */

    // NOTE: line x0,y0:x1,y1 collision with all r2 lines
    if (lines_are_colliding(
            x0, y0,
            x1, y1,
            s0, t0,
            s1, t1,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y1,
            s1, t1,
            s2, t2,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y1,
            s2, t2,
            s3, t3,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y1,
            s3, t3,
            s0, t0,
            NULL, NULL
        )) return true;

    // NOTE: line x1,y1:x2,y2 collision with all r2 lines
    if (lines_are_colliding(
            x1, y1,
            x2, y2,
            s0, t0,
            s1, t1,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x1, y1,
            x2, y2,
            s1, t1,
            s2, t2,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x1, y1,
            x2, y2,
            s2, t2,
            s3, t3,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x1, y1,
            x2, y2,
            s3, t3,
            s0, t0,
            NULL, NULL
        )) return true;

    // NOTE: line x2,y2:x3,y3 collision with all r2 lines
    if (lines_are_colliding(
            x2, y2,
            x3, y3,
            s0, t0,
            s1, t1,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x2, y2,
            x3, y3,
            s1, t1,
            s2, t2,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x2, y2,
            x3, y3,
            s2, t2,
            s3, t3,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x0, y1,
            s3, t3,
            s0, t0,
            NULL, NULL
        )) return true;

    // NOTE: line x3,y3:x0,y0 collision with all r2 lines
    if (lines_are_colliding(
            x3, y3,
            x0, y0,
            s0, t0,
            s1, t1,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x3, y3,
            x0, y0,
            s1, t1,
            s2, t2,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x3, y3,
            x0, y0,
            s2, t2,
            s3, t3,
            NULL, NULL
        )) return true;
    if (lines_are_colliding(
            x3, y3,
            x0, y0,
            s3, t3,
            s0, t0,
            NULL, NULL
        )) return true;

    float cos1 = cos(-r1_angle);
    float sin1 = sin(-r1_angle);
    float cos2 = cos(-r2_angle);
    float sin2 = sin(-r2_angle);

    // NOTE: Check if a vertex of r1 is inside r2
    glm::vec2 p1;
    p1.x = center_x2 +
            (x0 - center_x2) * cos2 -
            (y0 - center_y2) * sin2;
    p1.y = center_y2 +
            (x0 - center_x2) * sin2 +
            (y0 - center_y2) * cos2;
    if (r2->pos.x < p1.x && p1.x < r2->pos.x+r2->dim.x &&
        r2->pos.y < p1.y && p1.y < r2->pos.y+r2->dim.y) {
        return true;
    }

    // NOTE: Check if a vertex of r2 is inside r1
    glm::vec2 p2;
    p2.x = center_x1 +
            (s0 - center_x1) * cos1 -
            (t0 - center_y1) * sin1;
    p2.y = center_y1 +
            (s0 - center_x1) * sin1 +
            (t0 - center_y1) * cos1;
    if (r1->pos.x < p2.x && p2.x < r1->pos.x+r1->dim.x &&
        r1->pos.y < p2.y && p2.y < r1->pos.y+r1->dim.y) {
        return true;
    }

    return false;
}

// NOTE: Returns true the lines intersect, otherwise false. In addition, if the lines 
//       intersect the intersection point may be stored in the floats x and y.
bool lines_are_colliding(float x0, float y0,
                           float x1, float y1,
                           float s0, float t0,
                           float s1, float t1,
                           float *x, float *y) {

    float s1_x, s1_y, s2_x, s2_y;

    s1_x = x1 - x0;
    s1_y = y1 - y0;
    s2_x = s1 - s0;
    s2_y = t1 - t0;

    float s, t;

    s = (-s1_y * (x0 - s0) + s1_x * (y0 - t0)) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (y0 - t0) - s2_y * (x0 - s0)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        // Collision detected
        if (x) *x = x0 + (t * s1_x);
        if (y) *y = y0 + (t * s1_y);

        return true;
    }

    return false; // No collision
}
