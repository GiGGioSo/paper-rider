#include "../include/glfw3.h"

struct PP {

    struct WinInfo {
        unsigned int w;
        unsigned int h;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

};

extern PP* glob;
