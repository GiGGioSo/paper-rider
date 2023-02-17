#ifndef PP_GLOBALS_H
#define PP_GLOBALS_H

#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "pp_shaderer.h"
#include "pp_texturer.h"
#include "pp_quad_renderer.h"
#include "pp_text_renderer.h"
#include "pp_input.h"
#include "pp_rect.h"

#define ARRAY_LENGTH(arr) ((int)(sizeof(arr) / sizeof(arr[0])))

struct PR {

    struct Plane {
        Rect body;

        Rect render_zone;

        bool crashed;
        glm::vec2 crash_position;

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

        bool crashed;
        glm::vec2 crash_position;

        float base_velocity;
        float input_velocity;

        float input_max_accelleration;
        float air_friction_acc;

        float mass;

        bool attached;
        float jump_time_elapsed;

        bool second_jump;
    };
    struct Obstacle {
        Rect body;
        bool collide_plane;
        bool collide_rider;
    };
    struct BoostPad {
        Rect body;
        float boost_angle;
        float boost_power;
    };
    struct Camera {
        glm::vec2 pos;

        float speed_multiplier;
    };
    struct Atmosphere {
        float density;
    };
    struct Particle {
        Rect body;
        glm::vec2 vel;
        glm::vec4 color;
    };
    struct ParticleSystem {
        Particle *particles;
        size_t particles_number;

        size_t current_particle;

        bool active;

        void (*create_particle)(PR::ParticleSystem *, PR::Particle *);
        void (*update_particle)(PR::ParticleSystem *, PR::Particle *);

        float time_between_particles;
        float time_elapsed;
    };

    struct Level {
        Plane plane;
        Camera camera;

        Atmosphere air;
        Rider rider;

        ParticleSystem particle_systems[3];

        size_t obstacles_number;
        Obstacle *obstacles;
        size_t boosts_number;
        BoostPad *boosts;
        // TODO: Coins?
    };
    Level current_level;

    struct Rendering {
        glm::mat4 ortho_proj;
        RendererQuad quad_renderer;

        Shader shaders[3];

        Texture global_sprite;
    };
    Rendering rend;

    TextRenderer text_rend;

    InputController input;

    enum GameCase {
        MENU = 0,
        LEVEL1 = 1,
        LEVEL2 = 2,
    };
    struct GameState {
        float delta_time;
        GameCase current_case;
    };
    GameState state;

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
