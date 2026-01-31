#ifndef _PR_PORTAL_H_
#define _PR_PORTAL_H_

#include "pr_types.h"

#include "pr_polygon.h"
#include "pr_mathy.h"
#include "pr_rect.h"

// ###############
// ### SETTERS ###
// ###############
void
portal_set_option_buttons(PR_Button *buttons);

// ###############
// ### GETTERS ###
// ###############
vec4f
get_portal_color(PR_Portal *portal);

// ##############
// ### CREATE ###
// ##############
void
portal_init(PR_Portal *portal, vec2f pos, vec2f dim, float angle);

// ##################
// ### COLLISIONS ###
// ##################
bool
portal_contains_point(PR_Portal *portal, vec2f p);
bool
portal_collides_with_plane(PR_Portal *portal, PR_Plane *plane, vec2f *crash_pos);
bool
portal_collides_with_rider(PR_Portal *portal, PR_Rider *rid, vec2f *crash_pos);

// #################
// ### RENDERING ###
// #################
void
portal_render(PR_Portal *portal);

void
portal_render_info(PR_Portal *boost, float tx, float ty);

// ##############
// ### MODIFY ###
// ##############
void
portal_translate(PR_Portal *obs, vec2f move);

void
portal_rotate(PR_Portal *obs, float angle);

void
portal_set_size(PR_Portal *obs, vec2f size);
void
portal_resize(PR_Portal *obs, vec2f delta);
void
portal_scale(PR_Portal *obs, vec2f factor);

#endif//_PR_PORTAL_H_
