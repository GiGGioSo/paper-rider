#ifndef PR_PARALLAX_H
#define PR_PARALLAX_H

#include "pr_common.h"
#include "pr_globals.h"

void
parallax_init(PR::Parallax *px, float fc, TexCoords tc, float p_start_x, float p_start_y, float p_w, float p_h);

void
parallax_update_n_queue_render(PR::Parallax *px, float current_x);

#endif//PR_PARALLAX_H
