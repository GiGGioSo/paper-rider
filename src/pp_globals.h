#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "shaderer.h"
#include "quad_renderer.h"

struct PP {

    struct WinInfo {
        unsigned int w;
        unsigned int h;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

    glm::mat4 ortho_proj;

    Shader* shaders;

    RendererQuad quad_renderer;

    struct Plane {
        float x;
        float y;
        float w;
        float h;
        float rotation;
    };
    Plane plane;

};

extern PP* glob;
