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

        enum animation_state {
            IDLE_ACC = 0,
            UPWARDS_ACC = 1,
            DOWNWARDS_ACC = 2
        } current_animation;
        float animation_countdown;

        float mass;
        float alar_surface;
    };
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
    struct Camera {
        glm::vec2 pos;

        float speed_multiplier;
    };
    struct Atmosphere {
        float density;
    };

    struct Level {
        Plane plane;
        Camera camera;

        Atmosphere air;
        Rider rider;

        size_t obstacles_number;
        Rect *obstacles;
        // TODO: Boost pads
        // TODO: Coins?
    };
    Level current_level;

    struct Rendering {
        glm::mat4 ortho_proj;
        RendererQuad quad_renderer;

        Shader shaders[2];

        Texture global_sprite;
    };
    Rendering rend;

    InputController input;

    enum game_state {
        MENU = 0,
        LEVEL1 = 1,
        LEVEL2 = 2,
    };
    game_state current_state;

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
