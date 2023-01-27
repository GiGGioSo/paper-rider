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

    if (r1->pos.x + r1->dim.x*2 < r2->pos.x
        || r1->pos.x > r2->pos.x + r2->dim.x*2
        || r1->pos.y + r1->dim.y*2 < r2->pos.y
        || r1->pos.y > r2->pos.y + r2->dim.y*2
    ) return false;

    float center_x1 = r1->pos.x + r1->dim.x * 0.5f;
    float center_y1 = r1->pos.y + r1->dim.y * 0.5f;
    float r1_angle = glm::radians(-r1->angle);

    float x0 = center_x1 +
                (r1->pos.x - center_x1) * cos(r1_angle) -
                (r1->pos.y - center_y1) * sin(r1_angle);
    float y0 = center_y1 +
                (r1->pos.x - center_x1) * sin(r1_angle) +
                (r1->pos.y - center_y1) * cos(r1_angle);
    float x1 = center_x1 +
                (r1->pos.x + r1->dim.x - center_x1) * cos(r1_angle) -
                (r1->pos.y - center_y1) * sin(r1_angle);
    float y1 = center_y1 +
                (r1->pos.x + r1->dim.x - center_x1) * sin(r1_angle) +
                (r1->pos.y - center_y1) * cos(r1_angle);
    float x2 = center_x1 +
                (r1->pos.x + r1->dim.x - center_x1) * cos(r1_angle) -
                (r1->pos.y + r1->dim.y - center_y1) * sin(r1_angle);
    float y2 = center_y1 +
                (r1->pos.x + r1->dim.x - center_x1) * sin(r1_angle) +
                (r1->pos.y + r1->dim.y - center_y1) * cos(r1_angle);
    float x3 = center_x1 +
                (r1->pos.x - center_x1) * cos(r1_angle) -
                (r1->pos.y + r1->dim.y - center_y1) * sin(r1_angle);
    float y3 = center_y1 +
                (r1->pos.x - center_x1) * sin(r1_angle) +
                (r1->pos.y + r1->dim.y - center_y1) * cos(r1_angle);


    float center_x2 = r2->pos.x + r2->dim.x * 0.5f;
    float center_y2 = r2->pos.y + r2->dim.y * 0.5f;
    float r2_angle = glm::radians(-r2->angle);

    float s0 = center_x2 +
                (r2->pos.x - center_x2) * cos(r2_angle) -
                (r2->pos.y - center_y2) * sin(r2_angle);
    float t0 = center_y2 +
                (r2->pos.x - center_x2) * sin(r2_angle) +
                (r2->pos.y - center_y2) * cos(r2_angle);
    float s1 = center_x2 +
                (r2->pos.x + r2->dim.x - center_x2) * cos(r2_angle) -
                (r2->pos.y - center_y2) * sin(r2_angle);
    float t1 = center_y2 +
                (r2->pos.x + r2->dim.x - center_x2) * sin(r2_angle) +
                (r2->pos.y - center_y2) * cos(r2_angle);
    float s2 = center_x2 +
                (r2->pos.x + r2->dim.x - center_x2) * cos(r2_angle) -
                (r2->pos.y + r2->dim.y - center_y2) * sin(r2_angle);
    float t2 = center_y2 +
                (r2->pos.x + r2->dim.x - center_x2) * sin(r2_angle) +
                (r2->pos.y + r2->dim.y - center_y2) * cos(r2_angle);
    float s3 = center_x2 +
                (r2->pos.x - center_x2) * cos(r2_angle) -
                (r2->pos.y + r2->dim.y - center_y2) * sin(r2_angle);
    float t3 = center_y2 +
                (r2->pos.x - center_x2) * sin(r2_angle) +
                (r2->pos.y + r2->dim.y - center_y2) * cos(r2_angle);

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

#if 0 // NOTE: I'm using another method, that's able to handle edge cases
bool lines_are_colliding(float x0, float y0,
                         float x1, float y1,
                         float s0, float t0,
                         float s1, float t1) {

    // NOTE: The m (inclination) of the lines created by the points
    float m1 = (y1-y0) / (x1-x0);
    float m2 = (t1-t0) / (s1-s0);

    /* std::cout << "m1: " << m1 << ", m2: " << m2 << std::endl; */

    // NOTE: The q (offset at x=0) of the lines created by the points
    float q1 = y0 - x0 * m1;
    float q2 = t0 - s0 * m2;

    // NOTE: Coordinates of the resulting point
    float x = (q2-q1) / (m1-m2);
    float y = m1 * x + q1;

    /* std::cout << "Int.x: " << x */
    /*             << ", Int.y: " << y << std::endl; */
        

    // NOTE: The resulting point must be inside both lines
    bool result = ((x0 <= x && x <= x1) || (x1 <= x && x <= x0)) &&
                    ((y0 <= y && y <= y1) || (y1 <= y && y <= y0)) &&
                  ((s0 <= x && x <= s1) || (s1 <= x && x <= s0)) &&
                    ((t0 <= y && y <= t1) || (t1 <= y && y <= t0));

    return result;
}
#endif
