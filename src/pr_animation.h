#ifndef PR_ANIMATION_H
#define PR_ANIMATION_H

#include "pr_common.h"
#include "pr_globals.h"

void
animation_init(PR_Animation *a, PR_Texture tex, size_t start_x, size_t start_y, size_t dim_x, size_t dim_y, size_t step_x, size_t step_y, size_t frame_number, float frame_duration, bool loop);

void
animation_step(PR_Animation *a);

void
animation_queue_render(PR_Rect b, PR_Animation *a, bool inverse);

void
animation_reset(PR_Animation *a);

#endif//PR_ANIMATION_H
