#ifndef PP_GLOBALS_H
#define PP_GLOBALS_H

#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"

#include "pp_shaderer.h"
#include "pp_renderer.h"
#include "pp_input.h"
#include "pp_rect.h"

#define SCREEN_WIDTH_PROPORTION 320
#define SCREEN_HEIGHT_PROPORTION 240

#define ARRAY_LENGTH(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define UNUSED(expr) do { (void)(expr); } while (0)

struct PR {

    struct Plane {
        Rect body;

        Rect render_zone;

        // portal effect
        bool inverse;

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

        // portal effect
        bool inverse;

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

    glm::vec4 colors[4];

    enum ObstacleColorIndex {
        RED = 0,
        WHITE = 1,
        BLUE = 2,
        GRAY = 3,
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
    enum PortalType {
        INVERSE = 0,
        SHUFFLE_COLORS = 1,
    };
    struct Portal {
        Rect body;
        PortalType type;
        bool enable_effect;
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
    struct LevelButton {
        Rect body;
        glm::vec4 col;
        bool from_center;
        char text[99];

        char mapfile_path[99];
    };

    #define CAMPAIGN_LEVELS_NUMBER 2
    struct Menu {
        Camera camera;
        float camera_goal_position;

        bool showing_campaign_buttons;
        LevelButton show_campaign_button;
        LevelButton show_custom_button;

        LevelButton campaign_buttons[CAMPAIGN_LEVELS_NUMBER];

        size_t custom_buttons_number;
        LevelButton *custom_buttons;
        LevelButton *custom_edit_buttons;
    };
    Menu current_menu;

    enum ObjectType {
        PORTAL_TYPE = 0,
        BOOST_TYPE = 1,
        OBSTACLE_TYPE = 2,
    };

    struct Level {

        char file_path[99];

        Plane plane;
        Camera camera;

        Atmosphere air;
        Rider rider;

        float goal_line;
        glm::vec2 start_pos;

        ParticleSystem particle_systems[3];

        bool colors_shuffled;
        ObstacleColorIndex current_red;
        ObstacleColorIndex current_white;
        ObstacleColorIndex current_blue;
        ObstacleColorIndex current_gray;

        bool editing_available;
        bool editing_now;

        bool adding_now;

        void *selected;
        void *old_selected;
        ObjectType selected_type;
#define SELECTED_PORTAL_OPTIONS 2
#define SELECTED_BOOST_OPTIONS 2
#define SELECTED_OBSTACLE_OPTIONS 2
#define SELECTED_MAX_OPTIONS 2
        LevelButton selected_options_buttons[SELECTED_MAX_OPTIONS];

        size_t obstacles_number;
        Obstacle *obstacles;
        size_t boosts_number;
        BoostPad *boosts;
        size_t portals_number;
        Portal *portals;
    };
    Level current_level;

#define DEFAULT_FONT 0
#define OBJECT_INFO_FONT 1

#define DEFAULT_FONT_SIZE (64.f)
#define OBJECT_INFO_FONT_SIZE (24.f)

    struct RenderResources {
        glm::mat4 ortho_proj;
        Shader shaders[3];
        Font fonts[2];
        Texture global_sprite;
    };
    RenderResources rend_res;
    Renderer renderer;

    InputController input;

    enum GameCase {
        MENU = 0,
        LEVEL = 1,
    };
    struct GameState {
        float delta_time;
        GameCase current_case;
    };
    GameState state;

    struct WinInfo {
        uint32_t w;
        uint32_t h;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

};

extern PR* glob;

#endif
