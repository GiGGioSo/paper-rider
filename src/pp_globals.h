#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "pp_shaderer.h"
#include "pp_quad_renderer.h"

struct PP {

    struct WinInfo {
        unsigned int w;
        unsigned int h;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

    struct Rendering {
        glm::mat4 ortho_proj;
        Shader* shaders;
        RendererQuad quad_renderer;
    };
    Rendering rend;

    // TODO: Maybe a "atmosphere" struct, with all the information needed for the simulation
    struct Atmosphere {
        float density;
    };
    Atmosphere air;

    struct Plane {
        glm::vec2 pos;
        glm::vec2 dim; // width and height

        glm::vec2 vel;
        glm::vec2 acc;

        float mass;
        float alar_surface;

        float angle;
    };
    Plane plane;

};

extern PP* glob;
