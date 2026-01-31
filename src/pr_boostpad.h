#ifndef _PR_BOOSTPAD_H_
#define _PR_BOOSTPAD_H_

#include "pr_types.h"

#include "pr_polygon.h"
#include "pr_mathy.h"
#include "pr_rect.h"

// ###############
// ### SETTERS ###
// ###############
void
boostpad_set_option_buttons(PR_Button *buttons);

// ##############
// ### CREATE ###
// ##############
void
boostpad_init(PR_BoostPad *pad, vec2f pos, vec2f dim, float angle);

// ##################
// ### COLLISIONS ###
// ##################
bool
boostpad_contains_point(PR_BoostPad *pad, vec2f p);
bool
boostpad_collides_with_plane(PR_BoostPad *pad, PR_Plane *plane, vec2f *crash_pos);
bool
boostpad_collides_with_rider(PR_BoostPad *pad, PR_Rider *rid, vec2f *crash_pos);

// #################
// ### RENDERING ###
// #################
void
boostpad_render(PR_BoostPad *pad);

void
boostpad_render_info(PR_BoostPad *boost, float tx, float ty);

// ##############
// ### MODIFY ###
// ##############
void
boostpad_translate(PR_BoostPad *obs, vec2f move);

void
boostpad_rotate(PR_BoostPad *obs, float angle);

void
boostpad_set_size(PR_BoostPad *obs, vec2f size);
void
boostpad_resize(PR_BoostPad *obs, vec2f delta);
void
boostpad_scale(PR_BoostPad *obs, vec2f factor);

#endif//_PR_BOOSTPAD_H_
