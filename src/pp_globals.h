#ifndef PP_GLOBALS_H
#define PP_GLOBALS_H

#include "../include/glfw3.h"
#include "../include/glm/mat4x4.hpp"
#include "../include/miniaudio.h"

#include "pp_shaderer.h"
#include "pp_renderer.h"
#include "pp_input.h"
#include "pp_rect.h"

#define GAME_WIDTH 1440
#define GAME_HEIGHT 1080

#define ARRAY_LENGTH(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define UNUSED(expr) do { (void)(expr); } while (0)

struct PR {
    enum DisplayMode {
        FULLSCREEN = 0,
        BORDERLESS = 1,
        WINDOWED = 2,
    };
    enum WindowResolution {
        R1440x1080 = 4320,
        R1280X960 = 3840,
        R1200x900 = 3600,
        R960x720 = 2880,
        R800x600 = 2400,
        R640x480 = 1920,
        R400x300 = 1200,
        R320x240 = 960,
        R_NONE = 0,
    };

    struct Animation {
        bool active;
        bool finished;
        bool loop;
        float frame_duration;
        float frame_elapsed;
        size_t current;
        size_t frame_number;
        size_t frame_stop;
        TexCoords *tc;
    };

    struct Plane {
        Rect body;

        Rect render_zone;
        Animation anim;

        // portal effect
        bool inverse;

        bool crashed;
        glm::vec2 crash_position;

        glm::vec2 vel;
        glm::vec2 acc;

        enum AnimationState {
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
        float attach_time_elapsed;

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
        bool active;
    };
    struct ParticleSystem {
        Particle *particles;
        size_t particles_number;

        size_t current_particle;

        bool frozen;
        bool active;
        bool all_inactive;

        void (*create_particle)(PR::ParticleSystem *, PR::Particle *);
        void (*update_particle)(PR::ParticleSystem *, PR::Particle *);
        void (*draw_particle)(PR::ParticleSystem *, PR::Particle *);

        float time_between_particles;
        float time_elapsed;
    };
    struct ParallaxPiece {
        float base_pos_x;
        Rect body;
    };
    struct Parallax {
        float reference_point;
        float follow_coeff;
        ParallaxPiece pieces[3];
        TexCoords tex_coords;
    };
    struct Button {
        bool from_center;
        Rect body;
        glm::vec4 col;
        char text[256];
    };
    struct LevelButton {
        Button button;

        char mapfile_path[99];
        bool is_new_level;
    };
    struct CustomLevelButton {
        Button button;
        Button edit;
        Button del;
        char mapfile_path[99];
        bool is_new_level;
    };
    struct CustomLevelButtons {
        CustomLevelButton *items;
        size_t count;
        size_t capacity;
    };
    struct MenuCamera {
        glm::vec2 pos;
        float speed_multiplier;
        float goal_position;
        bool follow_selection;
    };

    enum StartMenuSelection {
        START_BUTTON_PLAY,
        START_BUTTON_OPTIONS,
        START_BUTTON_QUIT
    };

    struct StartMenu {
        StartMenuSelection selection;

        Button play;
        Button options;
        Button quit;
    };
    StartMenu current_start_menu;

    enum OptionsMenuSelection {
        OPTION_NONE,
        OPTION_MASTER_VOLUME,
        OPTION_SFX_VOLUME,
        OPTION_MUSIC_VOLUME,
        OPTION_DISPLAY_MODE,
        OPTION_RESOLUTION,
    };
    struct OptionSlider {
        Rect background;
        char label[256];
        float value;
        bool mouse_hooked;
        char value_text[32];
        Rect selection;
    };
    struct OptionsMenu {
        MenuCamera camera;

        Button to_start_menu;

        OptionsMenuSelection current_selection;

        bool showing_general_pane;
        Button to_general_pane;
        Button to_controls_pane;
        // General pane options
        OptionSlider master_volume;
        OptionSlider sfx_volume;
        OptionSlider music_volume;
        DisplayMode display_mode_selection;
        Button display_mode_fullscreen;
        Button display_mode_borderless;
        Button display_mode_windowed;
        WindowResolution resolution_selection;
        Button resolution_up;
        Button resolution_down;
        // TODO: Keybinding options
    };
    OptionsMenu current_options_menu;

    enum DeleteButtonChoice {
        BUTTON_YES,
        BUTTON_NO,
    };
    #define CAMPAIGN_LEVELS_NUMBER 2
    struct PlayMenu {
        MenuCamera camera;

        bool showing_campaign_buttons;
        Button show_campaign_button;
        Button show_custom_button;

        int selected_campaign_button;
        LevelButton campaign_buttons[CAMPAIGN_LEVELS_NUMBER];

        int selected_custom_button;
        CustomLevelButtons custom_buttons;
        
        Button add_custom_button;

        Button to_start_menu;

        DeleteButtonChoice delete_selection;
        bool deleting_level;
        Button delete_yes;
        Button delete_no;
        size_t deleting_index;
        Rect deleting_frame;
    };
    PlayMenu current_play_menu;

    enum ObjectType {
        PORTAL_TYPE = 0,
        BOOST_TYPE = 1,
        OBSTACLE_TYPE = 2,
        GOAL_LINE_TYPE = 3,
        P_START_POS_TYPE = 4,
    };

    struct Obstacles {
        Obstacle *items;
        size_t count;
        size_t capacity;
    };
    struct BoostPads {
        BoostPad *items;
        size_t count;
        size_t capacity;
    };
    struct Portals {
        Portal *items;
        size_t count;
        size_t capacity;
    };

    enum GameMenuChoice {
        BUTTON_RESUME,
        BUTTON_RESTART,
        BUTTON_QUIT,
    };

    struct Level {
        char file_path[99];
        char name[99];
        bool is_new;

        Plane plane;
        Camera camera;

        Atmosphere air;
        Rider rider;

        Rect goal_line;
        Rect start_pos;
        glm::vec2 start_vel;

        ParticleSystem particle_systems[3];
        Parallax parallaxs[3];

        bool colors_shuffled;
        ObstacleColorIndex current_red;
        ObstacleColorIndex current_white;
        ObstacleColorIndex current_blue;
        ObstacleColorIndex current_gray;

        bool editing_available;
        bool editing_now;

        bool adding_now;

        GameMenuChoice gamemenu_selected;
        bool pause_now;
        bool game_over;
        bool game_won;

        float finish_time;

        float text_wave_time;

        void *selected;
        void *old_selected;
        ObjectType selected_type;
#define SELECTED_PORTAL_OPTIONS 4
#define SELECTED_BOOST_OPTIONS 6
#define SELECTED_OBSTACLE_OPTIONS 6
#define SELECTED_START_POS_OPTIONS 3
#define SELECTED_MAX_OPTIONS 6
        Button selected_options_buttons[SELECTED_MAX_OPTIONS];

        Obstacles obstacles;
        BoostPads boosts;
        Portals portals;
    };
    Level current_level;

#define DEFAULT_FONT 0
#define OBJECT_INFO_FONT 1

#define DEFAULT_FONT_SIZE (64.f)
#define OBJECT_INFO_FONT_SIZE (24.f)

    struct Sound {
        float master_volume;
        float sfx_volume;
        float music_volume;

        ma_engine engine;

        // ### Sound groups (to control volume levels) ###
        ma_sound_group music_group;
        ma_sound_group sfx_group;

        // ### Sounds in the music group ###
        ma_sound menu_music;
        ma_sound playing_music;
        ma_sound gameover_music;

        // ### Sounds in the sfx group ###
        // Menu
        ma_sound change_selection;
        ma_sound click_selected;
        ma_sound change_pane;
        ma_sound delete_level;
        ma_sound to_start_menu;
        // Game
        ma_sound rider_detach;
        ma_sound rider_double_jump;
        ma_sound plane_crash;
        ma_sound rider_crash;
    };
    Sound sound;

    struct RenderResources {
        glm::mat4 ortho_proj;
        Shader shaders[4];
        Font fonts[2];
        Texture global_sprite;
    };
    RenderResources rend_res;
    Renderer renderer;

    InputController input;

    enum GameCase {
        START_MENU = 0,
        PLAY_MENU = 1,
        OPTIONS_MENU = 2,
        LEVEL = 3,
    };
    struct GameState {
        float delta_time;
        GameCase current_case;
    };
    GameState state;

    struct WinInfo {
        int vertical_bar;
        int horizontal_bar;
        int width;
        int height;
        WindowResolution windowed_resolution;
        DisplayMode display_mode;
        const char* title;
        GLFWwindow* glfw_win;
    };
    WinInfo window;

};

extern PR* glob;

#endif
