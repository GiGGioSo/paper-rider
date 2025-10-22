#include "pr_window.h"

void display_mode_update(PR_WinInfo *win, PR_DisplayMode dm) {
    printf("New display mode: %d\n", dm);

    GLFWmonitor* main_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(main_monitor);

    if (dm == PR_FULLSCREEN) {
        glfwSetWindowMonitor(win->glfw_win, main_monitor, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR_BORDERLESS) {
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, false);
        glfwSetWindowMonitor(win->glfw_win, NULL, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR_WINDOWED) {
        int windowed_w = window_resolution_width(win->windowed_resolution);
        int windowed_h = window_resolution_height(win->windowed_resolution);
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, true);
        glfwSetWindowMonitor(win->glfw_win, NULL,
                             (mode->width - windowed_w) * 0.5f,
                             (mode->height - windowed_h) * 0.5f,
                             windowed_w, windowed_h,
                             GLFW_DONT_CARE);
    } else {
        printf("[ERROR] Unknown window mode: %d\n", dm);
    }
}

void window_resolution_set(PR_WinInfo *win, PR_WindowResolution res) {
    if (win->display_mode != PR_WINDOWED || res == PR_R_NONE) return;

    win->windowed_resolution = res;

    glfwSetWindowSize(win->glfw_win, window_resolution_width(res),
                      window_resolution_height(res));
}
PR_WindowResolution window_resolution_prev(PR_WindowResolution res) {
    switch(res) {
        case PR_R1440x1080: return PR_R1280X960;
        case PR_R1280X960: return PR_R1200x900;
        case PR_R1200x900: return PR_R960x720;
        case PR_R960x720: return PR_R800x600;
        case PR_R800x600: return PR_R640x480;
        case PR_R640x480: return PR_R400x300;
        case PR_R400x300: return PR_R320x240;
        case PR_R320x240: return PR_R320x240;
        default:
            printf("Unknown window resolution: %d\n", res);
            return PR_R_NONE;
    }
}
PR_WindowResolution window_resolution_next(PR_WindowResolution res) {
    switch(res) {
        case PR_R1440x1080: return PR_R1440x1080;
        case PR_R1280X960: return PR_R1440x1080;
        case PR_R1200x900: return PR_R1280X960;
        case PR_R960x720: return PR_R1200x900;
        case PR_R800x600: return PR_R960x720;
        case PR_R640x480: return PR_R800x600;
        case PR_R400x300: return PR_R640x480;
        case PR_R320x240: return PR_R400x300;
        default:
            printf("Unknown window resolution: %d\n", res);
            return PR_R_NONE;
    }
}
const char *window_resolution_to_str(PR_WindowResolution res) {
    switch(res) {
        case PR_R1440x1080: return "1440x1080";
        case PR_R1280X960: return "1280x960";
        case PR_R1200x900: return "1200x900";
        case PR_R960x720: return "960x720";
        case PR_R800x600: return "800x600";
        case PR_R640x480: return "640x480";
        case PR_R400x300: return "400x300";
        case PR_R320x240: return "320x240";
        default: return "UKNOWN";
    }
}
PR_WindowResolution window_resolution_from_dim(int width, int height) {
    if (width == 1440 && height == 1080) {
        return PR_R1440x1080;
    } else if (width == 1280 && height == 960) {
        return PR_R1280X960;
    } else if (width == 1200 && height == 900) {
        return PR_R1200x900;
    } else if (width == 960 && height == 720) {
        return PR_R960x720;
    } else if (width == 800 && height == 600) {
        return PR_R800x600;
    } else if (width == 640 && height == 480) {
        return PR_R640x480;
    } else if (width == 400 && height == 300) {
        return PR_R400x300;
    } else if (width == 320 && height == 240) {
        return PR_R320x240;
    } else {
        return PR_R_NONE;
    }
}
int window_resolution_width(PR_WindowResolution res) {
    return (int)(res / 3);
}
int window_resolution_height(PR_WindowResolution res) {
    return (int)(res / 4);
}
