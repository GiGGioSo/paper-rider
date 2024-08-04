#ifndef PR_ANIMATION_H
#define PR_ANIMATION_H

#include "pr_common.h"
#include "pr_globals.h"

void
animation_init(PR::Animation *a, Texture tex, size_t start_x, size_t start_y, size_t dim_x, size_t dim_y, size_t step_x, size_t step_y, size_t frame_number, float frame_duration, bool loop);

void
animation_step(PR::Animation *a);

void
animation_queue_render(Rect b, PR::Animation *a, bool inverse);

void
animation_reset(PR::Animation *a);

#endif//PR_ANIMATION_H
