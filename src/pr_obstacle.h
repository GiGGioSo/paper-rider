#ifndef _PR_OBSTACLE_H_
#define _PR_OBSTACLE_H_

#include "pr_types.h"

#include "pr_polygon.h"
#include "pr_mathy.h"
#include "pr_rect.h"

vec4f
obstacle_get_color(PR_Obstacle *obs);

bool
obstacle_contains_point(PR_Obstacle *o, vec2f p);

bool
obstacle_collides_with_plane(PR_Obstacle *obs, PR_Plane *plane, vec2f *crash_pos);

bool
obstacle_collides_with_rider(PR_Obstacle *obs, PR_Rider *rid, vec2f *crash_pos);

void
obstacle_render(PR_Obstacle *obs);

void
obstacle_render_info(PR_Obstacle *obstacle, float tx, float ty);

#endif//_PR_OBSTACLE_H_
