#ifndef PP_GLOBALS_H
#define PP_GLOBALS_H

#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "pp_shaderer.h"
#include "pp_texturer.h"
#include "pp_quad_renderer.h"
#include "pp_input.h"
#include "pp_rect.h"

#define ARRAY_LENGTH(arr) ((int)(sizeof(arr) / sizeof(arr[0])))

struct PR {

    struct Plane {
        Rect body;

        Rect render_zone;

        glm::vec2 vel;
        glm::vec2 acc;

        float mass;
        float alar_surface;
    };
    Plane plane;

    struct Rendering {
        glm::mat4 ortho_proj;
        RendererQuad quad_renderer;

        Shader shaders[2];

        Texture global_sprite;
    };
    Rendering rend;

    struct Camera {
        glm::vec2 pos;

        float speed_multiplier;
    };
    Camera cam;

    struct Atmosphere {
        float density;
    };
    Atmosphere air;

    struct Rider {
        Rect body;

        Rect render_zone;

        glm::vec2 vel;

        float base_velocity;
        float input_velocity;

        float input_max_accelleration;
        float air_friction_acc;

        float mass;

        bool attached;
        float jump_time_elapsed;
    };
    Rider rider;

    Rect obstacles[64];

    InputController input;

    struct WinInfo {
        unsigned int w;
        unsigned int h;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

};

extern PR* glob;

#endif
