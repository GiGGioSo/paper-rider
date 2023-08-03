#ifndef PP_GAME_H
#define PP_GAME_H

#include "../include/glm/trigonometric.hpp"
#include "pp_globals.h"

#define CHANGE_CASE_TO_LEVEL(map_path, level_name, edit, is_new, ret)  do {\
    PR::Level t_level = glob->current_level;\
    level_set_to_null(&t_level);\
    t_level.editing_available = (edit);\
    std::snprintf(t_level.name, std::strlen((level_name))+1,\
                  "%s", (level_name));\
    int preparation_result = level_prepare(&t_level, (map_path), (is_new));\
    if (preparation_result == 0) {\
        free_all_cases(\
                &glob->current_play_menu,\
                &glob->current_level,\
                &glob->current_start_menu,\
                &glob->current_options_menu);\
        glob->current_level = t_level;\
        glob->state.current_case = PR::LEVEL;\
        return(ret);\
    } else {\
        std::cout << "[ERROR] Could not prepare the level: "\
                  << (level_name)\
                  << std::endl;\
    }\
} while(0)

#define CHANGE_CASE_TO_PLAY_MENU(ret) do {\
    PR::PlayMenu t_menu = glob->current_play_menu;\
    play_menu_set_to_null(&t_menu);\
    int preparation_result = play_menu_prepare(&t_menu);\
    if (preparation_result == 0) {\
        free_all_cases(\
                &glob->current_play_menu,\
                &glob->current_level,\
                &glob->current_start_menu,\
                &glob->current_options_menu);\
        glob->current_play_menu = t_menu;\
        glob->state.current_case = PR::PLAY_MENU;\
        return(ret);\
    } else {\
        std::cout << "[ERROR] Could not prepare the play menu"\
                  << std::endl;\
    }\
} while(0)

#define CHANGE_CASE_TO_START_MENU(ret) do {\
    PR::StartMenu t_start = glob->current_start_menu;\
    start_menu_set_to_null(&t_start);\
    int preparation_result = start_menu_prepare(&t_start);\
    if (preparation_result == 0) {\
        free_all_cases(\
                &glob->current_play_menu,\
                &glob->current_level,\
                &glob->current_start_menu,\
                &glob->current_options_menu);\
        glob->current_start_menu = t_start;\
        glob->state.current_case = PR::START_MENU;\
        return(ret);\
    } else {\
        std::cout << "[ERROR] Could not prepare the start menu"\
                  << std::endl;\
    }\
} while(0)

#define CHANGE_CASE_TO_OPTIONS_MENU(ret) do {\
    PR::OptionsMenu t_options = glob->current_options_menu;\
    options_menu_set_to_null(&t_options);\
    int preparation_result = options_menu_prepare(&t_options);\
    if (preparation_result == 0) {\
        free_all_cases(\
                &glob->current_play_menu,\
                &glob->current_level,\
                &glob->current_start_menu,\
                &glob->current_options_menu);\
        glob->current_options_menu = t_options;\
        glob->state.current_case = PR::OPTIONS_MENU;\
        return(ret);\
    } else {\
        std::cout << "[ERROR] Could not prepare the options menu"\
                  << std::endl;\
    }\
} while(0)

void
free_all_cases(PR::PlayMenu *menu, PR::Level *level, PR::StartMenu *start, PR::OptionsMenu *opt);
void
play_menu_set_to_null(PR::PlayMenu *menu);
void
start_menu_set_to_null(PR::StartMenu *start);
void
options_menu_set_to_null(PR::OptionsMenu *opt);
void
level_set_to_null(PR::Level *level);

int play_menu_prepare(PR::PlayMenu *menu);
void play_menu_update(void);

int level_prepare(PR::Level *level, const char* mapfile_path, bool is_new);
void level_update(void);

int start_menu_prepare(PR::StartMenu *start);
void start_menu_update(void);

int options_menu_prepare(PR::OptionsMenu *opt);
void options_menu_update(void);

inline float lerp(float x1, float x2, float t) {
    float result = (1.f - t) * x1 + t * x2;
    return result;
}

inline glm::vec2 lerp_v2(glm::vec2 x1, glm::vec2 x2, float t) {
    glm::vec2 result;
    result.x = lerp(x1.x, x2.x, t);
    result.y = lerp(x1.y, x2.y, t);

    return result;
}

// NOTE: Vertical lift and horizontal drag are identical,
//       because falling is just like moving right.

// TODO: Maybe modify there coefficients to make them feel better?

inline
float vertical_lift_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(180.f - 2.f * angle));
    return result;
}

inline
float vertical_drag_coefficient(float angle) {
    float result = (float) sin(glm::radians(180.f - 2.f * angle));
    return result;
}

inline
float horizontal_lift_coefficient(float angle) {
    float result = (float) sin(glm::radians(2.f * angle));
    return result;
}

inline
float horizontal_drag_coefficient(float angle) {
    float result = 1.f - (float) cos(glm::radians(2.f * angle));
    return result;
}

inline
const char *get_case_name(PR::GameCase c) {
    switch (c) {
        case PR::START_MENU:
            return "START_MENU";
        case PR::PLAY_MENU:
            return "PLAY_MENU";
        case PR::OPTIONS_MENU:
            return "OPTIONS_MENU";
        case PR::LEVEL:
            return "LEVEL";
        default:
            return "UNKNOWN";
    }
    return "";
}

inline
const char *get_portal_type_name(PR::PortalType t) {
    switch (t) {
        case PR::INVERSE:
            return "INVERSE";
        case PR::SHUFFLE_COLORS:
            return "SHUFFLE_COLORS";
        default:
            return "UNKNOWN";
    }
    return "";
}

#endif
