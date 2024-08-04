#include "pr_rect.h"

#include "../include/glm/glm.hpp"

#include <math.h>
#include <iostream>

// NOTE: Returns true the lines intersect, otherwise false. In addition, if the lines 
//       intersect the intersection point may be stored in the floats x and y.
bool lines_are_colliding(float x1, float y1,
                           float x2, float y2,
                           float x3, float y3,
                           float x4, float y4,
                           float *x, float *y) {

    float denominator = ((x1-x2) * (y3-y4)) - ((y1-y2) * (x3-x4));

    float t_numerator = ((x1-x3) * (y3-y4)) - ((y1-y3) * (x3-x4));

    float u_numerator = ((x1-x3) * (y1-y2)) - ((y1-y3) * (x1-x2));

    if (denominator < 0) {
        denominator = -denominator;
        t_numerator = -t_numerator;
        u_numerator = -u_numerator;
    }

    if (0 <= t_numerator && t_numerator <= denominator &&
        0 <= u_numerator && u_numerator <= denominator) {

        float t = t_numerator / denominator;

        if (x) {
            *x = x1 + t * (x2-x1);
        }
        if (y) {
            *y = y1 + t * (y2-y1);
        }

        return true;

    }


    return false; // No collision
}

bool rect_contains_point(const Rect rec, float px, float py, bool centered) {
    // NOTE: Check if rec contains the point
    float center_x, center_y;
    float x, y, w, h;

    if (centered) {
        center_x = rec.pos.x;
        center_y = rec.pos.y;
        x = rec.pos.x - rec.dim.x * 0.5f;
        y = rec.pos.y - rec.dim.y * 0.5f;
    } else {
        center_x = rec.pos.x + rec.dim.x * 0.5f;
        center_y = rec.pos.y + rec.dim.y * 0.5f;
        x = rec.pos.x;
        y = rec.pos.y;
    }

    w = rec.dim.x;
    h = rec.dim.y;
    float cosine = cos(glm::radians(rec.angle));
    float sine = sin(glm::radians(rec.angle));

    float rx = center_x +
            (px - center_x) * cosine -
            (py - center_y) * sine;
    float ry = center_y +
            (px - center_x) * sine +
            (py - center_y) * cosine;

    if (!rec.triangle) {
        if (x < rx && rx < x + w &&
            y < ry && ry < y + h) {
            return true;
        }
    } else {
        if (x < rx && rx < x + w &&
            y < ry && ry < y + h &&
            lines_are_colliding(
                rx, ry, x+w, y+h,
                x, y+h, x+w, y,
                NULL, NULL)) {
            return true;
        }
    }
    return false;
}

bool rect_are_colliding(const Rect r1, const Rect r2, float *cx, float *cy) {
    // NOTE: check if the objects are very distant,
    //       in that case don't check the collision
    if (glm::abs(r1.pos.x - r2.pos.x) >
            glm::abs(r1.dim.x) + glm::abs(r1.dim.y) +
            glm::abs(r2.dim.x) + glm::abs(r2.dim.y)
     || glm::abs(r1.pos.y - r2.pos.y) >
            glm::abs(r1.dim.x) + glm::abs(r1.dim.y) +
            glm::abs(r2.dim.x) + glm::abs(r2.dim.y)
    ) return false;

    float center_x1 = r1.pos.x + r1.dim.x * 0.5f;
    float center_y1 = r1.pos.y + r1.dim.y * 0.5f;
    float r1_angle = glm::radians(-r1.angle);
    float cos_r1 = cos(r1_angle);
    float sin_r1 = sin(r1_angle);

    // Top left
    float x0 = center_x1 +
                (r1.pos.x - center_x1) * cos_r1 -
                (r1.pos.y - center_y1) * sin_r1;
    float y0 = center_y1 +
                (r1.pos.x - center_x1) * sin_r1 +
                (r1.pos.y - center_y1) * cos_r1;
    // Top right
    float x1 = center_x1 +
                (r1.pos.x + r1.dim.x - center_x1) * cos_r1 -
                (r1.pos.y - center_y1) * sin_r1;
    float y1 = center_y1 +
                (r1.pos.x + r1.dim.x - center_x1) * sin_r1 +
                (r1.pos.y - center_y1) * cos_r1;
    // Bottom left
    float x2 = center_x1 +
                (r1.pos.x - center_x1) * cos_r1 -
                (r1.pos.y + r1.dim.y - center_y1) * sin_r1;
    float y2 = center_y1 +
                (r1.pos.x - center_x1) * sin_r1 +
                (r1.pos.y + r1.dim.y - center_y1) * cos_r1;
    // Bottom right
    float x3 = center_x1 +
                (r1.pos.x + r1.dim.x - center_x1) * cos_r1 -
                (r1.pos.y + r1.dim.y - center_y1) * sin_r1;
    float y3 = center_y1 +
                (r1.pos.x + r1.dim.x - center_x1) * sin_r1 +
                (r1.pos.y + r1.dim.y - center_y1) * cos_r1;

    //std::cout << "----------------------"
    //          << "\nx0: " << x0 << ", y0: " << y0
    //          << "\nx1: " << x1 << ", y1: " << y1
    //          << "\nx2: " << x2 << ", y2: " << y2
    //          << "\nx3: " << x3 << ", y3: " << y3
    //          << std::endl;


    float center_x2 = r2.pos.x + r2.dim.x * 0.5f;
    float center_y2 = r2.pos.y + r2.dim.y * 0.5f;
    float r2_angle = glm::radians(-r2.angle);
    float cos_r2 = cos(r2_angle);
    float sin_r2 = sin(r2_angle);

    // Top left
    float s0 = center_x2 +
                (r2.pos.x - center_x2) * cos_r2 -
                (r2.pos.y - center_y2) * sin_r2;
    float t0 = center_y2 +
                (r2.pos.x - center_x2) * sin_r2 +
                (r2.pos.y - center_y2) * cos_r2;
    // Top right
    float s1 = center_x2 +
                (r2.pos.x + r2.dim.x - center_x2) * cos_r2 -
                (r2.pos.y - center_y2) * sin_r2;
    float t1 = center_y2 +
                (r2.pos.x + r2.dim.x - center_x2) * sin_r2 +
                (r2.pos.y - center_y2) * cos_r2;
    // Bottom left
    float s2 = center_x2 +
                (r2.pos.x - center_x2) * cos_r2 -
                (r2.pos.y + r2.dim.y - center_y2) * sin_r2;
    float t2 = center_y2 +
                (r2.pos.x - center_x2) * sin_r2 +
                (r2.pos.y + r2.dim.y - center_y2) * cos_r2;
    // Bottom right
    float s3 = center_x2 +
                (r2.pos.x + r2.dim.x - center_x2) * cos_r2 -
                (r2.pos.y + r2.dim.y - center_y2) * sin_r2;
    float t3 = center_y2 +
                (r2.pos.x + r2.dim.x - center_x2) * sin_r2 +
                (r2.pos.y + r2.dim.y - center_y2) * cos_r2;


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
            cx, cy
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x1, y1,
            s0, t0,
            s2, t2,
            cx, cy
        )) return true;
    if (r2.triangle) {
        if (lines_are_colliding(
                x0, y0,
                x1, y1,
                s2, t2,
                s1, t1,
                cx, cy
            )) return true;
    } else {
        if (lines_are_colliding(
                x0, y0,
                x1, y1,
                s2, t2,
                s3, t3,
                cx, cy
            )) return true;
        if (lines_are_colliding(
                x0, y0,
                x1, y1,
                s3, t3,
                s1, t1,
                cx, cy
            )) return true;
    }

    // NOTE: line x0,y0:x2,y2 collision with all r2 lines
    if (lines_are_colliding(
            x0, y0,
            x2, y2,
            s0, t0,
            s1, t1,
            cx, cy
        )) return true;
    if (lines_are_colliding(
            x0, y0,
            x2, y2,
            s0, t0,
            s2, t2,
            cx, cy
        )) return true;
    if (r2.triangle) {
        if (lines_are_colliding(
                x0, y0,
                x2, y2,
                s2, t2,
                s1, t1,
                cx, cy
            )) return true;
    } else {
        if (lines_are_colliding(
                x0, y0,
                x2, y2,
                s2, t2,
                s3, t3,
                cx, cy
            )) return true;
        if (lines_are_colliding(
                x0, y0,
                x2, y2,
                s3, t3,
                s1, t1,
                cx, cy
            )) return true;
    }

    if (r1.triangle) {
        // NOTE: line x2,y2:x1,y1 collision with all r2 lines
        if (lines_are_colliding(
                x2, y2,
                x1, y1,
                s0, t0,
                s1, t1,
                cx, cy
            )) return true;
        if (lines_are_colliding(
                x2, y2,
                x1, y1,
                s0, t0,
                s2, t2,
                cx, cy
            )) return true;
        if (r2.triangle) {
            if (lines_are_colliding(
                    x2, y2,
                    x1, y1,
                    s2, t2,
                    s1, t1,
                    cx, cy
                )) return true;
        } else {
            if (lines_are_colliding(
                    x2, y2,
                    x1, y1,
                    s2, t2,
                    s3, t3,
                    cx, cy
                )) return true;
            if (lines_are_colliding(
                    x2, y2,
                    x1, y1,
                    s3, t3,
                    s1, t1,
                    cx, cy
                )) return true;
        }
    } else {
        // NOTE: line x2,y2:x3,y3 collision with all r2 lines
        if (lines_are_colliding(
                x2, y2,
                x3, y3,
                s0, t0,
                s1, t1,
                cx, cy
            )) return true;
        if (lines_are_colliding(
                x2, y2,
                x3, y3,
                s0, t0,
                s2, t2,
                cx, cy
            )) return true;
        if (r2.triangle) {
            if (lines_are_colliding(
                    x2, y2,
                    x3, y3,
                    s2, t2,
                    s1, t1,
                    cx, cy
                )) return true;
        } else {
            if (lines_are_colliding(
                    x2, y2,
                    x3, y3,
                    s2, t2,
                    s3, t3,
                    cx, cy
                )) return true;
            if (lines_are_colliding(
                    x2, y2,
                    x3, y3,
                    s3, t3,
                    s1, t1,
                    cx, cy
                )) return true;
        }

        // NOTE: line x3,y3:x1,y1 collision with all r2 lines
        if (lines_are_colliding(
                x3, y3,
                x1, y1,
                s0, t0,
                s1, t1,
                cx, cy
            )) return true;
        if (lines_are_colliding(
                x3, y3,
                x1, y1,
                s0, t0,
                s2, t2,
                cx, cy
            )) return true;
        if (r2.triangle) {
            if (lines_are_colliding(
                    x3, y3,
                    x1, y1,
                    s2, t2,
                    s1, t1,
                    cx, cy
                )) return true;
        } else {
            if (lines_are_colliding(
                    x3, y3,
                    x1, y1,
                    s2, t2,
                    s3, t3,
                    cx, cy
                )) return true;
            if (lines_are_colliding(
                    x3, y3,
                    x1, y1,
                    s3, t3,
                    s1, t1,
                    cx, cy
                )) return true;
        }
    }

    if (rect_contains_point(r2, x0, y0, false)) return true;

    if (rect_contains_point(r1, s0, t0, false)) return true;

    return false;
}

