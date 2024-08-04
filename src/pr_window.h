#ifndef PR_WINDOW_H
#define PR_WINDOW_H

#include "pr_common.h"
#include "pr_globals.h"

void
display_mode_update(PR::WinInfo *win, PR::DisplayMode dm);

void
window_resolution_set(PR::WinInfo *win, PR::WindowResolution res);

PR::WindowResolution
window_resolution_prev(PR::WindowResolution res);

PR::WindowResolution
window_resolution_next(PR::WindowResolution res);

const char *
window_resolution_to_str(PR::WindowResolution res);

PR::WindowResolution
window_resolution_from_dim(int width, int height);

int
window_resolution_width(PR::WindowResolution res);

int
window_resolution_height(PR::WindowResolution res);

#endif//PR_WINDOW_H
