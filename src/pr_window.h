#ifndef PR_WINDOW_H
#define PR_WINDOW_H

#include "pr_common.h"
#include "pr_globals.h"

void
display_mode_update(PR_WinInfo *win, PR_DisplayMode dm);

void
window_resolution_set(PR_WinInfo *win, PR_WindowResolution res);

PR_WindowResolution
window_resolution_prev(PR_WindowResolution res);

PR_WindowResolution
window_resolution_next(PR_WindowResolution res);

const char *
window_resolution_to_str(PR_WindowResolution res);

PR_WindowResolution
window_resolution_from_dim(int width, int height);

int
window_resolution_width(PR_WindowResolution res);

int
window_resolution_height(PR_WindowResolution res);

#endif//PR_WINDOW_H
