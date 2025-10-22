#ifndef PR_GLOBALS_H
#define PR_GLOBALS_H

#include <stdbool.h>

#include "glfw3.h"
#include "miniaudio.h"

#include "pr_common.h"
#include "pr_shaderer.h"
#include "pr_renderer.h"
#include "pr_input.h"
#include "pr_rect.h"
#include "pr_mathy.h"

#define GAME_WIDTH 1440
#define GAME_HEIGHT 1080

#define CAMPAIGN_LEVELS_NUMBER 2

// how many options appear when the object is selected
#define SELECTED_PORTAL_OPTIONS 4
#define SELECTED_BOOST_OPTIONS 6
#define SELECTED_OBSTACLE_OPTIONS 6
#define SELECTED_START_POS_OPTIONS 3
#define SELECTED_MAX_OPTIONS 6

// Font configuration
#define DEFAULT_FONT 0
#define OBJECT_INFO_FONT 1
#define ACTION_NAME_FONT 2
//   font sizes
#define DEFAULT_FONT_SIZE (64.f)
#define OBJECT_INFO_FONT_SIZE (24.f)
#define ACTION_NAME_FONT_SIZE (40.f)

typedef enum PR_DisplayMode {
    PR_FULLSCREEN = 0,
    PR_BORDERLESS = 1,
    PR_WINDOWED = 2,
} PR_DisplayMode;

typedef enum PR_WindowResolution {
    PR_R1440x1080 = 4320,
    PR_R1280X960 = 3840,
    PR_R1200x900 = 3600,
    PR_R960x720 = 2880,
    PR_R800x600 = 2400,
    PR_R640x480 = 1920,
    PR_R400x300 = 1200,
    PR_R320x240 = 960,
    PR_R_NONE = 0,
} PR_WindowResolution;

typedef struct PR_Animation {
    bool active;
    bool finished;
    bool loop;
    float frame_duration;
    float frame_elapsed;
    size_t current;
    size_t frame_number;
    size_t frame_stop;
    PR_TexCoords *tc;
} PR_Animation;

typedef enum PR_PlaneAnimationState {
    PR_PLANE_IDLE_ACC = 0,
    PR_PLANE_UPWARDS_ACC = 1,
    PR_PLANE_DOWNWARDS_ACC = 2
} PR_PlaneAnimationState;

typedef struct PR_Plane {
    PR_Rect body;

    PR_Rect render_zone;
    PR_Animation anim;

    // portal effect
    bool inverse;

    bool crashed;
    vec2f crash_position;

    vec2f vel;
    vec2f acc;

    PR_PlaneAnimationState current_animation;
    float animation_countdown;

    float mass;
    float alar_surface;
} PR_Plane;

typedef struct PR_Rider {
    PR_Rect body;

    PR_Rect render_zone;

    vec2f vel;

    // portal effect
    bool inverse;

    bool crashed;
    vec2f crash_position;

    float base_velocity;
    float input_velocity;

    float input_max_accelleration;
    float air_friction_acc;

    float mass;

    bool attached;
    float jump_time_elapsed;
    float attach_time_elapsed;

    bool second_jump;
} PR_Rider;

typedef enum PR_ObstacleColorIndex {
    PR_RED = 0,
    PR_WHITE = 1,
    PR_BLUE = 2,
    PR_GRAY = 3,
} PR_ObstacleColorIndex;

typedef struct PR_Obstacle {
    PR_Rect body;
    bool collide_plane;
    bool collide_rider;
} PR_Obstacle;
typedef struct PR_BoostPad {
    PR_Rect body;
    float boost_angle;
    float boost_power;
} PR_BoostPad;
typedef enum PR_PortalType {
    PR_INVERSE = 0,
    PR_SHUFFLE_COLORS = 1,
} PR_PortalType;
typedef struct PR_Portal {
    PR_Rect body;
    PR_PortalType type;
    bool enable_effect;
} PR_Portal;

typedef struct PR_Camera {
    vec2f pos;
    float speed_multiplier;
} PR_Camera;

typedef struct PR_Atmosphere {
    float density;
} PR_Atmosphere;

typedef struct PR_Particle {
    PR_Rect body;
    vec2f vel;
    vec4f color;
    bool active;
} PR_Particle;
typedef struct PR_ParticleSystem {
    PR_Particle *particles;
    size_t particles_number;

    size_t current_particle;

    bool frozen;
    bool active;
    bool all_inactive;

    void (*create_particle)(struct PR_ParticleSystem *, PR_Particle *);
    void (*update_particle)(struct PR_ParticleSystem *, PR_Particle *);
    void (*draw_particle)(struct PR_ParticleSystem *, PR_Particle *);

    float time_between_particles;
    float time_elapsed;
} PR_ParticleSystem;

typedef struct PR_ParallaxPiece {
    float base_pos_x;
    PR_Rect body;
} PR_ParallaxPiece;
typedef struct PR_Parallax {
    float reference_point;
    float follow_coeff;
    PR_ParallaxPiece pieces[3];
    PR_TexCoords tex_coords;
} PR_Parallax;

typedef struct PR_Button {
    bool from_center;
    PR_Rect body;
    vec4f col;
    char text[256];
} PR_Button;
typedef struct PR_LevelButton {
    PR_Button button;

    char mapfile_path[99];
    bool is_new_level;
} PR_LevelButton;
typedef struct PR_CustomLevelButton {
    PR_Button button;
    PR_Button edit;
    PR_Button del;
    char mapfile_path[99];
    bool is_new_level;
} PR_CustomLevelButton;
typedef struct PR_CustomLevelButtons {
    PR_CustomLevelButton *items;
    size_t count;
    size_t capacity;
} PR_CustomLevelButtons;

typedef struct PR_MenuCamera {
    vec2f pos;
    float speed_multiplier;
    float goal_position;
    bool follow_selection;
} PR_MenuCamera;

typedef enum PR_StartMenuSelection {
    PR_START_BUTTON_PLAY,
    PR_START_BUTTON_OPTIONS,
    PR_START_BUTTON_QUIT
} PR_StartMenuSelection;

typedef struct PR_StartMenu {
    PR_StartMenuSelection selection;

    PR_Button play;
    PR_Button options;
    PR_Button quit;
} PR_StartMenu;

typedef enum PR_OptionsMenuSelection {
    PR_OPTION_NONE,
    PR_OPTION_MASTER_VOLUME,
    PR_OPTION_SFX_VOLUME,
    PR_OPTION_MUSIC_VOLUME,
    PR_OPTION_DISPLAY_MODE,
    PR_OPTION_RESOLUTION,
} PR_OptionsMenuSelection;
typedef struct PR_OptionSlider {
    PR_Rect background;
    char label[256];
    float value;
    bool mouse_hooked;
    char value_text[32];
    PR_Rect selection;
} PR_OptionSlider;
typedef struct PR_OptionsMenu {
    PR_MenuCamera camera;

    PR_Button to_start_menu;

    PR_OptionsMenuSelection current_selection;

    bool showing_general_pane;
    PR_Button to_general_pane;
    PR_Button to_controls_pane;
    // General pane options

    PR_OptionSlider master_volume;
    PR_OptionSlider sfx_volume;
    PR_OptionSlider music_volume;
    PR_DisplayMode display_mode_selection;
    PR_Button display_mode_fullscreen;
    PR_Button display_mode_borderless;
    PR_Button display_mode_windowed;
    PR_WindowResolution resolution_selection;
    PR_Button resolution_up;
    PR_Button resolution_down;

    PR_Button change_kb_binds1[PR_LAST_ACTION+1];
    PR_Button change_kb_binds2[PR_LAST_ACTION+1];
    PR_Button change_gp_binds1[PR_LAST_ACTION+1];
    PR_Button change_gp_binds2[PR_LAST_ACTION+1];

    int selected_row;
    int selected_column;
} PR_OptionsMenu;

typedef enum PR_DeleteButtonChoice {
    PR_BUTTON_YES,
    PR_BUTTON_NO,
} PR_DeleteButtonChoice;

typedef struct PR_PlayMenu {
    PR_MenuCamera camera;

    bool showing_campaign_buttons;
    PR_Button show_campaign_button;
    PR_Button show_custom_button;

    int selected_campaign_button;
    PR_LevelButton campaign_buttons[CAMPAIGN_LEVELS_NUMBER];

    int selected_custom_button;
    PR_CustomLevelButtons custom_buttons;

    PR_Button add_custom_button;

    PR_Button to_start_menu;

    PR_DeleteButtonChoice delete_selection;
    bool deleting_level;
    PR_Button delete_yes;
    PR_Button delete_no;
    size_t deleting_index;
    PR_Rect deleting_frame;
} PR_PlayMenu;

typedef enum PR_ObjectType {
    PR_PORTAL_TYPE = 0,
    PR_BOOST_TYPE = 1,
    PR_OBSTACLE_TYPE = 2,
    PR_GOAL_LINE_TYPE = 3,
    PR_P_START_POS_TYPE = 4,
} PR_ObjectType;
typedef struct PR_Obstacles {
    PR_Obstacle *items;
    size_t count;
    size_t capacity;
} PR_Obstacles;
typedef struct PR_BoostPads {
    PR_BoostPad *items;
    size_t count;
    size_t capacity;
} PR_BoostPads;
typedef struct PR_Portals {
    PR_Portal *items;
    size_t count;
    size_t capacity;
} PR_Portals;

typedef enum PR_GameMenuChoice {
    PR_BUTTON_RESUME,
    PR_BUTTON_RESTART,
    PR_BUTTON_QUIT,
} PR_GameMenuChoice;

typedef struct PR_Level {
    char file_path[99];
    char name[99];
    bool is_new;
    PR_Plane plane;
    PR_Camera camera;

    PR_Atmosphere air;
    PR_Rider rider;

    PR_Rect goal_line;
    PR_Rect start_pos;
    vec2f start_vel;

    PR_ParticleSystem particle_systems[3];
    PR_Parallax parallaxs[3];

    bool colors_shuffled;
    PR_ObstacleColorIndex current_red;
    PR_ObstacleColorIndex current_white;
    PR_ObstacleColorIndex current_blue;
    PR_ObstacleColorIndex current_gray;

    bool editing_available;
    bool editing_now;

    bool adding_now;

    PR_GameMenuChoice gamemenu_selected;
    bool pause_now;
    bool game_over;
    bool game_won;

    float finish_time;

    float text_wave_time;

    void *selected;
    void *old_selected;
    PR_ObjectType selected_type;
    PR_Button selected_options_buttons[SELECTED_MAX_OPTIONS];

    PR_Obstacles obstacles;
    PR_BoostPads boosts;
    PR_Portals portals;
} PR_Level;

typedef struct PR_Sound {
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
} PR_Sound;

typedef struct PR_RenderResources {
    mat4f ortho_proj;
    PR_Shader shaders[5];
    PR_Font fonts[3];
    PR_Texture global_sprite;
    PR_ArrayTexture array_textures[2];
} PR_RenderResources;

typedef enum PR_GameCase {
    PR_START_MENU = 0,
    PR_PLAY_MENU = 1,
    PR_OPTIONS_MENU = 2,
    PR_LEVEL = 3,
} PR_GameCase;
typedef struct PR_GameState {
    float delta_time;
    PR_GameCase current_case;
} PR_GameState;

typedef struct PR_WinInfo {
    int vertical_bar;
    int horizontal_bar;
    int width;
    int height;
    PR_WindowResolution windowed_resolution;
    PR_DisplayMode display_mode;
    const char* title;
    GLFWwindow* glfw_win;
} PR_WinInfo;

typedef struct PR {
    vec4f colors[4];

    PR_StartMenu current_start_menu;

    PR_OptionsMenu current_options_menu;

    PR_PlayMenu current_play_menu;

    PR_Level current_level;

    PR_Sound sound;

    PR_RenderResources rend_res;
    PR_Renderer renderer;

    PR_InputController input;

    PR_GameState state;

    PR_WinInfo window;

} PR;

extern PR* glob;

#endif
