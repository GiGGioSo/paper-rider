#ifndef _PR_TYPES_H
#define _PR_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pr_mathy.h"
#include "pr_rect.h"
#include "pr_polygon.h"

// ############################
// ### FORWARD DECLARATIONS ###
// ############################

typedef struct PR_TexCoords PR_TexCoords;

// ##################
// ### BASE TYPES ###
// ##################

// # BUTTONS
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


typedef enum PR_ObjectType {
    PR_PORTAL_TYPE = 0,
    PR_BOOST_TYPE = 1,
    PR_OBSTACLE_TYPE = 2,
    PR_GOAL_LINE_TYPE = 3,
    PR_P_START_POS_TYPE = 4,
} PR_ObjectType;

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

#define ObstaclePolygon Polygon4
typedef enum PR_ObstacleColorIndex {
    PR_RED = 0,
    PR_WHITE = 1,
    PR_BLUE = 2,
    PR_GRAY = 3,
} PR_ObstacleColorIndex;
typedef struct PR_Obstacle {
    ObstaclePolygon poly;
    PR_Rect body;
    bool collide_plane;
    bool collide_rider;
} PR_Obstacle;

#define BoostpadPolygon Polygon4
typedef struct PR_BoostPad {
    BoostpadPolygon poly;
    PR_Rect body;
    float boost_angle;
    float boost_power;
} PR_BoostPad;

#define PortalPolygon Polygon4
typedef enum PR_PortalType {
    PR_INVERSE = 0,
    PR_SHUFFLE_COLORS = 1,
} PR_PortalType;
typedef struct PR_Portal {
    PortalPolygon poly;
    PR_Rect body;
    PR_PortalType type;
    bool enable_effect;
} PR_Portal;

// ###################
// ### COLLECTIONS ###
// ###################

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

#endif//_PR_TYPES_H
