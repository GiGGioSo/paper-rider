#include "pr_window.h"

void display_mode_update(PR::WinInfo *win, PR::DisplayMode dm) {
    std::cout << "New display mode: " << dm << std::endl;

    GLFWmonitor* main_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(main_monitor);

    if (dm == PR::FULLSCREEN) {
        glfwSetWindowMonitor(win->glfw_win, main_monitor, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR::BORDERLESS) {
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, false);
        glfwSetWindowMonitor(win->glfw_win, NULL, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR::WINDOWED) {
        int windowed_w = window_resolution_width(win->windowed_resolution);
        int windowed_h = window_resolution_height(win->windowed_resolution);
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, true);
        glfwSetWindowMonitor(win->glfw_win, NULL,
                             (mode->width - windowed_w) * 0.5f,
                             (mode->height - windowed_h) * 0.5f,
                             windowed_w, windowed_h,
                             GLFW_DONT_CARE);
    } else {
        std::cout << "[ERROR] Unknown window mode: " << dm << std::endl;
    }
}

void window_resolution_set(PR::WinInfo *win, PR::WindowResolution res) {
    if (win->display_mode != PR::WINDOWED || res == PR::R_NONE) return;

    win->windowed_resolution = res;

    glfwSetWindowSize(win->glfw_win, window_resolution_width(res),
                      window_resolution_height(res));
}
PR::WindowResolution window_resolution_prev(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return PR::R1280X960;
        case PR::R1280X960: return PR::R1200x900;
        case PR::R1200x900: return PR::R960x720;
        case PR::R960x720: return PR::R800x600;
        case PR::R800x600: return PR::R640x480;
        case PR::R640x480: return PR::R400x300;
        case PR::R400x300: return PR::R320x240;
        case PR::R320x240: return PR::R320x240;
        default:
            std::cout << "Unknown window resolution: " << res << std::endl;
            return PR::R_NONE;
    }
}
PR::WindowResolution window_resolution_next(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return PR::R1440x1080;
        case PR::R1280X960: return PR::R1440x1080;
        case PR::R1200x900: return PR::R1280X960;
        case PR::R960x720: return PR::R1200x900;
        case PR::R800x600: return PR::R960x720;
        case PR::R640x480: return PR::R800x600;
        case PR::R400x300: return PR::R640x480;
        case PR::R320x240: return PR::R400x300;
        default:
            std::cout << "Unknown window resolution: " << res << std::endl;
            return PR::R_NONE;
    }
}
const char *window_resolution_to_str(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return "1440x1080";
        case PR::R1280X960: return "1280x960";
        case PR::R1200x900: return "1200x900";
        case PR::R960x720: return "960x720";
        case PR::R800x600: return "800x600";
        case PR::R640x480: return "640x480";
        case PR::R400x300: return "400x300";
        case PR::R320x240: return "320x240";
        default: return "UKNOWN";
    }
}
PR::WindowResolution window_resolution_from_dim(int width, int height) {
    if (width == 1440 && height == 1080) {
        return PR::R1440x1080;
    } else if (width == 1280 && height == 960) {
        return PR::R1280X960;
    } else if (width == 1200 && height == 900) {
        return PR::R1200x900;
    } else if (width == 960 && height == 720) {
        return PR::R960x720;
    } else if (width == 800 && height == 600) {
        return PR::R800x600;
    } else if (width == 640 && height == 480) {
        return PR::R640x480;
    } else if (width == 400 && height == 300) {
        return PR::R400x300;
    } else if (width == 320 && height == 240) {
        return PR::R320x240;
    } else {
        return PR::R_NONE;
    }
}
int window_resolution_width(PR::WindowResolution res) {
    return (int)(res / 3);
}
int window_resolution_height(PR::WindowResolution res) {
    return (int)(res / 4);
}
