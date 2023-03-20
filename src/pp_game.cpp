#include <cassert>
#include <cstring>
#include <cmath>
#include <iostream>
#include <cstdio>
#include <cerrno>

#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_renderer.h"
#include "pp_rect.h"

#ifdef _WIN32
#    define MINIRENT_IMPLEMENTATION
#    include <minirent.h>
#else
#    include <dirent.h>
#endif // _WIN32

// TODO(before starting game editor):
//      - Proper gamepad support
//      - Fixing collision position
// TODO(after finishing game editor):
//      - Update the README
//      - Alphabetic order of the levels
//      - Better way to signal boost direction in boost pads
//      - Find/make textures
//      - Fine tune angle change velocity
//      - Profiling

// TODO(maybe):
//  - Make changing angle more or less difficult
//  - Make the plane change angle alone when the rider jumps off
//  - Make the boost change the plane angle


#define GRAVITY (9.81f * 50.f)

#define PLANE_VELOCITY_LIMIT (1000.f)
#define RIDER_VELOCITY_Y_LIMIT (600.f)
#define RIDER_INPUT_VELOCITY_LIMIT (550.f)

#define CAMERA_MAX_VELOCITY (1500.f)

#define CHANGE_CASE_TO(new_case, prepare_func, map_path, edit)  do {\
        PR::Level t_level = glob->current_level;\
        t_level.editing_available = edit;\
        PR::Menu t_menu = glob->current_menu;\
        int preparation_result = (prepare_func)(&t_menu, &t_level, map_path);\
        if (preparation_result == 0) {\
            free_menu_level(&glob->current_menu, &glob->current_level);\
            glob->current_level = t_level;\
            glob->current_menu = t_menu;\
            glob->state.current_case = new_case;\
            return;\
        } else {\
            std::cout << "[ERROR] Could not prepare case: "\
                      << get_case_name(new_case)\
                      << std::endl;\
        }\
    } while(0)

#define EDIT_BUTTON_DEFAULT_COLOR (glm::vec4(0.9f, 0.4f, 0.6f, 1.0f))
#define EDIT_BUTTON_SELECTED_COLOR (glm::vec4(1.0, 0.6f, 0.8f, 1.0f))

#define LEVEL_BUTTON_DEFAULT_COLOR (glm::vec4(0.8f, 0.2f, 0.5f, 1.0f))
#define LEVEL_BUTTON_SELECTED_COLOR (glm::vec4(0.6, 0.0f, 0.3f, 1.0f))

#define SHOW_BUTTON_DEFAULT_COLOR (glm::vec4(0.8f, 0.2f, 0.5f, 1.0f))
#define SHOW_BUTTON_SELECTED_COLOR (glm::vec4(0.6, 0.0f, 0.3f, 1.0f))

// Utilities functions for code reuse
void apply_air_resistances(PR::Plane* p);
void lerp_camera_x_to_rect(PR::Camera *cam, Rect *rec, bool center);
void move_rider_to_plane(PR::Rider *rid, PR::Plane *p);

// Particle system functions
void create_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle);
void update_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle);

void create_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle);
void update_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle);

void create_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle);
void update_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle);

void free_menu_level(PR::Menu *menu, PR::Level *level) {
    // Menu freeing
    if (menu->show_campaign_button.text) {
        std::free(menu->show_campaign_button.text);
    }
    for(size_t i = 0;
        i < ARRAY_LENGTH(menu->campaign_buttons);
        ++i) {
        if (menu->campaign_buttons[i].text) {
            std::free(menu->campaign_buttons[i].text);
        }
    }
    for(size_t i = 0;
        i < menu->custom_buttons_number;
        ++i) {
        if (menu->custom_buttons[i].text) {
            std::free(menu->custom_buttons[i].text);
            menu->custom_buttons[i].text = NULL;
        }
        if (menu->custom_edit_buttons[i].text) {
            std::free(menu->custom_edit_buttons[i].text);
            menu->custom_edit_buttons[i].text = NULL;
        }
    }
    if (menu->custom_buttons) {
        std::free(menu->custom_buttons);
        menu->custom_buttons = NULL;
    }
    if (menu->custom_edit_buttons) {
        std::free(menu->custom_edit_buttons);
        menu->custom_edit_buttons = NULL;
    }

    // Level freeing
    if (level->portals) {
        std::free(level->portals);
        level->portals = NULL;
    }
    if (level->obstacles) {
        std::free(level->obstacles);
        level->obstacles = NULL;
    }
    if (level->boosts) {
        std::free(level->boosts);
        level->boosts = NULL;
    }
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(level->particle_systems);
        ++ps_index) {
        if (level->particle_systems[ps_index].particles) {
            std::free(level->particle_systems[ps_index].particles);
            level->particle_systems[ps_index].particles = NULL;
        }
    }
}

inline
void menu_set_to_null(PR::Menu *menu) {
    for(size_t i = 0;
        i < ARRAY_LENGTH(menu->campaign_buttons);
        ++i) {
        menu->campaign_buttons[i].text = NULL;
    }
    for(size_t i = 0;
        i < menu->custom_buttons_number;
        ++i) {
        menu->custom_buttons[i].text = NULL;
        menu->custom_edit_buttons[i].text = NULL;
    }
    menu->custom_buttons_number = 0;
    menu->custom_buttons = NULL;
    menu->custom_edit_buttons = NULL;
    menu->show_campaign_button.text = NULL;
    menu->show_custom_button.text = NULL;
}

inline
void level_set_to_null(PR::Level *level) {
    level->portals_number = 0;
    level->portals = NULL;
    level->obstacles_number = 0;
    level->obstacles = NULL;
    level->boosts_number = 0;
    level->boosts = NULL;
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(level->particle_systems);
        ++ps_index) {
        level->particle_systems[ps_index].particles_number = 0;
        level->particle_systems[ps_index].particles = NULL;
    }
}

#define return_defer(ret) do { result = ret; goto defer; } while(0)
int load_map_from_file(const char *file_path,
                       PR::Obstacle **obstacles,
                       size_t *number_of_obstacles,
                       PR::BoostPad **boosts,
                       size_t *number_of_boosts,
                       PR::Portal **portals,
                       size_t *number_of_portals,
                       float *start_x, float *start_y,
                       float *goal_line,
                       const float width, const float height) {

    // NOTE: The screen resolution
    float proportion_x = width / SCREEN_WIDTH_PROPORTION;
    float proportion_y = height / SCREEN_HEIGHT_PROPORTION;
    
    int result = 0;
    FILE *map_file;

    {
        map_file = std::fopen(file_path, "r");
        if (map_file == NULL) return_defer(1);

        std::fscanf(map_file, " %zu", number_of_obstacles);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << *number_of_obstacles
                  << " obstacles from the file " << file_path
                  << std::endl;

        *obstacles = (PR::Obstacle *) std::malloc(sizeof(PR::Obstacle) *
                                                  *number_of_obstacles);
        if (*obstacles == NULL) {
            std::cout << "[ERROR] Buy more RAM!" << std::endl;
            return_defer(2);
        }

        for (size_t obstacle_index = 0;
             obstacle_index < *number_of_obstacles;
             ++obstacle_index) {

            int collide_plane, collide_rider, triangle;
            float x, y, w, h, r;

            std::fscanf(map_file,
                        " %i %i %i %f %f %f %f %f",
                        &collide_plane, &collide_rider, &triangle,
                        &x, &y, &w, &h, &r);

            if (std::ferror(map_file)) return_defer(1);
            // TODO: You cannot check feof on the obstacles
            //       because after them there are the boosts
            if (std::feof(map_file)) {
                *number_of_obstacles = obstacle_index;
                std::cout << "[WARNING] Found only " << obstacle_index
                          << " obstacles in the map file" << std::endl;
                return_defer(1);
            }

            PR::Obstacle *obs = *obstacles + obstacle_index;
            obs->body.pos.x = x * proportion_x;
            obs->body.pos.y = y * proportion_y;
            obs->body.dim.x = w * proportion_x;
            obs->body.dim.y = h * proportion_y;
            obs->body.angle = r;
            obs->body.triangle = triangle;

            obs->collide_plane = collide_plane;
            obs->collide_rider = collide_rider;

            std::cout << "x: " << x * proportion_x
                      << " y: " << y * proportion_y
                      << " w: " << w * proportion_x
                      << " h: " << h * proportion_y
                      << " r: " << r
                      << " tr: " << triangle
                      << " cp: " << collide_plane
                      << " cr: " << collide_rider
                      << std::endl;

        }

        // NOTE: Loading the boosts from memory
        std::fscanf(map_file, " %zu", number_of_boosts);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << *number_of_boosts
                  << " boost pads from file " << file_path
                  << std::endl;

        *boosts = (PR::BoostPad *) std::malloc(sizeof(PR::BoostPad) *
                                   *number_of_boosts);
        if (*boosts == NULL) {
            std::cout << "[ERROR] Buy more RAM!" << std::endl;
            return_defer(2);
        }

        for(size_t boost_index = 0;
            boost_index < *number_of_boosts;
            ++boost_index) {
            
            int triangle;
            float x, y, w, h, r, ba, bp;

            std::fscanf(map_file,
                        " %i %f %f %f %f %f %f %f",
                        &triangle, &x, &y, &w, &h, &r, &ba, &bp);

            if (std::ferror(map_file)) return_defer(1);
            if (std::feof(map_file)) {
                *number_of_boosts = boost_index;
                std::cout << "[WARNING] Found only " << boost_index
                          << " boosts in the map file" << std::endl;
                break;
            }

            PR::BoostPad *pad = *boosts + boost_index;
            pad->body.pos.x = x * proportion_x;
            pad->body.pos.y = y * proportion_y;
            pad->body.dim.x = w * proportion_x;
            pad->body.dim.y = h * proportion_y;
            pad->body.angle = r;
            pad->body.triangle = triangle;
            pad->boost_angle = ba;
            pad->boost_power = bp;

            std::cout << "x: " << x * proportion_x
                      << " y: " << y * proportion_y
                      << " w: " << w * proportion_x
                      << " h: " << h * proportion_y
                      << " r: " << r
                      << " tr: " << triangle
                      << " ba: " << ba
                      << " bp: " << bp
                      << std::endl;


        }

        // NOTE: Loading the portals from memory
        std::fscanf(map_file, " %zu", number_of_portals);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << *number_of_portals
                  << " portals from file " << file_path
                  << std::endl;

        *portals = (PR::Portal *) std::malloc(sizeof(PR::Portal) *
                                   *number_of_portals);
        if (*portals == NULL) {
            std::cout << "[ERROR] Buy more RAM!" << std::endl;
            return_defer(2);
        }

        for(size_t portal_index = 0;
            portal_index < *number_of_portals;
            ++portal_index) {

            float x, y, w, h;
            int type;
            int enable;

            std::fscanf(map_file,
                        " %i %i %f %f %f %f",
                        &type, &enable, &x, &y, &w, &h);
            if (std::ferror(map_file)) return_defer(1);
            if (std::feof(map_file)) {
                *number_of_portals = portal_index;
                std::cout << "[WARNING] Found only " << portal_index
                          << " portals in the map file" << std::endl;
                break;
            }

            PR::Portal *portal = *portals + portal_index;

            switch(type) {
                case PR::INVERSE:
                {
                    portal->type = PR::INVERSE;
                    break;
                }
                case PR::SHUFFLE_COLORS:
                {
                    portal->type = PR::SHUFFLE_COLORS;
                    break;
                }
                default:
                {
                    std::cout << "[WARNING] Unknown portal type: "
                              << type
                              << ". Assigning the default portal type: 0."
                              << std::endl;
                    break;
                }

            }

            portal->enable_effect = enable;
            portal->body.pos.x = x * proportion_x;
            portal->body.pos.y = y * proportion_y;
            portal->body.dim.x = w * proportion_x;
            portal->body.dim.y = h * proportion_y;
            portal->body.angle = 0.f;
            portal->body.triangle = false;

            std::cout << "x: " << x * proportion_x
                      << " y: " << y * proportion_y
                      << " w: " << w * proportion_x
                      << " h: " << h * proportion_y
                      << " type: " << type
                      << " enable: " << enable
                      << std::endl;
        }

        std::fscanf(map_file, " %f", goal_line);
        if (std::ferror(map_file)) return_defer(1);
        *goal_line = *goal_line * proportion_x;
        
        std::cout << "[LOADING] goal_line set at: "
                  << *goal_line
                  << std::endl;

        std::fscanf(map_file, " %f %f", start_x, start_y);
        if (std::ferror(map_file)) return_defer(1);
        *start_x = *start_x * proportion_x;
        *start_y = *start_y * proportion_y;

        std::cout << "[LOADING] player start position set to"
                  << " x: " << *start_x
                  << " y: " << *start_y
                  << std::endl;
    }

    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

int save_map_to_file(const char *file_path,
                     PR::Level *level,
                     const float width, const float height) {
    int result = 0;
    FILE *map_file = NULL;

    float inv_proportion_x = SCREEN_WIDTH_PROPORTION / width;
    float inv_proportion_y = SCREEN_HEIGHT_PROPORTION / height;

    {
        map_file = std::fopen(file_path, "wb");
        if (map_file == NULL) return_defer(1);

        std::fprintf(map_file, "%zu\n", level->obstacles_number);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t obs_index = 0;
            obs_index < level->obstacles_number;
            ++obs_index) {

            PR::Obstacle *obs = level->obstacles + obs_index;
            Rect b = obs->body;
            b.pos.x *= inv_proportion_x;
            b.pos.y *= inv_proportion_y;
            b.dim.x *= inv_proportion_x;
            b.dim.y *= inv_proportion_y;

            std::fprintf(map_file,
                         "%i %i %i %f %f %f %f %f\n",
                         obs->collide_plane, obs->collide_rider,
                         b.triangle, b.pos.x, b.pos.y,
                         b.dim.x, b.dim.y, b.angle);
            if (std::ferror(map_file)) return_defer(1);

        }

        std::fprintf(map_file, "%zu\n", level->boosts_number);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t boost_index = 0;
            boost_index < level->boosts_number;
            ++boost_index) {

            PR::BoostPad *pad = level->boosts + boost_index;
            Rect b = pad->body;
            b.pos.x *= inv_proportion_x;
            b.pos.y *= inv_proportion_y;
            b.dim.x *= inv_proportion_x;
            b.dim.y *= inv_proportion_y;

            std::fprintf(map_file,
                         "%i %f %f %f %f %f %f %f\n",
                         b.triangle, b.pos.x, b.pos.y,
                         b.dim.x, b.dim.y, b.angle,
                         pad->boost_angle, pad->boost_power);
            if (std::ferror(map_file)) return_defer(1);
        }

        std::fprintf(map_file, "%zu", level->portals_number);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t portal_index = 0;
            portal_index < level->portals_number;
            ++portal_index) {

            PR::Portal *portal = level->portals + portal_index;
            Rect b = portal->body;
            b.pos.x *= inv_proportion_x;
            b.pos.y *= inv_proportion_y;
            b.dim.x *= inv_proportion_x;
            b.dim.y *= inv_proportion_y;

            std::fprintf(map_file,
                        "%i %i %f %f %f %f\n",
                        portal->type, portal->enable_effect,
                        b.pos.x, b.pos.y, b.dim.x, b.dim.y);
            if (std::ferror(map_file)) return_defer(1);

        }
    }

    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

int load_custom_buttons_from_dir(const char *dir_path,
                                 PR::LevelButton **buttons,
                                 PR::LevelButton **edit_buttons,
                                 size_t *buttons_number) {

    PR::WinInfo *win = &glob->window;

    int result = 0;
    DIR *dir = NULL;
    *buttons = NULL;
    *edit_buttons = NULL;
    *buttons_number = 0;

    {
        dir = opendir(dir_path);
        if (dir == NULL) {
            std::cout << "[ERROR] Could not open directory: "
                      << dir_path << std::endl;
            return_defer(1);
        }

        size_t button_index = 0;

        dirent *dp = NULL;
        while ((dp = readdir(dir))) {

            const char *extension = std::strrchr(dp->d_name, '.');

            if (std::strcmp(extension, ".prmap") == 0) {

                char map_name[99] = "";
                std::strncpy(map_name,
                             dp->d_name,
                             std::strlen(dp->d_name) - std::strlen(extension));

                std::cout << "Found map_file: "
                          << map_name << " length: " << std::strlen(map_name)
                          << std::endl;

                size_t row = button_index / 3;
                size_t col = button_index % 3;

                char map_path[99] = "";
                std::strcat(map_path, dir_path);
                std::strcat(map_path, dp->d_name);
                std::cout << "map_path: " 
                          << map_path
                          << " length: "
                          << std::strlen(map_path)
                          << std::endl;

                // Creating the custom level button
                // NOTE: If it's the first button, malloc, otherwise just realloc
                *buttons = (button_index == 0) ?
                    (PR::LevelButton *) std::malloc(sizeof(PR::LevelButton)) :
                    (PR::LevelButton *) std::realloc(*buttons,
                                                     sizeof(PR::LevelButton) *
                                                      (button_index+1));

                PR::LevelButton *button = *buttons + button_index;

                button->body.pos.x = (col * 2.f + 1.f) / 6.f * win->w;
                button->body.pos.y = (row * 2.f + 3.f) / 10.f * win->h;
                button->body.dim.x = win->w / 4.f;
                button->body.dim.y = win->h / 8.f;
                button->body.angle = 0.f;
                button->body.triangle = false;
                button->col = LEVEL_BUTTON_DEFAULT_COLOR;
                button->from_center = true;
                button->text =
                    (char *) std::malloc(std::strlen(map_name) *
                                         sizeof(char));
                std::strcpy(button->text, map_name);

                button->mapfile_path =
                    (char *) std::malloc(std::strlen(map_path) *
                                         sizeof(char));
                std::strcpy(button->mapfile_path, map_path);

                // Creating the little custom level edit button
                *edit_buttons = (button_index == 0) ?
                    (PR::LevelButton *) std::malloc(sizeof(PR::LevelButton)) :
                    (PR::LevelButton *) std::realloc(*edit_buttons,
                                                     sizeof(PR::LevelButton) *
                                                      (button_index+1));
                PR::LevelButton *edit_button = *edit_buttons + button_index;

                edit_button->body.pos.x = button->body.pos.x +
                                            button->body.dim.x * 0.05f;
                edit_button->body.pos.y = button->body.pos.y +
                                            button->body.dim.y * 0.25f;
                edit_button->body.dim.x = button->body.dim.x * 0.4f;
                edit_button->body.dim.y = button->body.dim.y * 0.5f;
                edit_button->body.angle = 0.f;
                edit_button->body.triangle = false;
                edit_button->col = EDIT_BUTTON_DEFAULT_COLOR;
                edit_button->from_center = false;
                edit_button->text =
                    (char *) std::malloc(std::strlen("EDIT") * sizeof(char));
                std::strcpy(edit_button->text, "EDIT");

                edit_button->mapfile_path =
                    (char *) std::malloc(std::strlen(map_path) *
                                         sizeof(char));
                std::strcpy(edit_button->mapfile_path, map_path);

                ++button_index;
                
            }
        }
        *buttons_number = button_index;
    }

    defer:
    if (result != 0 && *buttons) std::free(*buttons);
    if (dir) {
        if (closedir(dir) < 0) {
            std::cout << "[ERROR] Could not close directory: "
                      << dir_path << std::endl;
            result = 1;
        }
    }
    return result;
}

glm::vec4 get_obstacle_color(PR::Obstacle *obs) {
    if (obs->collide_rider && obs->collide_plane) {
        return glob->colors[glob->current_level.current_red];
    } else
    if (obs->collide_rider) {
        return glob->colors[glob->current_level.current_white];
    } else
    if (obs->collide_plane) {
        return glob->colors[glob->current_level.current_blue];
    } else {
        return glob->colors[glob->current_level.current_gray];
    }
}

glm::vec4 get_portal_color(PR::Portal *portal) {
    switch(portal->type) {
        case PR::INVERSE:
        {
            return glm::vec4(0.f, 0.f, 0.f, 1.f);
            break;
        }
        case PR::SHUFFLE_COLORS:
        {
            return glm::vec4(1.f);
            break;
        }
    }
}


const char *campaign_levels_filepath[2] = {
    "",
    "./campaign_maps/level2.prmap"
};


int menu_prepare(PR::Menu *menu, PR::Level *level, const char* mapfile_path) {
    PR::WinInfo *win = &glob->window;

    level_set_to_null(level);

    // NOTE: By not setting this, it will remain the same as before
    //          starting the level
    //menu->showing_campaign_buttons = true;

    // NOTE: Button to select which buttons to show
    PR::LevelButton *campaign = &menu->show_campaign_button;
    campaign->body.pos.x = (win->w * 3.f) / 10.f;
    campaign->body.pos.y = win->h / 10.f;
    campaign->body.dim.x = win->w / 3.f;
    campaign->body.dim.y = win->h / 8.f;
    campaign->body.angle = 0.f;
    campaign->body.triangle = false;
    campaign->col =
        menu->showing_campaign_buttons ? SHOW_BUTTON_SELECTED_COLOR :
                                         SHOW_BUTTON_DEFAULT_COLOR;
    campaign->from_center = true;
    campaign->text = (char *) std::malloc(strlen("CAMPAIGN") * sizeof(char));
    std::sprintf(campaign->text, "CAMPAIGN");

    PR::LevelButton *custom = &menu->show_custom_button;
    custom->body.pos.x = (win->w * 7.f) / 10.f;
    custom->body.pos.y = win->h / 10.f;
    custom->body.dim.x = win->w / 3.f;
    custom->body.dim.y = win->h / 8.f;
    custom->body.angle = 0.f;
    custom->body.triangle = false;
    custom->col = menu->showing_campaign_buttons ? SHOW_BUTTON_DEFAULT_COLOR :
                                                   SHOW_BUTTON_SELECTED_COLOR;
    custom->from_center = true;
    custom->text = (char *) std::malloc(strlen("CUSTOM") * sizeof(char));
    std::sprintf(custom->text, "CUSTOM");

    // NOTE: I want to put 3 buttons on each row
    //       I want to have 4 rows of buttons on the screen,
    //        with some space on top (1/5) to change category (campaign/custom)
    for(size_t levelbutton_index = 0;
        levelbutton_index < ARRAY_LENGTH(menu->campaign_buttons);
        ++levelbutton_index) {

        size_t row = levelbutton_index / 3;
        size_t col = levelbutton_index % 3;

        PR::LevelButton *button =
            &menu->campaign_buttons[levelbutton_index];

        button->body.pos.x = (col * 2.f + 1.f) / 6.f * win->w;
        button->body.pos.y = (row * 2.f + 3.f) / 10.f * win->h;
        button->body.dim.x = win->w / 4.f;
        button->body.dim.y = win->h / 8.f;
        button->body.angle = 0.f;
        button->body.triangle = false;

        button->col = LEVEL_BUTTON_DEFAULT_COLOR;

        button->from_center = true;

        button->text = (char *) std::malloc(11 * sizeof(char));
        std::sprintf(button->text, "LEVEL %zu", levelbutton_index+1);
        if (levelbutton_index < ARRAY_LENGTH(campaign_levels_filepath)) {
            button->mapfile_path =
                (char *) campaign_levels_filepath[levelbutton_index];
        } else {
            button->mapfile_path = (char *) "";
        }
    }

    // NOTE: Custom levels buttons
    if (menu->showing_campaign_buttons) {
        menu->custom_buttons_number = 0;
        menu->custom_buttons = NULL;
        menu->custom_edit_buttons = NULL;
    } else {
        int result = 0;
        result =
            load_custom_buttons_from_dir("./custom_maps/",
                                         &menu->custom_buttons,
                                         &menu->custom_edit_buttons,
                                         &menu->custom_buttons_number);
        if (result != 0) {
            std::cout << "[ERROR] Could not load custom map files"
                      << std::endl;

            return result;
        }
    }

    PR::Camera *cam = &menu->camera;
    cam->pos.x = win->w * 0.5f;
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 6.f;
    menu->camera_goal_position = cam->pos.y;

    // NOTE: Have to set to null what is not used

    // NOTE: Make the cursor show
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    return 0;
}

void menu_update(void) {
    InputController *input = &glob->input;
    PR::WinInfo *win = &glob->window;
    PR::Camera *cam = &glob->current_menu.camera;
    PR::Menu *menu = &glob->current_menu;

    size_t buttons_shown_number =
        menu->showing_campaign_buttons ? CAMPAIGN_LEVELS_NUMBER :
                                         menu->custom_buttons_number;

    // NOTE: Consider the cursor only if it's inside the window
    if (0 < input->mouseX && input->mouseX < win->w &&
        0 < input->mouseY && input->mouseY < win->h) {

        // NOTE: 0.2f is an arbitrary amount
        if (menu->camera_goal_position > win->h*0.5f &&
            input->mouseY < win->h * 0.2f) {
            float cam_velocity = (1.f - (input->mouseY / (win->h*0.20))) *
                                 CAMERA_MAX_VELOCITY;
            menu->camera_goal_position -=
                cam_velocity * glob->state.delta_time;
        }

        // NOTE: 15 because there can be 15 buttons in the screen together
        //       +2 because i do:
        //          +3 to count for the empty row left at the top
        //          -1 so that when a row is exactly filled at the end
        //              it doesn't count that as another row
        size_t rows_in_screen = (((buttons_shown_number+2) % 15) / 3) + 1;
        float screens_of_buttons = ((buttons_shown_number+2) / 15) +
                                    rows_in_screen / 5.f;

        if (menu->camera_goal_position < win->h*screens_of_buttons &&
            input->mouseY > win->h * 0.8f) {
            float cam_velocity =
                ((input->mouseY - win->h*0.8f) / (win->h*0.20)) *
                CAMERA_MAX_VELOCITY;
            menu->camera_goal_position +=
                cam_velocity * glob->state.delta_time;
        }
    }

    cam->pos.y = lerp(cam->pos.y, menu->camera_goal_position,
         glob->state.delta_time * cam->speed_multiplier);

    // TODO: Maybe remove this
    if (input->level1.clicked) {
        CHANGE_CASE_TO(PR::LEVEL, level_prepare, "", false);
    } else if (input->level2.clicked) {
        CHANGE_CASE_TO(PR::LEVEL, level_prepare, "level2.prmap", false);
    }

    if (rect_contains_point(
                rect_in_camera_space(menu->show_campaign_button.body, cam),
                input->mouseX, input->mouseY,
                menu->show_campaign_button.from_center)) {
        if (input->mouse_left.clicked) {
            menu->showing_campaign_buttons = true;
            menu->show_campaign_button.col = SHOW_BUTTON_SELECTED_COLOR;
            menu->show_custom_button.col = SHOW_BUTTON_DEFAULT_COLOR;
            menu->camera_goal_position = win->h * 0.5f;
        }
    }
    if (rect_contains_point(
                rect_in_camera_space(menu->show_custom_button.body, cam),
                input->mouseX, input->mouseY,
                menu->show_custom_button.from_center)) {
        if (input->mouse_left.clicked) {
            int result = 0;
            PR::LevelButton *temp_custom_buttons = NULL;
            PR::LevelButton *temp_custom_edit_buttons = NULL;
            size_t temp_custom_buttons_number = 0;
            result =
                load_custom_buttons_from_dir("./custom_maps/",
                                             &temp_custom_buttons,
                                             &temp_custom_edit_buttons,
                                             &temp_custom_buttons_number);
            if (result == 0) {
                // NOTE: Free the previous present custom buttons
                for(size_t i = 0;
                    i < menu->custom_buttons_number;
                    ++i) {
                    if (menu->custom_buttons[i].text) {
                        std::free(menu->custom_buttons[i].text);
                        menu->custom_buttons[i].text = NULL;
                    }
                    if (menu->custom_edit_buttons[i].text) {
                        std::free(menu->custom_edit_buttons[i].text);
                        menu->custom_edit_buttons[i].text = NULL;
                    }
                }
                if (menu->custom_buttons) {
                    std::free(menu->custom_buttons);
                    menu->custom_buttons = NULL;
                }
                if (menu->custom_edit_buttons) {
                    std::free(menu->custom_edit_buttons);
                    menu->custom_edit_buttons = NULL;
                }
                // NOTE: Reassing the updated and checked values
                menu->custom_buttons = temp_custom_buttons;
                menu->custom_edit_buttons = temp_custom_edit_buttons;
                menu->custom_buttons_number = temp_custom_buttons_number;
                menu->showing_campaign_buttons = false;
                menu->show_campaign_button.col = SHOW_BUTTON_DEFAULT_COLOR;
                menu->show_custom_button.col = SHOW_BUTTON_SELECTED_COLOR;
                menu->camera_goal_position = win->h * 0.5f;
            } else {
                std::cout << "[ERROR] Could not load custom map files"
                          << std::endl;
            }
        }
    }

    if (menu->showing_campaign_buttons) {
        for(size_t levelbutton_index = 0;
            levelbutton_index < ARRAY_LENGTH(menu->campaign_buttons);
            ++levelbutton_index) {

            PR::LevelButton *button =
                &menu->campaign_buttons[levelbutton_index];

            if (rect_contains_point(rect_in_camera_space(button->body, cam),
                                    input->mouseX, input->mouseY,
                                    button->from_center)) {
                
                button->col = LEVEL_BUTTON_SELECTED_COLOR;
                if (input->mouse_left.clicked) {
                    CHANGE_CASE_TO(PR::LEVEL,
                                   level_prepare,
                                   button->mapfile_path, false);
                }
            } else {
                button->col = LEVEL_BUTTON_DEFAULT_COLOR;
            }
        }
    } else {
        for(size_t custombutton_index = 0;
            custombutton_index < menu->custom_buttons_number;
            ++custombutton_index) {

            PR::LevelButton *edit_button =
                menu->custom_edit_buttons + custombutton_index;

            PR::LevelButton *button =
                menu->custom_buttons + custombutton_index;

            if (rect_contains_point(rect_in_camera_space(edit_button->body, cam),
                                    input->mouseX, input->mouseY,
                                    edit_button->from_center)) {
                
                edit_button->col = EDIT_BUTTON_SELECTED_COLOR;
                button->col = LEVEL_BUTTON_DEFAULT_COLOR;
                if (input->mouse_left.clicked) {
                    CHANGE_CASE_TO(PR::LEVEL,
                                   level_prepare,
                                   edit_button->mapfile_path, true);
                }
            } else {
                edit_button->col = EDIT_BUTTON_DEFAULT_COLOR;

                // NOTE: Don't even check if it's inside the edit button
                if (rect_contains_point(rect_in_camera_space(button->body, cam),
                                        input->mouseX, input->mouseY,
                                        button->from_center)) {
                    
                    button->col = LEVEL_BUTTON_SELECTED_COLOR;
                    if (input->mouse_left.clicked) {
                        CHANGE_CASE_TO(PR::LEVEL,
                                       level_prepare,
                                       button->mapfile_path, false);
                    }
                } else {
                    button->col = LEVEL_BUTTON_DEFAULT_COLOR;
                }
            }

        }
    }

    return; 
}

void menu_draw(void) {
    if (glob->state.current_case != PR::MENU) return;

    PR::Menu *menu = &glob->current_menu;
    PR::Camera *cam = &glob->current_menu.camera;

    Rect campaign_rend_rect =
        rect_in_camera_space(menu->show_campaign_button.body, cam);
    renderer_add_queue_uni(campaign_rend_rect,
                           menu->show_campaign_button.col,
                           menu->show_campaign_button.from_center);
    renderer_add_queue_text(campaign_rend_rect.pos.x,
                            campaign_rend_rect.pos.y,
                            menu->show_campaign_button.text, glm::vec4(1.0f),
                            &glob->rend_res.fonts[0], true);

    Rect custom_rend_rect =
        rect_in_camera_space(menu->show_custom_button.body, cam);
    renderer_add_queue_uni(custom_rend_rect,
                           menu->show_custom_button.col,
                           menu->show_custom_button.from_center);
    renderer_add_queue_text(custom_rend_rect.pos.x,
                            custom_rend_rect.pos.y,
                            menu->show_custom_button.text, glm::vec4(1.0f),
                            &glob->rend_res.fonts[0], true);

    if (menu->showing_campaign_buttons) {
        for(size_t levelbutton_index = 0;
            levelbutton_index < CAMPAIGN_LEVELS_NUMBER;
            ++levelbutton_index) {

            PR::LevelButton *button =
                &menu->campaign_buttons[levelbutton_index];

            Rect button_rend_rect = rect_in_camera_space(button->body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   button->col,
                                   button->from_center);
            renderer_add_queue_text(button_rend_rect.pos.x,
                                    button_rend_rect.pos.y,
                                    button->text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);
        }
    } else {
        for(size_t custombutton_index = 0;
            custombutton_index < menu->custom_buttons_number;
            ++custombutton_index) {

            PR::LevelButton *button =
                menu->custom_buttons + custombutton_index;

            Rect button_rend_rect = rect_in_camera_space(button->body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   button->col,
                                   button->from_center);
            renderer_add_queue_text(button_rend_rect.pos.x,
                                    button_rend_rect.pos.y,
                                    button->text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);


            PR::LevelButton *edit_button =
                menu->custom_edit_buttons + custombutton_index;

            button_rend_rect = rect_in_camera_space(edit_button->body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   edit_button->col,
                                   edit_button->from_center);
            renderer_add_queue_text(button_rend_rect.pos.x+
                                        button_rend_rect.dim.x*0.5f,
                                    button_rend_rect.pos.y+
                                        button_rend_rect.dim.y*0.5f,
                                    edit_button->text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);
        }
    }

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    return;
}

int level_prepare(PR::Menu *menu, PR::Level *level, const char *mapfile_path) {
    PR::WinInfo *win = &glob->window;

    PR::Plane *p = &level->plane;
    PR::Rider *rid = &level->rider;

    // Have to set to NULL if I don't use it
    menu_set_to_null(menu);

    std::cout << "Is the level editable? "
              << level->editing_available
              << std::endl;

    PR::Camera *cam = &level->camera;
    // cam->pos.x is set afterwards
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 3.f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

    level->colors_shuffled = false;
    level->current_red = PR::RED;
    level->current_white = PR::WHITE;
    level->current_blue = PR::BLUE;
    level->current_gray = PR::GRAY;

    level->editing_now = false;

    if (std::strcmp(mapfile_path, "")) {

        // TODO: include this in the mapfile
        level->portals_number = 0;

        int loading_result =
            load_map_from_file(mapfile_path,
                &level->obstacles,
                &level->obstacles_number,
                &level->boosts,
                &level->boosts_number,
                &level->portals,
                &level->portals_number,
                &p->body.pos.x, &p->body.pos.y,
                &level->goal_line,
                win->w, win->h);
        if (loading_result != 0) return loading_result;

        // Hardcoding the fuck out of everything,
        // except for what is set in the mapfile
        {
            cam->pos.x = p->body.pos.x;

            p->crashed = false;
            p->crash_position.x = 0.f;
            p->crash_position.y = 0.f;
            p->body.dim.y = 23.f;
            p->body.dim.x = p->body.dim.y * 3.f;
            p->body.angle = 0.f;
            p->body.triangle = true;
            p->render_zone.dim.y = 25.f;
            p->render_zone.dim.x = p->render_zone.dim.y * 3.f;
            p->render_zone.pos = p->body.pos;
            /*p->render_zone.pos = p->body.pos +
                                 (p->body.dim - p->render_zone.dim) * 0.5f;*/
            p->render_zone.angle = p->body.angle;
            p->render_zone.triangle = false;
            p->vel.x = 0.f;
            p->vel.y = 0.f;
            p->acc.x = 0.f;
            p->acc.y = 0.f;
            p->mass = 0.003f; // kg
            // TODO: The alar surface should be somewhat proportional
            //       to the dimension of the actual rectangle
            p->alar_surface = 0.12f; // m squared
            p->current_animation = PR::Plane::IDLE_ACC;
            p->animation_countdown = 0.f;
            p->inverse = false;


            rid->crashed = false;
            rid->crash_position.x = 0.f;
            rid->crash_position.y = 0.f;
            rid->body.dim.x = 30.f;
            rid->body.dim.y = 50.f;
            rid->body.triangle = false;
            //move_rider_to_plane(rid, p);
            rid->body.angle = p->render_zone.angle;
            rid->body.pos.x =
                p->render_zone.pos.x +
                (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                    sin(glm::radians(rid->body.angle)) -
                (p->render_zone.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
            rid->body.pos.y =
                p->render_zone.pos.y +
                (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                    cos(glm::radians(rid->body.angle)) +
                (p->render_zone.dim.x*0.2f) * sin(glm::radians(rid->body.angle));
            rid->render_zone.dim = rid->body.dim;
            rid->render_zone.pos = rid->body.pos +
                                   (rid->body.dim - rid->render_zone.dim) * 0.5f;
            rid->vel.x = 0.0f;
            rid->vel.y = 0.0f;
            rid->body.angle = p->body.angle;
            rid->attached = true;
            rid->mass = 0.010f;
            rid->jump_time_elapsed = 0.f;
            rid->air_friction_acc = 100.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;
            rid->input_max_accelleration = 5000.f;
            rid->inverse = false;
        }
    } else {

        std::cout << "[WARNING] Loading fallback map information!"
                  << std::endl;

        // Hardcoding the fuck out of everything
        {
            level->goal_line = 1000.f;

            p->crashed = false;
            p->crash_position.x = 0.f;
            p->crash_position.y = 0.f;
            p->body.pos.x = 200.0f;
            p->body.pos.y = 350.0f;
            p->body.dim.y = 23.f;
            p->body.dim.x = p->body.dim.y * 3.f;
            p->body.angle = 0.f;
            p->body.triangle = true;
            p->render_zone.dim.y = 25.f;
            p->render_zone.dim.x = p->render_zone.dim.y * 3.f;
            p->render_zone.pos = p->body.pos;
            /*p->render_zone.pos = p->body.pos +
                                 (p->body.dim - p->render_zone.dim) * 0.5f;*/
            p->render_zone.angle = p->body.angle;
            p->render_zone.triangle = false;
            p->vel.x = 0.f;
            p->vel.y = 0.f;
            p->acc.x = 0.f;
            p->acc.y = 0.f;
            p->mass = 0.003f; // kg
            // TODO: The alar surface should be somewhat proportional
            //       to the dimension of the actual rectangle
            p->alar_surface = 0.12f; // m squared
            p->current_animation = PR::Plane::IDLE_ACC;
            p->animation_countdown = 0.f;
            p->inverse = false;

            cam->pos.x = p->body.pos.x;

            rid->crashed = false;
            rid->crash_position.x = 0.f;
            rid->crash_position.y = 0.f;
            rid->body.dim.x = 30.f;
            rid->body.dim.y = 50.f;
            rid->body.triangle = false;
            //move_rider_to_plane(rid, p);
            rid->body.angle = p->render_zone.angle;
            rid->body.pos.x =
                p->render_zone.pos.x +
                (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                    sin(glm::radians(rid->body.angle)) -
                (p->render_zone.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
            rid->body.pos.y =
                p->render_zone.pos.y +
                (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                    cos(glm::radians(rid->body.angle)) +
                (p->render_zone.dim.x*0.2f) * sin(glm::radians(rid->body.angle));
            rid->render_zone.dim = rid->body.dim;
            rid->render_zone.pos = rid->body.pos +
                                   (rid->body.dim - rid->render_zone.dim) * 0.5f;
            rid->vel.x = 0.0f;
            rid->vel.y = 0.0f;
            rid->body.angle = p->body.angle;
            rid->attached = true;
            rid->mass = 0.010f;
            rid->jump_time_elapsed = 0.f;
            rid->air_friction_acc = 100.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;
            rid->input_max_accelleration = 5000.f;
            rid->inverse = false;
        }

        level->portals_number = 2;

        if (level->portals_number) {
            level->portals =
                (PR::Portal *) std::malloc(sizeof(PR::Portal) *
                                           level->portals_number);
            if (level->portals == NULL) {
                std::cout << "Buy more RAM!" << std::endl;
                return 1;
            }
        }


        PR::Portal *inverse_portal = level->portals;
        inverse_portal->body.pos.x = 400.f;
        inverse_portal->body.pos.y = 300.f;
        inverse_portal->body.dim.x = 70.f;
        inverse_portal->body.dim.y = 400.f;
        inverse_portal->body.angle = 0.f;
        inverse_portal->body.triangle = false;
        inverse_portal->type = PR::SHUFFLE_COLORS;
        inverse_portal->enable_effect = true;

        PR::Portal *reverse_portal = level->portals + 1;
        reverse_portal->body.pos.x = 4300.f;
        reverse_portal->body.pos.y = 300.f;
        reverse_portal->body.dim.x = 70.f;
        reverse_portal->body.dim.y = 400.f;
        reverse_portal->body.angle = 0.f;
        reverse_portal->body.triangle = false;
        reverse_portal->type = PR::SHUFFLE_COLORS;
        reverse_portal->enable_effect = false;

        level->obstacles_number = 50;
        if (level->obstacles_number) {
            level->obstacles =
                (PR::Obstacle *) std::malloc(sizeof(PR::Obstacle) *
                                             level->obstacles_number);
            if (level->obstacles == NULL) {
                std::cout << "Buy more RAM!" << std::endl;
                return 1;
            }
        }
        for(size_t obstacle_index = 0;
            obstacle_index < level->obstacles_number;
            ++obstacle_index) {

            PR::Obstacle *obs = level->obstacles + obstacle_index;

            obs->body.pos.x = win->w * 0.8f * obstacle_index;
            obs->body.pos.y = win->h * 0.4f;

            obs->body.dim.x = win->w * 0.1f;
            obs->body.dim.y = win->h * 0.3f;

            obs->body.angle = 0.0f;
            obs->body.triangle = false;

            obs->collide_plane = (obstacle_index % 2);
            obs->collide_rider = !(obstacle_index % 2);
        }

        level->boosts_number = 2;
        if (level->boosts_number) {
            level->boosts =
                (PR::BoostPad *) std::malloc(sizeof(PR::BoostPad) *
                                        level->boosts_number);
            if (level->boosts == NULL) {
                std::cout << "Buy more RAM!" << std::endl;
                return 1;
            }
        }
        for (size_t boost_index = 0;
             boost_index < level->boosts_number;
             ++boost_index) {
            PR::BoostPad *pad = level->boosts + boost_index;

            pad->body.pos.x = (boost_index + 1) * 1100.f;
            pad->body.pos.y = 300.f;
            pad->body.dim.x = 700.f;
            pad->body.dim.y = 200.f;
            pad->body.angle = 40.f;
            pad->body.triangle = false;
            pad->boost_angle = pad->body.angle;
            pad->boost_power = 20.f;
        }
    }


    PR::ParticleSystem *boost_ps = &level->particle_systems[0];
    boost_ps->particles_number = 150;
    if (boost_ps->particles_number) {
        boost_ps->particles =
            (PR::Particle *) std::malloc(sizeof(PR::Particle) *
                                         boost_ps->particles_number);
        if (boost_ps->particles == NULL) {
            std::cout << "Buy more RAM!" << std::endl;
            return 1;
        }
    }
    boost_ps->current_particle = 0;
    boost_ps->time_between_particles = 0.f;
    boost_ps->time_elapsed = 0.f;
    boost_ps->active = false;
    boost_ps->create_particle = create_particle_plane_boost;
    boost_ps->update_particle = update_particle_plane_boost;
    for(size_t particle_index = 0;
        particle_index < boost_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = boost_ps->particles + particle_index;
        (boost_ps->create_particle)(boost_ps, particle);
    }

    PR::ParticleSystem *plane_crash_ps = &level->particle_systems[1];
    plane_crash_ps->particles_number = 30;
    if (plane_crash_ps->particles_number) {
        plane_crash_ps->particles =
            (PR::Particle *) std::malloc(sizeof(PR::Particle) *
                                         plane_crash_ps->particles_number);
        if (plane_crash_ps->particles == NULL) {
            std::cout << "Buy more RAM!" << std::endl;
            return 1;
        }
    }
    plane_crash_ps->current_particle = 0;
    plane_crash_ps->time_between_particles = 0.10f;
    plane_crash_ps->time_elapsed = 0.f;
    plane_crash_ps->active = false;
    plane_crash_ps->create_particle = create_particle_plane_crash;
    plane_crash_ps->update_particle = update_particle_plane_crash;
    for(size_t particle_index = 0;
        particle_index < plane_crash_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = plane_crash_ps->particles + particle_index;
        (plane_crash_ps->create_particle)(plane_crash_ps, particle);
    }

    PR::ParticleSystem *rider_crash_ps = &level->particle_systems[2];
    rider_crash_ps->particles_number = 30;
    if (rider_crash_ps->particles_number) {
        rider_crash_ps->particles =
            (PR::Particle *) std::malloc(sizeof(PR::Particle) *
                                     rider_crash_ps->particles_number);
        if (rider_crash_ps->particles == NULL) {
            std::cout << "Buy more RAM!" << std::endl;
            return 1;
        }
    }
    rider_crash_ps->current_particle = 0;
    rider_crash_ps->time_between_particles = 0.10f;
    rider_crash_ps->time_elapsed = 0.f;
    rider_crash_ps->active = false;
    rider_crash_ps->create_particle = create_particle_rider_crash;
    rider_crash_ps->update_particle = update_particle_rider_crash;
    for(size_t particle_index = 0;
        particle_index < rider_crash_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = rider_crash_ps->particles +
                                 particle_index;
        (rider_crash_ps->create_particle)(rider_crash_ps, particle);
    }

    // NOTE: Hide the cursor only if it succeded in doing everything else
    glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    return 0;
}

void level_update(void) {
    // Level stuff
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;
    size_t portals_number = glob->current_level.portals_number;
    PR::Portal *portals = glob->current_level.portals;

    PR::Level *level = &glob->current_level;

    PR::ParticleSystem *boost_ps =
        &glob->current_level.particle_systems[0];
    PR::ParticleSystem *plane_crash_ps =
        &glob->current_level.particle_systems[1];
    PR::ParticleSystem *rider_crash_ps =
        &glob->current_level.particle_systems[2];

    // Global stuff
    PR::WinInfo *win = &glob->window;
    InputController *input = &glob->input;
    float dt = glob->state.delta_time;

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu.clicked) {
        CHANGE_CASE_TO(PR::MENU, menu_prepare, "", false);
    }

    if (level->editing_available &&
        input->edit.clicked) {

        if (level->editing_now) {
            std::cout << "Deactivating edit mode!" << std::endl;
            level->editing_now = false;
            glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else {
            std::cout << "Activating edit mode!" << std::endl;
            level->editing_now = true;
            if (p->inverse) {
                p->body.pos.y += p->body.dim.y;
                p->body.dim.y = -p->body.dim.y;
            }
            p->inverse = false;
            p->crashed = false;
            p->vel = glm::vec2(0.f);
            if (rid->inverse) {
                rid->body.dim.y = -rid->body.dim.y;
            }
            rid->inverse = false;
            rid->crashed = false;
            rid->attached = true;
            rid->vel = glm::vec2(0.f);
            glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    #if 0
    // Do I want the plane to keep going if the rider crashes?

    // Structure of the update loop of level1
    if (!p->crashed) {
        p->update();
        if (!rid->crashed) {
            if (rid->attached) {
                p->input();
                rid->move_to_plane();
                cam->to_plane();
            } else { // !rid->attached
                rid->input();
                rid->update();
                cam->to_rider();
            }
        } else { // rid->crashed
            rid->crash_particles();
            if (rid->attached) {
                cam->to_plane();
            } else {
                cam->to_rider();
            }
        }
    } else { // p->crashed
        p->particles();
        if (!rid->crashed) {
            if (rid->attached) {
                rid->move_to_plane();
                cam->to_plane();
            } else { // !rid->attached
                rid->input();
                rid->update();
                cam->to_rider();
            }
        } else { // rid->crashed
            rid->crash_particles();
            if (rid->attached) {
                cam->to_plane();
            } else { // !rid->attached
                cam->to_rider();
            }
        }
    }
    #endif

    // It will be set to true once the boost key is pressed
    boost_ps->active = false;
    plane_crash_ps->active = false;
    rider_crash_ps->active = false;

    if (!p->crashed) {
        // #### START PLANE STUFF
        // NOTE: Reset the accelleration for it to be recalculated
        p->acc *= 0;

        if (level->editing_now) { // EDITING
            float velx = 1000.f;
            float vely = 600.f;
            if (input->up.pressed) p->body.pos.y -= vely * dt;
            if (input->down.pressed) p->body.pos.y += vely * dt;
            if (input->left.pressed) p->body.pos.x -= velx * dt;
            if (input->right.pressed) p->body.pos.x += velx * dt;
        } else { // PLAYING
            apply_air_resistances(p);

            // NOTE: Checking collision with boost_pads
            for (size_t boost_index = 0;
                 boost_index < boosts_number;
                 ++boost_index) {

                PR::BoostPad *pad = boosts + boost_index;

                if (rect_are_colliding(&p->body, &pad->body, NULL, NULL)) {

                    p->acc.x += pad->boost_power *
                                cos(glm::radians(pad->boost_angle));
                    p->acc.y += pad->boost_power *
                                -sin(glm::radians(pad->boost_angle));

                    // std::cout << "BOOOST!!! against " << boost_index << std::endl;
                }
            }
            // Propulsion
            // TODO: Could this be a "powerup" or something?
            if (glob->input.boost.pressed &&
                !rid->crashed && rid->attached) {
                float propulsion = 8.f;
                p->acc.x += propulsion * cos(glm::radians(p->body.angle));
                p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
                boost_ps->active = true;
            }
            // NOTE: The mass is greater if the rider is attached
            if (rid->attached) p->acc *= 1.f/(p->mass + rid->mass);
            else p->acc *= 1.f/p->mass;

            // NOTE: Gravity is already an accelleration so it doesn't need to be divided
            p->acc.y += p->inverse ? -GRAVITY : GRAVITY;

            // NOTE: Motion of the plane
            p->vel += p->acc * dt;
            // NOTE: Limit velocities
            if (glm::length(p->vel) > PLANE_VELOCITY_LIMIT) {
                p->vel *= PLANE_VELOCITY_LIMIT / glm::length(p->vel);
            }
            p->body.pos += p->vel * dt + p->acc * POW2(dt) * 0.5f;
        }

        // #### END PLANE STUFF
        if (!rid->crashed) {
            if (rid->attached) {
                move_rider_to_plane(rid, p);
                // NOTE: Changing plane angle based on input
                if (input->left_right) {
                    p->body.angle -= p->inverse ?
                                     -150.f * input->left_right * dt :
                                     150.f * input->left_right * dt;
                }
                // NOTE: Limiting the angle of the plane
                if (p->body.angle > 360.f) {
                    p->body.angle -= 360.f;
                }
                if (p->body.angle < -360) {
                    p->body.angle += 360.f;
                }
                // NOTE: Make the rider jump based on input
                if (!level->editing_now && input->jump.clicked) { // PLAYING
                    rid->attached = false;
                    rid->second_jump = true;
                    rid->base_velocity = p->vel.x;
                    rid->vel.y = rid->inverse ? p->vel.y*0.5f + 500.f :
                                                p->vel.y*0.5f - 500.f;
                    rid->body.angle = 0.f;
                    rid->jump_time_elapsed = 0.f;
                }
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached

                // NOTE: Modify accelleration based on input
                if (input->left_right) {
                    rid->input_velocity +=
                        rid->inverse ?
                        -rid->input_max_accelleration *
                            input->left_right * dt :
                        rid->input_max_accelleration *
                            input->left_right * dt;
                } else {
                    rid->input_velocity +=
                        rid->input_max_accelleration *
                            -glm::sign(rid->input_velocity) * dt;
                }
                // NOTE: Base speed is the speed of the plane at the moment of the jump,
                //          which decreases slowly but constantly overtime
                //       Input speed is the speed that results from the player input.
                //
                //       This way, by touching nothing you get to keep the plane velocity,
                //       but you are still able to move left and right in a satisfying way
                if (glm::abs(rid->input_velocity) > RIDER_INPUT_VELOCITY_LIMIT) {
                    rid->input_velocity = glm::sign(rid->input_velocity) *
                                          RIDER_INPUT_VELOCITY_LIMIT;
                }
                rid->base_velocity -= glm::sign(rid->base_velocity) *
                                      rid->air_friction_acc * dt;

                rid->vel.x = rid->base_velocity + rid->input_velocity;
                rid->vel.y += rid->inverse ? -GRAVITY * 1.5f * dt :
                                             GRAVITY * 1.5f * dt;

                // NOTE: Rider double jump if available
                if(rid->second_jump && input->jump.clicked) {
                    if ((!rid->inverse && rid->vel.y < 0) ||
                         (rid->inverse && rid->vel.y > 0)) {
                        rid->vel.y -= rid->inverse ? -300.f:
                                                     300.f;
                    } else {
                        rid->vel.y = rid->inverse ? 300.f:
                                                    -300.f;
                    }
                    rid->second_jump = false;
                }

                // NOTE: Limit velocity before applying it to the rider
                if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
                    rid->vel.y = glm::sign(rid->vel.y) *
                                 RIDER_VELOCITY_Y_LIMIT ;
                }
                rid->body.pos += rid->vel * dt;
                rid->jump_time_elapsed += dt;

                // NOTE: Check if rider remounts the plane
                if (rect_are_colliding(&p->body, &rid->body, NULL, NULL) &&
                    rid->jump_time_elapsed > 0.5f) {
                    rid->attached = true;
                    p->vel += (rid->vel - p->vel) * 0.5f;
                    rid->vel *= 0.f;
                }

                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        } else { // rid->crashed
            rider_crash_ps->active = true;
            if (rid->attached) {
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached
                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        }
    } else { // p->crashed
        plane_crash_ps->active = true;
        if (!rid->crashed) {
            if (rid->attached) {
                move_rider_to_plane(rid, p);
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached

                // NOTE: Modify accelleration based on input
                if (input->left_right) {
                    rid->input_velocity +=
                        rid->inverse ?
                        -rid->input_max_accelleration *
                            input->left_right * dt :
                        rid->input_max_accelleration *
                            input->left_right * dt;
                } else {
                    rid->input_velocity +=
                        rid->input_max_accelleration *
                            -glm::sign(rid->input_velocity) * dt;
                }
                // NOTE: Base speed is the speed of the plane at the moment of the jump,
                //          which decreases slowly but constantly overtime
                //       Input speed is the speed that results from the player input.
                //
                //       This way, by touching nothing you get to keep the plane velocity,
                //       but you are still able to move left and right in a satisfying way
                if (glm::abs(rid->input_velocity) > RIDER_INPUT_VELOCITY_LIMIT) {
                    rid->input_velocity = glm::sign(rid->input_velocity) *
                                          RIDER_INPUT_VELOCITY_LIMIT;
                }
                rid->base_velocity -= glm::sign(rid->base_velocity) *
                                      rid->air_friction_acc * dt;

                rid->vel.x = rid->base_velocity + rid->input_velocity;
                rid->vel.y += rid->inverse ? -GRAVITY * 1.5f * dt :
                                             GRAVITY * 1.5f * dt;

                // NOTE: Rider double jump if available
                if(rid->second_jump && input->jump.clicked) {
                    if ((!rid->inverse && rid->vel.y < 0) ||
                         (rid->inverse && rid->vel.y > 0)) {
                        rid->vel.y -= rid->inverse ? -300.f:
                                                     300.f;
                    } else {
                        rid->vel.y = rid->inverse ? 300.f:
                                                    -300.f;
                    }
                    rid->second_jump = false;
                }

                // NOTE: Limit velocity before applying it to the rider
                if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
                    rid->vel.y = glm::sign(rid->vel.y) *
                                 RIDER_VELOCITY_Y_LIMIT ;
                }
                rid->body.pos += rid->vel * dt;
                rid->jump_time_elapsed += dt;

                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        } else { // rid->crashed
            rider_crash_ps->active = true;
            if (rid->attached) {
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached
                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        }
    }


    if (!p->crashed && !level->editing_now &&
        p->body.pos.x + p->body.dim.x*0.5f > level->goal_line) {
        CHANGE_CASE_TO(PR::MENU, menu_prepare, "", false);
    }


    if (!level->editing_now) {
        // NOTE: The portal can be activated only by the rider.
        //       If the rider is attached, then, by extensions, also
        //          the plane will activate the portal.
        //       Even if the rider is not attached, the effect is also
        //          applied to the plane.
        for(size_t portal_index = 0;
            portal_index < portals_number;
            ++portal_index) {

            PR::Portal *portal = portals + portal_index;

            if (!rid->crashed &&
                (rect_are_colliding(&rid->body, &portal->body, NULL, NULL) ||
                 (rid->attached && rect_are_colliding(&p->body,
                                                     &portal->body,
                                                     NULL, NULL)))) {
                switch(portal->type) {
                    case PR::INVERSE:
                    {
                        // NOTE: Skip if the plane/rider already has the effect
                        if (p->inverse == portal->enable_effect ||
                            rid->inverse == portal->enable_effect) break;

                        if (!p->crashed) {
                            p->inverse = portal->enable_effect;
                            p->body.pos.y += p->body.dim.y;
                            p->body.dim.y = -p->body.dim.y;
                        }
                        rid->inverse = portal->enable_effect;
                        rid->body.dim.y = -rid->body.dim.y;
                        break;
                    }
                    case PR::SHUFFLE_COLORS:
                    {
                        if (level->colors_shuffled == portal->enable_effect) break;

                        level->colors_shuffled = portal->enable_effect;

                        if (portal->enable_effect) {
                            level->current_red = PR::WHITE;
                            level->current_blue = PR::RED;
                            level->current_gray = PR::BLUE;
                            level->current_white = PR::GRAY;
                        } else {
                            level->current_red = PR::RED;
                            level->current_blue = PR::BLUE;
                            level->current_gray = PR::GRAY;
                            level->current_white = PR::WHITE;
                        }

                        break;
                    }
                }
            }
        }

        // NOTE: Checking collision with obstacles
        for (int obstacle_index = 0;
             obstacle_index < obstacles_number;
             obstacle_index++) {

            PR::Obstacle *obs = obstacles + obstacle_index;

            if (!p->crashed && obs->collide_plane &&
                rect_are_colliding(&p->body, &obs->body,
                                   &p->crash_position.x,
                                   &p->crash_position.y)) {
                // NOTE: Plane colliding with an obstacle

                if (rid->attached) {
                    rid->vel *= 0.f;
                    rid->base_velocity = 0.f;
                    rid->input_velocity = 0.f;
                }
                p->crashed = true;
                p->acc *= 0.f;
                p->vel *= 0.f;

                // TODO: Debug flag
                std::cout << "Plane collided with " << obstacle_index << std::endl;
            }
            if (!rid->crashed && obs->collide_rider &&
                rect_are_colliding(&rid->body, &obs->body,
                                   &rid->crash_position.x,
                                   &rid->crash_position.y)) {
                // NOTE: Rider colliding with an obstacle

                rid->crashed = true;
                rid->attached = false;
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;

                // TODO: Debug flag
                std::cout << "Rider collided with " << obstacle_index << std::endl;
            }
        }
    }

    // NOTE: Loop over window edged pacman style,
    //       but only on the top and bottom
    if (p->body.pos.y + p->body.dim.y/2 > win->h) {
        p->body.pos.y -= win->h;
    }
    if (p->body.pos.y + p->body.dim.y/2 < 0) {
        p->body.pos.y += win->h;
    }

    if (!rid->attached) {
        if (rid->body.pos.y + rid->body.dim.y * 0.5f > win->h) {
            rid->body.pos.y -= win->h;
        }
        if (rid->body.pos.y + rid->body.dim.y * 0.5f < 0) {
            rid->body.pos.y += win->h;
        }
    }

    // NOTE: Update the `render_zone`s based on the `body`s
    p->render_zone.pos.x = p->body.pos.x;
    p->render_zone.pos.y = p->inverse ? p->body.pos.y+p->body.dim.y :
                                        p->body.pos.y;
    p->render_zone.angle = p->body.angle;

    rid->render_zone.pos = rid->body.pos +
                           (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->render_zone.angle = rid->body.angle;

    // NOTE: Animation based on the accelleration
    if (p->animation_countdown < 0) {
        if (glm::abs(p->acc.y) < 20.f) {
            p->current_animation = PR::Plane::IDLE_ACC;
            p->animation_countdown = 0.25f;
        } else {
            if (p->current_animation == PR::Plane::UPWARDS_ACC) {
                p->current_animation =
                    (p->acc.y > 0) ?
                    PR::Plane::IDLE_ACC :
                    PR::Plane::UPWARDS_ACC;
            } else
            if (p->current_animation == PR::Plane::IDLE_ACC) {
                p->current_animation =
                    (p->acc.y > 0) ?
                    PR::Plane::DOWNWARDS_ACC :
                    PR::Plane::UPWARDS_ACC;
            } else
            if (p->current_animation == PR::Plane::DOWNWARDS_ACC) {
                p->current_animation =
                    (p->acc.y > 0) ?
                    PR::Plane::DOWNWARDS_ACC :
                    PR::Plane::IDLE_ACC;
            }

            p->animation_countdown = 0.25f;
        }
    } else {
        p->animation_countdown -= dt;
    }
}

void level_draw(void) {
    if (glob->state.current_case != PR::LEVEL) return;

    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;
    size_t portals_number = glob->current_level.portals_number;
    PR::Portal *portals = glob->current_level.portals;

    PR::Level *level = &glob->current_level;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    float dt = glob->state.delta_time;

    renderer_add_queue_uni(level->goal_line - cam->pos.x + win->w*0.5f, 0.f,
                           30.f, win->h,
                           0.f, glm::vec4(1.0f,1.0f,1.0f,1.0f), false, false);

    // TESTING: Rendering the portals
    for(size_t portal_index = 0;
        portal_index < portals_number;
        ++portal_index) {

        PR::Portal *portal = portals + portal_index;

        if (portal->type == PR::SHUFFLE_COLORS) {

            Rect b = portal->body;

            Rect q1;
            q1.angle = 0.f;
            q1.triangle = false;
            q1.pos = b.pos;
            q1.dim = b.dim * 0.5f;
            renderer_add_queue_uni(
                rect_in_camera_space(q1, cam),
                glob->colors[glob->current_level.current_gray],
                false);

            Rect q2;
            q2.angle = 0.f;
            q2.triangle = false;
            q2.pos.x = b.pos.x + b.dim.x*0.5f;
            q2.pos.y = b.pos.y;
            q2.dim = b.dim * 0.5f;
            renderer_add_queue_uni(
                rect_in_camera_space(q2, cam),
                glob->colors[glob->current_level.current_white],
                false);

            Rect q3;
            q3.angle = 0.f;
            q3.triangle = false;
            q3.pos.x = b.pos.x;
            q3.pos.y = b.pos.y + b.dim.y*0.5f;
            q3.dim = b.dim * 0.5f;
            renderer_add_queue_uni(
                rect_in_camera_space(q3, cam),
                glob->colors[glob->current_level.current_blue],
                false);

            Rect q4;
            q4.angle = 0.f;
            q4.triangle = false;
            q4.pos.x = b.pos.x + b.dim.x*0.5f;
            q4.pos.y = b.pos.y + b.dim.y*0.5f;
            q4.dim = b.dim * 0.5f;
            renderer_add_queue_uni(
                rect_in_camera_space(q4, cam),
                glob->colors[glob->current_level.current_red],
                false);
        } else {
            renderer_add_queue_uni(rect_in_camera_space(portal->body, cam),
                                   get_portal_color(portal),
                                   false);
        }
    }

    // NOTE: Rendering the boosts
    for(size_t boost_index = 0;
        boost_index < boosts_number;
        ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

        if (pad_in_cam_pos.pos.x < -((float)win->w) ||
            pad_in_cam_pos.pos.x > win->w * 2.f) continue;

        renderer_add_queue_uni(pad_in_cam_pos,
                              glm::vec4(0.f, 1.f, 0.f, 1.f),
                              false);
        
        if (pad->body.triangle) {
            pad_in_cam_pos.pos.x += pad_in_cam_pos.dim.x * 0.25f;
            pad_in_cam_pos.pos.y += pad_in_cam_pos.dim.y * 0.5f;
            pad_in_cam_pos.dim *= 0.4f;
        } else  {
            pad_in_cam_pos.pos += pad_in_cam_pos.dim * 0.5f;
            pad_in_cam_pos.dim *= 0.5f;
        }
        pad_in_cam_pos.angle = pad->boost_angle;
        renderer_add_queue_tex(pad_in_cam_pos,
                               texcoords_in_texture_space(
                                   0.f, 8.f, 96.f, 32.f,
                                   &glob->rend_res.global_sprite, false),
                               true);

    }

    // NOTE: Rendering the obstacles
    for(size_t obstacle_index = 0;
        obstacle_index < obstacles_number;
        ++obstacle_index) {

        PR::Obstacle *obs = obstacles + obstacle_index;

        Rect obs_in_cam_pos = rect_in_camera_space(obs->body, cam);

        if (obs_in_cam_pos.pos.x < -((float)win->w) ||
            obs_in_cam_pos.pos.x > win->w * 2.f) continue;

        renderer_add_queue_uni(obs_in_cam_pos,
                              get_obstacle_color(obs),
                              false);
    }

    // Set the time_between_particles for the boost based on the velocity
    glob->current_level.particle_systems[0].time_between_particles =
        lerp(0.02f, 0.01f, glm::length(p->vel)/PLANE_VELOCITY_LIMIT);



    // NOTE: Rendering the plane
    renderer_add_queue_uni(rect_in_camera_space(p->body, cam),
                           glm::vec4(1.0f, 1.0f, 1.0f, 1.f),
                           false);

    /*
    glm::vec2 p_cam_pos = rect_in_camera_space(p->render_zone, cam).pos;
    if (glob->input.boost) {
        float bx = p_cam_pos.x + p->render_zone.dim.x*0.5f -
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f *
                    cos(glm::radians(p->render_zone.angle));
        float by = p_cam_pos.y + p->render_zone.dim.y*0.5f +
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f *
                    sin(glm::radians(p->render_zone.angle));

        // NOTE: Rendering the boost of the plane
        renderer_add_queue_uni(bx, by,
                              p->render_zone.dim.y, p->render_zone.dim.y,
                              p->render_zone.angle,
                              glm::vec4(1.0f, 0.0f, 0.0f, 1.f),
                              true);
    }
    */

    renderer_add_queue_uni(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);


    // NOTE: Rendering plane texture
    renderer_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                           texcoords_in_texture_space(
                                p->current_animation * 32.f, 0.f,
                                32.f, 8.f,
                                &glob->rend_res.global_sprite, p->inverse),
                           false);

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_tex(glob->rend_res.shaders[1],
                      &glob->rend_res.global_sprite);

    // NOTE: Updating and rendering all the particle systems
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(glob->current_level.particle_systems);
        ++ps_index) {

        PR::ParticleSystem *ps =
            &glob->current_level.particle_systems[ps_index];

        if (ps->particles_number == 0) continue;

        ps->time_elapsed += dt;
        if (ps->time_elapsed > ps->time_between_particles) {
            ps->time_elapsed -= ps->time_between_particles;

            PR::Particle *particle = ps->particles +
                                     ps->current_particle;
            ps->current_particle = (ps->current_particle + 1) %
                                         ps->particles_number;

            (ps->create_particle)(ps, particle);
        }
        for (size_t particle_index = 0;
             particle_index < ps->particles_number;
             ++particle_index) {

            PR::Particle *particle = ps->particles + particle_index;

            (ps->update_particle)(ps, particle);

            renderer_add_queue_uni(rect_in_camera_space(particle->body, cam),
                                 particle->color, true);
        }
    }
    renderer_draw_uni(glob->rend_res.shaders[0]);
}

// Utilities

void apply_air_resistances(PR::Plane* p) {
    PR::Atmosphere *air = &glob->current_level.air;


    float vertical_alar_surface = p->alar_surface * cos(glm::radians(p->body.angle));

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_lift = vertical_alar_surface *
                            POW2(p->vel.y) * air->density *
                            vertical_lift_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `vertical_lift`
    vertical_lift = glm::abs(vertical_lift) *
                    glm::sign(-p->vel.y);

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float vertical_drag = vertical_alar_surface *
                            POW2(p->vel.y) * air->density *
                            vertical_drag_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `vertical_drag`
    if ((0 < p->body.angle && p->body.angle <= 90) || // 1o quadrante
        (180 < p->body.angle && p->body.angle <= 270) || // 3o quadrante
        (-360 < p->body.angle && p->body.angle <= -270) || // 1o quadrante
        (-180 < p->body.angle && p->body.angle <= -90))  { // 3o quadrante
        vertical_drag = -glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    } else {
        vertical_drag = glm::abs(vertical_drag) *
                            glm::sign(p->vel.y);
    }

    p->acc.y += vertical_lift;
    p->acc.x += vertical_drag;

    float horizontal_alar_surface = p->alar_surface * sin(glm::radians(p->body.angle));

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float horizontal_lift = horizontal_alar_surface *
                            POW2(p->vel.x) * air->density *
                            horizontal_lift_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `horizontal_lift`
    if ((0 < p->body.angle && p->body.angle <= 90) ||
        (180 < p->body.angle && p->body.angle <= 270) ||
        (-360 < p->body.angle && p->body.angle <= -270) ||
        (-180 < p->body.angle && p->body.angle <= -90))  {
        horizontal_lift = glm::abs(horizontal_lift) *
                            -glm::sign(p->vel.x);
    } else {
        horizontal_lift = glm::abs(horizontal_lift) *
                            glm::sign(p->vel.x);
    }

    // TODO: the same values are calculated multiple times,
    //       OPTIMIZE! (when you settled the mechanics)
    float horizontal_drag = horizontal_alar_surface *
                            POW2(p->vel.x) * air->density *
                            horizontal_drag_coefficient(p->body.angle) * 0.5f;
    // NOTE: Correcting the sign of `horizontal_drag`
    horizontal_drag = glm::abs(horizontal_drag) *
                        glm::sign(-p->vel.x);

    p->acc.y += horizontal_lift;
    p->acc.x += horizontal_drag;

    /* std::cout << "VL: " << vertical_lift << */
    /*             ",  VD: " << vertical_drag << */
    /*             ",  HL: " << horizontal_lift << */
    /*             ",  HD: " << horizontal_drag << std::endl; */
}

void lerp_camera_x_to_rect(PR::Camera *cam, Rect *rec, bool center) {
    // NOTE: Making the camera move to the plane
    PR::Level *level = &glob->current_level;
    float dest_x = center ?
                   rec->pos.x + rec->dim.x*0.5f :
                   rec->pos.x;
    if (level->editing_now || dest_x > cam->pos.x) {
        cam->pos.x = lerp(cam->pos.x,
                          dest_x,
                          level->editing_now ? 1.f :
                                               glob->state.delta_time *
                                                cam->speed_multiplier);
    }
}

void move_rider_to_plane(PR::Rider *rid, PR::Plane *p) {
    // NOTE: Making the rider stick to the plane
    rid->body.angle = p->body.angle;
    rid->body.pos.x =
        p->body.pos.x +
        (p->body.dim.x - rid->body.dim.x)*0.5f -
        (p->body.dim.y + rid->body.dim.y)*0.5f *
            sin(glm::radians(rid->body.angle)) -
        (p->body.dim.x*0.2f) *
            cos(glm::radians(rid->body.angle));
    rid->body.pos.y =
        p->body.pos.y +
        (p->body.dim.y - rid->body.dim.y)*0.5f -
        (p->body.dim.y + rid->body.dim.y)*0.5f *
            cos(glm::radians(rid->body.angle)) +
        (p->body.dim.x*0.2f) *
            sin(glm::radians(rid->body.angle));
}

// Particle systems

void create_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    if (ps->active) {
        PR::Plane *p = &glob->current_level.plane;
        particle->body.pos = p->body.pos + p->body.dim * 0.5f;
        particle->body.dim.x = 10.f;
        particle->body.dim.y = 10.f;
        particle->body.triangle = false;
        particle->color.r = 1.0f;
        particle->color.g = 1.0f;
        particle->color.b = 1.0f;
        particle->color.a = 1.0f;
        float movement_angle = glm::radians(p->body.angle);
        particle->vel.x =
            (glm::length(p->vel) - 400.f) * cos(movement_angle) +
            (float)((rand() % 101) - 50);
        particle->vel.y =
            -(glm::length(p->vel) - 400.f) * sin(movement_angle) +
            (float)((rand() % 101) - 50);
    } else {
        // NOTE: If the particle system is not active,
        //       the new particles should just delete the old ones
        particle->body.pos.x = 0.f;
        particle->body.pos.y = 0.f;
        particle->body.dim.x = 0.f;
        particle->body.dim.y = 0.f;
        particle->body.triangle = false;
        particle->vel.x = 0.f;
        particle->vel.y = 0.f;
        particle->color.r = 0.0f;
        particle->color.g = 0.0f;
        particle->color.b = 0.0f;
        particle->color.a = 0.0f;
    }
}
void update_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    float dt = glob->state.delta_time;
    particle->vel *= (1.f - dt); 
    particle->color.a -= particle->color.a * dt * 3.0f;
    particle->body.pos += particle->vel * dt;
}

void create_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    if (ps->active) {
        PR::Plane *p = &glob->current_level.plane;
        //particle->body.pos = p->body.pos + p->body.dim*0.5f;
        particle->body.dim.x = 15.f;
        particle->body.dim.y = 15.f;
        particle->body.pos = p->crash_position - particle->body.dim*0.5f;
        particle->body.angle = 0.f;
        particle->body.triangle = false;
        particle->vel.x = (float)((rand() % 301) - 150.f);
        particle->vel.y = -150.f + (float)((rand() % 131) - 130.f);
        particle->color.r = 1.0f;
        particle->color.g = 0.0f;
        particle->color.b = 0.0f;
        particle->color.a = 1.0f;
    } else {
        particle->body.pos.x = 0.f;
        particle->body.pos.y = 0.f;
        particle->body.dim.x = 0.f;
        particle->body.dim.y = 0.f;
        particle->body.triangle = false;
        particle->vel.x = 0.f;
        particle->vel.y = 0.f;
        particle->color.r = 0.0f;
        particle->color.g = 0.0f;
        particle->color.b = 0.0f;
        particle->color.a = 0.0f;
    }
}
void update_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos += particle->vel * dt;
    particle->body.angle -=
        glm::sign(particle->vel.x) *
        lerp(0.f, 720.f, glm::abs(particle->vel.x)/150.f) * dt;
}

void create_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    if (ps->active) {
        PR::Rider *rid = &glob->current_level.rider;
        //particle->body.pos = rid->body.pos + rid->body.dim*0.5f;
        particle->body.dim.x = 15.f;
        particle->body.dim.y = 15.f;
        particle->body.pos = rid->crash_position - particle->body.dim*0.5f;
        particle->body.angle = 0.f;
        particle->body.triangle = false;
        particle->vel.x = (float)((rand() % 301) - 150.f);
        particle->vel.y = -150.f + (float)((rand() % 131) - 130.f);
        particle->color.r = 0.0f;
        particle->color.g = 0.5f;
        particle->color.b = 0.5f;
        particle->color.a = 1.0f;
    } else {
        particle->body.pos.x = 0.f;
        particle->body.pos.y = 0.f;
        particle->body.dim.x = 0.f;
        particle->body.dim.y = 0.f;
        particle->body.angle = 0.f;
        particle->body.triangle = false;
        particle->vel.x = 0.f;
        particle->vel.y = 0.f;
        particle->color.r = 0.0f;
        particle->color.g = 0.0f;
        particle->color.b = 0.0f;
        particle->color.a = 0.0f;
    }
}
void update_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos += particle->vel * dt;
    particle->body.angle -=
        glm::sign(particle->vel.x) *
        lerp(0.f, 720.f, glm::abs(particle->vel.x)/150.f) * dt;
}
