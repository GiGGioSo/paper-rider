#ifndef PP_GLOBALS_H
#define PP_GLOBALS_H

#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "pp_shaderer.h"
#include "pp_texturer.h"
#include "pp_quad_renderer.h"
#include "pp_input.h"
#include "pp_rect.h"

#define ARRAY_LENGTH(arr) (int)(sizeof(arr) / sizeof(arr[0]))

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
        RendererQuad quad_renderer;

        Shader *shaders;

        Texture global_sprite;
    };
    Rendering rend;

    struct Atmosphere {
        float density;
    };
    Atmosphere air;

    struct Plane {
        Rect body;

        glm::vec2 vel;
        glm::vec2 acc;

        float mass;
        float alar_surface;
    };
    Plane plane;

    struct Rider {
        Rect body;

        glm::vec2 vel;
        glm::vec2 acc;

        float mass;

        bool attached;
        float jump_time_elapsed;
    };
    Rider rider;

    Rect obstacles[64];

    struct Camera {
        glm::vec2 pos;

        float speed_multiplier;
    };
    Camera cam;

    InputController input;

};

extern PP* glob;

#endif
