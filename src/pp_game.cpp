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

// TODO:
//      - Proper gamepad support
//      - Update the README
//      - Alphabetic order of the levels
//      - Better way to signal boost direction in boost pads
//      - Find/make textures
//      - Fine tune angle change velocity
//      - Profiling

// TODO(fix):
//      - Fix collision when rider/plane are inverted

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

#define RESET_LEVEL_COLORS(level) do {\
    level->current_red = PR::RED;\
    level->current_blue = PR::BLUE;\
    level->current_gray = PR::GRAY;\
    level->current_white = PR::WHITE;\
} while(0)

#define DEACTIVATE_EDIT_MODE(level) do {\
    std::cout << "Deactivating edit mode!" << std::endl;\
    (level)->selected = NULL;\
    (level)->editing_now = false;\
    (level)->camera.pos.x = (level)->plane.body.pos.x;\
} while(0)

// Utilities functions for code reuse
inline void portal_render(PR::Portal *portal);
inline void boostpad_render(PR::BoostPad *pad);
inline void obstacle_render(PR::Obstacle *obs);
inline void portal_render_info(PR::Portal *portal, float tx, float ty);
inline void boostpad_render_info(PR::BoostPad *boost, float tx, float ty);
inline void obstacle_render_info(PR::Obstacle *obstacle, float tx, float ty);
inline void goal_line_render_info(Rect *rect, float tx, float ty);
inline void start_pos_render_info(Rect *rect, float tx, float ty);
inline void plane_update_animation(PR::Plane *p);
inline Rect *get_selected_body(void *selected, PR::ObjectType selected_type);
void set_portal_option_buttons(PR::LevelButton *buttons);
void set_boost_option_buttons(PR::LevelButton *buttons);
void set_obstacle_option_buttons(PR::LevelButton *buttons);
void set_start_pos_option_buttons(PR::LevelButton *buttons);
inline Rect rect_in_camera_space(Rect r, PR::Camera *cam);
inline TexCoords texcoords_in_texture_space(float x, float y,
                                            float w, float h,
                                            Texture *tex, bool inverse);

inline void apply_air_resistances(PR::Plane* p);
inline void lerp_camera_x_to_rect(PR::Camera *cam, Rect *rec, bool center);
inline void move_rider_to_plane(PR::Rider *rid, PR::Plane *p);

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
    menu->custom_buttons_number = 0;
    menu->custom_buttons = NULL;
    menu->custom_edit_buttons = NULL;
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
                       float *start_angle,
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

        std::fscanf(map_file, " %f %f %f", start_x, start_y, start_angle);
        if (std::ferror(map_file)) return_defer(1);
        *start_x = *start_x * proportion_x;
        *start_y = *start_y * proportion_y;

        std::cout << "[LOADING] player start position set to"
                  << " x: " << *start_x
                  << " y: " << *start_y
                  << " with angle of: " << *start_angle
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

        std::fprintf(map_file, "%zu\n", level->portals_number);
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

        // Goal line
        std::fprintf(map_file,
                     "%f\n",
                     level->goal_line.pos.x * inv_proportion_x);
        if (std::ferror(map_file)) return_defer(1);

        // Player start position
        std::fprintf(map_file,
                     "%f %f %f",
                     level->start_pos.pos.x * inv_proportion_x,
                     level->start_pos.pos.y * inv_proportion_y,
                     level->start_pos.angle);
        if (std::ferror(map_file)) return_defer(1);

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
                assert((std::strlen(dp->d_name)-std::strlen(extension)+1 <=
                            ARRAY_LENGTH(map_name)) &&
                        "File name bigger than temporary buffer");
                std::snprintf(map_name,
                             std::strlen(dp->d_name)-std::strlen(extension)+1,
                             "%s", dp->d_name);

                std::cout << "Found map_file: "
                          << map_name << " length: " << std::strlen(map_name)
                          << std::endl;

                size_t row = button_index / 3;
                size_t col = button_index % 3;

                char map_path[99] = "";
                assert((std::strlen(dir_path)+std::strlen(dp->d_name)+1 <=
                            ARRAY_LENGTH(map_path))
                        && "Map path too big for temporary buffer!");
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

                assert((std::strlen(map_name)+1 <= ARRAY_LENGTH(button->text))
                        && "Map name bigger than button text buffer!");
                std::snprintf(button->text, std::strlen(map_name)+1,
                              "%s", map_name);

                assert((std::strlen(map_path)+1 <=
                            ARRAY_LENGTH(button->mapfile_path))
                        && "Mapfile path bigger than button mapfile buffer!");
                std::snprintf(button->mapfile_path, std::strlen(map_path)+1,
                              "%s", map_path);

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
                std::snprintf(edit_button->text, std::strlen("EDIT")+1, "EDIT");

                assert((std::strlen(map_path)+1 <=
                            ARRAY_LENGTH(edit_button->mapfile_path))
                        && "Mapfile path bigger than button mapfile buffer!");
                std::snprintf(edit_button->mapfile_path,
                              std::strlen(map_path)+1, "%s", map_path);

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
    std::snprintf(campaign->text, strlen("CAMPAIGN")+1, "CAMPAIGN");

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
    std::snprintf(custom->text, strlen("CUSTOM")+1, "CUSTOM");

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

        int size = std::snprintf(nullptr, 0, "LEVEL %zu", levelbutton_index+1);
        assert((size+1 <= ARRAY_LENGTH(button->text))
                && "Text buffer is too little!");
        std::snprintf(button->text, size+1, "LEVEL %zu", levelbutton_index+1);
        if (levelbutton_index < ARRAY_LENGTH(campaign_levels_filepath)) {
            assert((std::strlen(campaign_levels_filepath[levelbutton_index]) <
                        ARRAY_LENGTH(button->mapfile_path))
                    && "Map file too big for button mapfile buffer!");
            std::snprintf(
                    button->mapfile_path,
                    std::strlen(campaign_levels_filepath[levelbutton_index])+1,
                    "%s", campaign_levels_filepath[levelbutton_index]);
        } else {
            std::snprintf(button->mapfile_path, 1, "");
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

    level->adding_now = false;

    level->selected = NULL;

    level->goal_line.pos.y = 0.f;
    level->goal_line.dim.x =  30.f;
    level->goal_line.dim.y = win->h;
    level->goal_line.triangle = false;
    level->goal_line.angle = 0.f;

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
    std::snprintf(level->file_path,
                  std::strlen(mapfile_path)+1,
                  "%s", mapfile_path);

    level->start_pos.dim = p->body.dim;
    level->start_pos.triangle = false;

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
                &level->start_pos.pos.x, &level->start_pos.pos.y,
                &level->start_pos.angle,
                &level->goal_line.pos.x,
                win->w, win->h);
        if (loading_result != 0) return loading_result;

        p->body.pos = level->start_pos.pos;
        p->body.angle = level->start_pos.angle;
        cam->pos.x = p->body.pos.x;

    } else {

        std::cout << "[WARNING] Loading fallback map information!"
                  << std::endl;

        // Hardcoding the fuck out of everything
        level->goal_line.pos.x = 1000.f;

        p->body.pos.x = 200.0f;
        p->body.pos.y = 350.0f;
        cam->pos.x = p->body.pos.x;

        level->start_pos.angle = 0.f;

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

    level->old_selected = level->selected;

    if (level->editing_available &&
        input->edit.clicked) {

        if (level->editing_now) {
            DEACTIVATE_EDIT_MODE(level);
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
            level->colors_shuffled = false;
            RESET_LEVEL_COLORS(level);
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
            if (level->selected) {
                Rect *b = get_selected_body(level->selected,
                                            level->selected_type);
                if (b) {
                    if (level->selected_type != PR::GOAL_LINE_TYPE) {
                        if (input->up.pressed) b->pos.y -= vely * dt;
                        if (input->down.pressed) b->pos.y += vely * dt;
                    }
                    if (input->left.pressed) b->pos.x -= velx * dt;
                    if (input->right.pressed) b->pos.x += velx * dt;
                } else {
                    std::cout << "Could not get object body of type: "
                              << level->selected_type
                              << std::endl;
                }
            } else {
                if (input->up.pressed) p->body.pos.y -= vely * dt;
                if (input->down.pressed) p->body.pos.y += vely * dt;
                if (input->left.pressed) p->body.pos.x -= velx * dt;
                if (input->right.pressed) p->body.pos.x += velx * dt;
            }
        } else { // PLAYING
            apply_air_resistances(p);

            // NOTE: Checking collision with boost_pads
            for (size_t boost_index = 0;
                 boost_index < boosts_number;
                 ++boost_index) {

                PR::BoostPad *pad = boosts + boost_index;

                if (rect_are_colliding(p->body, pad->body, NULL, NULL)) {

                    p->acc.x += pad->boost_power *
                                cos(glm::radians(pad->boost_angle));
                    p->acc.y += pad->boost_power *
                                -sin(glm::radians(pad->boost_angle));

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
                if (level->selected) { // FOCUS SELECTED OBJECT
                    Rect *b = get_selected_body(level->selected,
                                                level->selected_type);
                    lerp_camera_x_to_rect(cam, b, true);
                } else {
                    lerp_camera_x_to_rect(cam, &p->body, true);
                }
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
                if (rect_are_colliding(p->body, rid->body, NULL, NULL) &&
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


    if (!rid->crashed && !level->editing_now &&
        rid->body.pos.x + rid->body.dim.x*0.5f > level->goal_line.pos.x) {
        CHANGE_CASE_TO(PR::MENU, menu_prepare, "", false);
    }


    // NOTE: Update the `render_zone`s based on the `body`s
    p->render_zone.pos.x = p->body.pos.x;
    p->render_zone.pos.y = p->inverse ? p->body.pos.y+p->body.dim.y :
                                        p->body.pos.y;
    p->render_zone.angle = p->body.angle;

    rid->render_zone.pos = rid->body.pos +
                           (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->render_zone.angle = rid->body.angle;

    plane_update_animation(p);

    // NOTE: If you click the left mouse button when you have something
    //          selected and you are not clicking on an option
    //          button, the object will be deselected
    bool set_selected_to_null = false;
    if (input->mouse_left.clicked && level->selected) {
        set_selected_to_null = true;
    }

    if (level->editing_now) { // EDITING


        for(size_t portal_index = 0;
            portal_index < portals_number;
            ++portal_index) {

            PR::Portal *portal = portals + portal_index;

            if (input->mouse_left.clicked &&
                level->selected == NULL &&
                !level->adding_now &&
                rect_contains_point(rect_in_camera_space(portal->body, cam),
                                    input->mouseX, input->mouseY, false)) {

                level->selected = (void *) portal;
                level->selected_type = PR::PORTAL_TYPE;
                level->adding_now = false;

                set_portal_option_buttons(level->selected_options_buttons);
            }


            portal_render(portal);

        }

        for (size_t boost_index = 0;
             boost_index < boosts_number;
             ++boost_index) {

            PR::BoostPad *pad = boosts + boost_index;

            if (input->mouse_left.clicked &&
                level->selected == NULL &&
                !level->adding_now &&
                rect_contains_point(rect_in_camera_space(pad->body, cam),
                                    input->mouseX, input->mouseY, false)) {

                level->selected = (void *) pad;
                level->selected_type = PR::BOOST_TYPE;
                level->adding_now = false;

                set_boost_option_buttons(level->selected_options_buttons);
            }

            boostpad_render(pad);
        }

        for (int obstacle_index = 0;
             obstacle_index < obstacles_number;
             obstacle_index++) {

            PR::Obstacle *obs = obstacles + obstacle_index;

            if (input->mouse_left.clicked &&
                level->selected == NULL &&
                !level->adding_now &&
                rect_contains_point(rect_in_camera_space(obs->body, cam),
                                    input->mouseX, input->mouseY, false)) {

                level->selected = (void *) obs;
                level->selected_type = PR::OBSTACLE_TYPE;
                level->adding_now = false;

                set_obstacle_option_buttons(level->selected_options_buttons);
            }

            obstacle_render(obs);

        }

        if (input->mouse_left.clicked &&
            level->selected == NULL &&
            !level->adding_now &&
            rect_contains_point(rect_in_camera_space(level->goal_line, cam),
                                input->mouseX, input->mouseY, false)) {

            level->selected = (void *) &level->goal_line;
            level->selected_type = PR::GOAL_LINE_TYPE;
            level->adding_now = false;

        }

        if (input->mouse_left.clicked &&
            level->selected == NULL &&
            !level->adding_now &&
            rect_contains_point(rect_in_camera_space(level->start_pos, cam),
                                input->mouseX, input->mouseY, false)) {

            level->selected = (void *) &level->start_pos;
            level->selected_type = PR::P_START_POS_TYPE;
            level->adding_now = false;

            set_start_pos_option_buttons(level->selected_options_buttons);
        }
        renderer_add_queue_uni(rect_in_camera_space(level->start_pos, cam),
                               glm::vec4(0.9f, 0.3f, 0.7f, 1.f), false);

        if (input->obj_add.clicked) {
            level->adding_now = true;
            level->selected = NULL;
        }

        if (level->selected == NULL && input->reset_pos.clicked) {
            p->body.pos = level->start_pos.pos;
            p->body.angle = level->start_pos.angle;
        }

        // NOTE: Loop over window edged pacman style,
        //       but only on the top and bottom
        if (p->body.pos.y + p->body.dim.y/2 > win->h) {
            p->body.pos.y -= win->h;
        }
        if (p->body.pos.y + p->body.dim.y/2 < 0) {
            p->body.pos.y += win->h;
        }
        
    } else { // PLAYING
        // NOTE: The portal can be activated only by the rider.
        //       If the rider is attached, then, by extensions, also
        //          the plane will activate the portal.
        //       Even if the rider is not attached, the effect is also
        //          applied to the plane.
        for(size_t portal_index = 0;
            portal_index < portals_number;
            ++portal_index) {

            PR::Portal *portal = portals + portal_index;

            portal_render(portal);

            if (!rid->crashed &&
                (rect_are_colliding(rid->body, portal->body, NULL, NULL) ||
                 (rid->attached && rect_are_colliding(p->body,
                                                      portal->body,
                                                      NULL, NULL)))) {
                // std::cout << "------------------------"
                //           << "\nCollided with portal"
                //           << std::endl;
                switch(portal->type) {
                    case PR::INVERSE:
                        // NOTE: Skip if the plane/rider already has the effect
                        // std::cout << "-------------------------"
                        //           << "\np->inv: " << p->inverse
                        //           << "\nrid->inv: " << rid->inverse
                        //           << std::endl;
                        if (p->inverse != portal->enable_effect &&
                            rid->inverse != portal->enable_effect) {

                            if (!p->crashed) {
                                p->inverse = portal->enable_effect;
                                p->body.pos.y += p->body.dim.y;
                                p->body.dim.y = -p->body.dim.y;
                            }
                            rid->inverse = portal->enable_effect;
                            rid->body.dim.y = -rid->body.dim.y;
                        }
                        break;
                    case PR::SHUFFLE_COLORS:
                        // std::cout << "--------------------"
                        //           << "\nrid.x: " << rid->body.pos.x
                        //           << "\npor.x: " << portal->body.pos.x
                        //           << "\ncp: " << rect_are_colliding(&p->body, &portal->body, NULL, NULL)
                        //           << "\ncr: " << rect_are_colliding(&rid->body, &portal->body, NULL, NULL)
                        //           << std::endl;
                        if (level->colors_shuffled != portal->enable_effect) {

                            level->colors_shuffled = portal->enable_effect;

                            if (portal->enable_effect) {
                                level->current_red = PR::WHITE;
                                level->current_blue = PR::RED;
                                level->current_gray = PR::BLUE;
                                level->current_white = PR::GRAY;
                            } else {
                                RESET_LEVEL_COLORS(level);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // NOTE: Render the boosts
        for (size_t boost_index = 0;
             boost_index < boosts_number;
             ++boost_index) {

            PR::BoostPad *pad = boosts + boost_index;
            boostpad_render(pad);
        }

        // NOTE: Checking collision with obstacles
        for (int obstacle_index = 0;
             obstacle_index < obstacles_number;
             obstacle_index++) {

            PR::Obstacle *obs = obstacles + obstacle_index;

            obstacle_render(obs);

            if (!p->crashed && obs->collide_plane &&
                rect_are_colliding(p->body, obs->body,
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
                std::cout << "Plane collided with "
                          << obstacle_index << std::endl;
            }
            if (!rid->crashed && obs->collide_rider &&
                rect_are_colliding(rid->body, obs->body,
                                   &rid->crash_position.x,
                                   &rid->crash_position.y)) {
                // NOTE: Rider colliding with an obstacle

                rid->crashed = true;
                rid->attached = false;
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;

                // TODO: Debug flag
                std::cout << "Rider collided with "
                          << obstacle_index << std::endl;
            }
        }

        // NOTE: Collide with the ceiling and the floor
        Rect ceiling;
        ceiling.pos.x = -((float) win->w);
        ceiling.pos.y = -((float) win->h);
        ceiling.dim.x = win->w*3.f;
        ceiling.dim.y = win->h;
        ceiling.angle = 0.f;
        ceiling.triangle = false;
        Rect floor;
        floor.pos.x = -((float) win->w);
        floor.pos.y = (float) win->h;
        floor.dim.x = win->w*3.f;
        floor.dim.y = win->h;
        floor.angle = 0.f;
        floor.triangle = false;

        // NOTE: Collisions with the ceiling
        if (!p->crashed &&
            rect_are_colliding(rect_in_camera_space(p->body, cam), ceiling,
                               &p->crash_position.x,
                               &p->crash_position.y)) {

            p->crash_position +=
                cam->pos - glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);

            if (rid->attached) {
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;
            }
            p->crashed = true;
            p->acc *= 0.f;
            p->vel *= 0.f;

            // TODO: Debug flag
            std::cout << "Plane collided with the ceiling" << std::endl;
        }
        if (!rid->crashed &&
            rect_are_colliding(rect_in_camera_space(rid->body, cam), ceiling,
                               &rid->crash_position.x,
                               &rid->crash_position.y)) {

            rid->crash_position +=
                cam->pos - glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);

            rid->crashed = true;
            rid->attached = false;
            rid->vel *= 0.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;

            // TODO: Debug flag
            std::cout << "Rider collided with the ceiling" << std::endl;
        }

        // NOTE: Collisions with the floor
        if (!p->crashed &&
            rect_are_colliding(rect_in_camera_space(p->body, cam), floor,
                               &p->crash_position.x,
                               &p->crash_position.y)) {

            p->crash_position +=
                cam->pos - glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);

            if (rid->attached) {
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;
            }
            p->crashed = true;
            p->acc *= 0.f;
            p->vel *= 0.f;

            // TODO: Debug flag
            std::cout << "Plane collided with the floor" << std::endl;
        }
        if (!rid->crashed &&
            rect_are_colliding(rect_in_camera_space(rid->body, cam), floor,
                               &rid->crash_position.x,
                               &rid->crash_position.y)) {

            rid->crash_position +=
                cam->pos - glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);

            rid->crashed = true;
            rid->attached = false;
            rid->vel *= 0.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;

            // TODO: Debug flag
            std::cout << "Rider collided with the floor" << std::endl;
        }
    }

    // NOTE: Updating and rendering all the particle systems
    // Set the time_between_particles for the boost based on the velocity
    glob->current_level.particle_systems[0].time_between_particles =
        lerp(0.02f, 0.01f, glm::length(p->vel)/PLANE_VELOCITY_LIMIT);
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

    // NOTE: Rendering the plane
    renderer_add_queue_uni(rect_in_camera_space(p->body, cam),
                           glm::vec4(1.0f, 1.0f, 1.0f, 1.f),
                           false);

    // NOTE: Rendering plane texture
    renderer_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                           texcoords_in_texture_space(
                                p->current_animation * 32.f, 0.f,
                                32.f, 8.f,
                                &glob->rend_res.global_sprite, p->inverse),
                           false);

    // NOTE: Rendering the rider
    renderer_add_queue_uni(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);

    // NOTE: Rendering goal line
    renderer_add_queue_uni(rect_in_camera_space(level->goal_line, cam),
                           glm::vec4(1.0f), false);

    // Actually issuing the render calls
    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_tex(glob->rend_res.shaders[1],
                      &glob->rend_res.global_sprite);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    if (level->adding_now) {
        PR::LevelButton add_portal;
        add_portal.from_center = true;
        add_portal.body.pos.x = win->w * 0.25f;
        add_portal.body.pos.y = win->h * 0.5f;
        add_portal.body.dim.x = win->w * 0.2f;
        add_portal.body.dim.y = win->h * 0.3f;
        add_portal.body.triangle = false;
        add_portal.body.angle = 0.f;
        add_portal.col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        std::snprintf(add_portal.text,
                      std::strlen("ADD PORTAL")+1,
                      "ADD PORTAL");

        PR::LevelButton add_boost;
        add_boost.from_center = true;
        add_boost.body.pos.x = win->w * 0.5f;
        add_boost.body.pos.y = win->h * 0.5f;
        add_boost.body.dim.x = win->w * 0.2f;
        add_boost.body.dim.y = win->h * 0.3f;
        add_boost.body.triangle = false;
        add_boost.body.angle = 0.f;
        add_boost.col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        std::snprintf(add_boost.text,
                      std::strlen("ADD BOOST")+1,
                      "ADD BOOST");

        PR::LevelButton add_obstacle;
        add_obstacle.from_center = true;
        add_obstacle.body.pos.x = win->w * 0.75f;
        add_obstacle.body.pos.y = win->h * 0.5f;
        add_obstacle.body.dim.x = win->w * 0.2f;
        add_obstacle.body.dim.y = win->h * 0.3f;
        add_obstacle.body.triangle = false;
        add_obstacle.body.angle = 0.f;
        add_obstacle.col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        std::snprintf(add_obstacle.text,
                      std::strlen("ADD OBSTACLE")+1,
                      "ADD OBSTACLE");

        if (input->mouse_left.clicked) {
            level->adding_now = false;
            if (rect_contains_point(add_portal.body,
                                input->mouseX, input->mouseY,
                                add_portal.from_center)) {

                level->portals_number++;
                level->portals = (PR::Portal *)
                    std::realloc((void *) level->portals,
                                 sizeof(PR::Portal) * level->portals_number);
                PR::Portal *portal =
                    level->portals + (level->portals_number - 1);
                portal->body.pos = cam->pos;
                portal->body.dim.x = win->w * 0.2f;
                portal->body.dim.y = win->h * 0.2f;
                portal->body.triangle = false;
                portal->body.angle = 0.f;
                portal->type = PR::SHUFFLE_COLORS;
                portal->enable_effect = true;

                level->selected = (void *) portal;
                level->selected_type = PR::PORTAL_TYPE;
                set_portal_option_buttons(level->selected_options_buttons);

            } else
            if (rect_contains_point(add_boost.body,
                                    input->mouseX, input->mouseY,
                                    add_boost.from_center)) {

                level->boosts_number++;
                level->boosts = (PR::BoostPad *)
                    std::realloc((void *) level->boosts,
                                 sizeof(PR::BoostPad) * level->boosts_number);
                PR::BoostPad *pad =
                    level->boosts + (level->boosts_number - 1);
                pad->body.pos = cam->pos;
                pad->body.dim.x = win->w * 0.2f;
                pad->body.dim.y = win->h * 0.2f;
                pad->body.triangle = false;
                pad->body.angle = 0.f;
                pad->boost_angle = 0.f;
                pad->boost_power = 0.f;

                level->selected = (void *) pad;
                level->selected_type = PR::BOOST_TYPE;
                set_boost_option_buttons(level->selected_options_buttons);

            } else
            if (rect_contains_point(add_obstacle.body,
                                    input->mouseX, input->mouseY,
                                    add_obstacle.from_center)) {

                level->obstacles_number++;
                level->obstacles = (PR::Obstacle *)
                    std::realloc((void *) level->obstacles,
                                 sizeof(PR::Obstacle) *
                                    level->obstacles_number);
                PR::Obstacle *obs =
                    level->obstacles+(level->obstacles_number - 1);
                obs->body.pos = cam->pos;
                obs->body.dim.x = win->w * 0.2f;
                obs->body.dim.y = win->h * 0.2f;
                obs->body.triangle = false;
                obs->body.angle = 0.f;
                obs->collide_plane = false;
                obs->collide_rider = false;

                level->selected = (void *) obs;
                level->selected_type = PR::OBSTACLE_TYPE;
                set_obstacle_option_buttons(level->selected_options_buttons);

            }
        }

        renderer_add_queue_uni(add_portal.body,
                               add_portal.col,
                               add_portal.from_center);
        // TODO: Selected an appropriate font for this
        renderer_add_queue_text(add_portal.body.pos.x,
                                add_portal.body.pos.y,
                                add_portal.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[1], true);

        renderer_add_queue_uni(add_boost.body,
                               add_boost.col,
                               add_boost.from_center);
        // TODO: Selected an appropriate font for this
        renderer_add_queue_text(add_boost.body.pos.x,
                                add_boost.body.pos.y,
                                add_boost.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[1], true);

        renderer_add_queue_uni(add_obstacle.body,
                               add_obstacle.col,
                               add_obstacle.from_center);
        // TODO: Selected an appropriate font for this
        renderer_add_queue_text(add_obstacle.body.pos.x,
                                add_obstacle.body.pos.y,
                                add_obstacle.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[1], true);

        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                           glob->rend_res.shaders[2]);
    }

    if (level->selected && input->obj_delete.clicked) {
        switch(level->selected_type) {
            case PR::PORTAL_TYPE:
            {
                PR::Portal *portal = (PR::Portal *) level->selected;

                int index = portal - level->portals;

                std::memmove((void *)(level->portals + index),
                             (void *)(level->portals + index + 1),
                             (size_t)(level->portals_number - index - 1) *
                                sizeof(PR::Portal));

                level->portals_number--;
                level->portals = (PR::Portal *)
                    std::realloc((void *) level->portals,
                                 sizeof(PR::Portal) *
                                    level->portals_number);

                std::cout << "Removed portal n. " << index << std::endl;
                
                level->selected = NULL;

                break;
            }
            case PR::BOOST_TYPE:
            {
                PR::BoostPad *pad = (PR::BoostPad *) level->selected;

                int index = pad - level->boosts;

                std::memmove((void *)(level->boosts + index),
                             (void *)(level->boosts + index + 1),
                             (size_t)(level->boosts_number - index - 1) *
                                sizeof(PR::BoostPad));

                level->boosts_number--;
                level->boosts = (PR::BoostPad *)
                    std::realloc((void *) level->boosts,
                                 sizeof(PR::BoostPad) *
                                    level->boosts_number);

                std::cout << "Removed boost n. " << index << std::endl;
                
                level->selected = NULL;

                break;
            }
            case PR::OBSTACLE_TYPE:
            {
                PR::Obstacle *obs = (PR::Obstacle *) level->selected;

                int index = obs - level->obstacles;

                std::memmove((void *)(level->obstacles + index),
                             (void *)(level->obstacles + index + 1),
                             (size_t)(level->obstacles_number - index - 1) *
                                sizeof(PR::Obstacle));

                level->obstacles_number--;
                level->obstacles = (PR::Obstacle *)
                    std::realloc((void *) level->obstacles,
                                 sizeof(PR::Obstacle) *
                                    level->obstacles_number);

                std::cout << "Removed obstacle n. " << index << std::endl;

                level->selected = NULL;

                break;
            }
            default:
                break;
        }
    }

    // NOTE: This check is done so that if when an obstacle is selected and
    //          a button (to modify the selected object) appears on the cursor
    //          position, that button is not pressed automatically
    if (level->selected != NULL &&
        level->selected == level->old_selected &&
        !level->adding_now && !input->obj_delete.clicked) {
        switch(level->selected_type) {
            case PR::PORTAL_TYPE:
            {
                PR::Portal *portal = (PR::Portal *) level->selected;
                portal_render_info(portal, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);

                // Render and check input on selected options
                for(size_t option_button_index = 0;
                    option_button_index < SELECTED_PORTAL_OPTIONS;
                    ++option_button_index) {
                    assert((option_button_index <
                                ARRAY_LENGTH(level->selected_options_buttons))
                            && "Selected options out of bound for portals");

                    PR::LevelButton *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index <= 1) {
                        PR::LevelButton minus1;
                        minus1.from_center = true;
                        minus1.body.angle = 0.f;
                        minus1.body.triangle = false;
                        minus1.body.dim.x = button->body.dim.x * 0.2f;
                        minus1.body.dim.y = minus1.body.dim.x;
                        minus1.body.pos.x = button->body.pos.x -
                                            button->body.dim.x * 0.5f +
                                            minus1.body.dim.x * 0.5f;
                        minus1.body.pos.y = button->body.pos.y -
                                            button->body.dim.y * 0.5f;
                        minus1.col = button->col * 0.5f;
                        minus1.col.a = 1.f;

                        PR::LevelButton minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;

                        PR::LevelButton plus1;
                        plus1.from_center = true;
                        plus1.body.angle = 0.f;
                        plus1.body.triangle = false;
                        plus1.body.dim = minus1.body.dim;
                        plus1.body.pos.x = button->body.pos.x +
                                           button->body.dim.x * 0.5f -
                                           plus1.body.dim.x * 0.5f;
                        plus1.body.pos.y = button->body.pos.y -
                                           button->body.dim.y * 0.5f;
                        plus1.col = button->col * 0.5f;
                        plus1.col.a = 1.f;

                        PR::LevelButton plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;


                        if (input->mouse_left.clicked) {
                            if (rect_contains_point(button->body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                            }
                            if (rect_contains_point(plus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        portal->body.dim.x += 1.f;
                                        break;
                                    case 1:
                                        portal->body.dim.y += 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(plus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        portal->body.dim.x += 5.f;
                                        break;
                                    case 1:
                                        portal->body.dim.y += 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        portal->body.dim.x -= 1.f;
                                        break;
                                    case 1:
                                        portal->body.dim.y -= 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        portal->body.dim.x -= 5.f;
                                        break;
                                    case 1:
                                        portal->body.dim.y -= 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus1.body,
                                               plus1.col,
                                               plus1.from_center);
                        renderer_add_queue_text(plus1.body.pos.x,
                                                plus1.body.pos.y,
                                                "+1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus5.body,
                                               plus5.col,
                                               plus5.from_center);
                        renderer_add_queue_text(plus5.body.pos.x,
                                                plus5.body.pos.y,
                                                "+5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus1.body,
                                               minus1.col,
                                               minus1.from_center);
                        renderer_add_queue_text(minus1.body.pos.x,
                                                minus1.body.pos.y,
                                                "-1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus5.body,
                                               minus5.col,
                                               minus5.from_center);
                        renderer_add_queue_text(minus5.body.pos.x,
                                                minus5.body.pos.y,
                                                "-5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    } else {

                        if (input->mouse_left.clicked &&
                            rect_contains_point(button->body,
                                                input->mouseX,
                                                input->mouseY, true)) {
                            // Something useful was indeed clicked,
                            // so don't reset the seleciton
                            set_selected_to_null = false;

                            switch (option_button_index) {
                                case 2:
                                    if (portal->type == PR::INVERSE) {
                                        portal->type = PR::SHUFFLE_COLORS;
                                    } else {
                                        portal->type = PR::INVERSE;
                                    }
                                    break;
                                case 3:
                                    portal->enable_effect =
                                        !portal->enable_effect;
                                    break;
                                default:
                                    break;
                            }
                        }

                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    }

                }
                break;
            }
            case PR::BOOST_TYPE:
            {
                PR::BoostPad *pad = (PR::BoostPad *) level->selected;
                boostpad_render_info(pad, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);

                // Render and check input on selected options
                for(size_t option_button_index = 0;
                    option_button_index < SELECTED_BOOST_OPTIONS;
                    ++option_button_index) {
                    assert((option_button_index <
                                ARRAY_LENGTH(level->selected_options_buttons))
                            && "Selected options out of bound for boosts");

                    PR::LevelButton *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index == 3) {
                        if (input->mouse_left.clicked &&
                            rect_contains_point(button->body,
                                                input->mouseX,
                                                input->mouseY, true)) {
                            // Something useful was indeed clicked,
                            // so don't reset the seleciton
                            set_selected_to_null = false;

                            switch (option_button_index) {
                                case 3:
                                    pad->body.triangle = !pad->body.triangle;
                                    break;
                                default:
                                    break;
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    } else {
                        PR::LevelButton minus1;
                        minus1.from_center = true;
                        minus1.body.angle = 0.f;
                        minus1.body.triangle = false;
                        minus1.body.dim.x = button->body.dim.x * 0.2f;
                        minus1.body.dim.y = minus1.body.dim.x;
                        minus1.body.pos.x = button->body.pos.x -
                                            button->body.dim.x * 0.5f +
                                            minus1.body.dim.x * 0.5f;
                        minus1.body.pos.y = button->body.pos.y -
                                            button->body.dim.y * 0.5f;
                        minus1.col = button->col * 0.5f;
                        minus1.col.a = 1.f;

                        PR::LevelButton minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;

                        PR::LevelButton plus1;
                        plus1.from_center = true;
                        plus1.body.angle = 0.f;
                        plus1.body.triangle = false;
                        plus1.body.dim = minus1.body.dim;
                        plus1.body.pos.x = button->body.pos.x +
                                           button->body.dim.x * 0.5f -
                                           plus1.body.dim.x * 0.5f;
                        plus1.body.pos.y = button->body.pos.y -
                                           button->body.dim.y * 0.5f;
                        plus1.col = button->col * 0.5f;
                        plus1.col.a = 1.f;

                        PR::LevelButton plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;

                        if (input->mouse_left.clicked) {
                            if (rect_contains_point(button->body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                            }
                            if (rect_contains_point(plus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        pad->body.dim.x += 1.f;
                                        break;
                                    case 1:
                                        pad->body.dim.y += 1.f;
                                        break;
                                    case 2:
                                        pad->body.angle += 1.f;
                                        break;
                                    case 4:
                                        pad->boost_angle += 1.f;
                                        break;
                                    case 5:
                                        pad->boost_power += 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(plus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        pad->body.dim.x += 5.f;
                                        break;
                                    case 1:
                                        pad->body.dim.y += 5.f;
                                        break;
                                    case 2:
                                        pad->body.angle += 5.f;
                                        break;
                                    case 4:
                                        pad->boost_angle += 5.f;
                                        break;
                                    case 5:
                                        pad->boost_power += 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        pad->body.dim.x -= 1.f;
                                        break;
                                    case 1:
                                        pad->body.dim.y -= 1.f;
                                        break;
                                    case 2:
                                        pad->body.angle -= 1.f;
                                        break;
                                    case 4:
                                        pad->boost_angle -= 1.f;
                                        break;
                                    case 5:
                                        pad->boost_power -= 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        pad->body.dim.x -= 5.f;
                                        break;
                                    case 1:
                                        pad->body.dim.y -= 5.f;
                                        break;
                                    case 2:
                                        pad->body.angle -= 5.f;
                                        break;
                                    case 4:
                                        pad->boost_angle -= 5.f;
                                        break;
                                    case 5:
                                        pad->boost_power -= 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus1.body,
                                               plus1.col,
                                               plus1.from_center);
                        renderer_add_queue_text(plus1.body.pos.x,
                                                plus1.body.pos.y,
                                                "+1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus5.body,
                                               plus5.col,
                                               plus5.from_center);
                        renderer_add_queue_text(plus5.body.pos.x,
                                                plus5.body.pos.y,
                                                "+5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus1.body,
                                               minus1.col,
                                               minus1.from_center);
                        renderer_add_queue_text(minus1.body.pos.x,
                                                minus1.body.pos.y,
                                                "-1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus5.body,
                                               minus5.col,
                                               minus5.from_center);
                        renderer_add_queue_text(minus5.body.pos.x,
                                                minus5.body.pos.y,
                                                "-5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    }


                }

                break;
            }
            case PR::OBSTACLE_TYPE:
            {
                PR::Obstacle *obs = (PR::Obstacle *) level->selected;
                obstacle_render_info(obs, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);

                for(size_t option_button_index = 0;
                    option_button_index < SELECTED_OBSTACLE_OPTIONS;
                    ++option_button_index) {
                    assert((option_button_index <
                                ARRAY_LENGTH(level->selected_options_buttons))
                            && "Selected options out of bound for obstacles");

                    PR::LevelButton *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index <= 2) {
                        PR::LevelButton minus1;
                        minus1.from_center = true;
                        minus1.body.angle = 0.f;
                        minus1.body.triangle = false;
                        minus1.body.dim.x = button->body.dim.x * 0.2f;
                        minus1.body.dim.y = minus1.body.dim.x;
                        minus1.body.pos.x = button->body.pos.x -
                                            button->body.dim.x * 0.5f +
                                            minus1.body.dim.x * 0.5f;
                        minus1.body.pos.y = button->body.pos.y -
                                            button->body.dim.y * 0.5f;
                        minus1.col = button->col * 0.5f;
                        minus1.col.a = 1.f;

                        PR::LevelButton minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;

                        PR::LevelButton plus1;
                        plus1.from_center = true;
                        plus1.body.angle = 0.f;
                        plus1.body.triangle = false;
                        plus1.body.dim = minus1.body.dim;
                        plus1.body.pos.x = button->body.pos.x +
                                           button->body.dim.x * 0.5f -
                                           plus1.body.dim.x * 0.5f;
                        plus1.body.pos.y = button->body.pos.y -
                                           button->body.dim.y * 0.5f;
                        plus1.col = button->col * 0.5f;
                        plus1.col.a = 1.f;

                        PR::LevelButton plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;


                        if (input->mouse_left.clicked) {
                            if (rect_contains_point(button->body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                            }
                            if (rect_contains_point(plus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        obs->body.dim.x += 1.f;
                                        break;
                                    case 1:
                                        obs->body.dim.y += 1.f;
                                        break;
                                    case 2:
                                        obs->body.angle += 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(plus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        obs->body.dim.x += 5.f;
                                        break;
                                    case 1:
                                        obs->body.dim.y += 5.f;
                                        break;
                                    case 2:
                                        obs->body.angle += 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        obs->body.dim.x -= 1.f;
                                        break;
                                    case 1:
                                        obs->body.dim.y -= 1.f;
                                        break;
                                    case 2:
                                        obs->body.angle -= 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        obs->body.dim.x -= 5.f;
                                        break;
                                    case 1:
                                        obs->body.dim.y -= 5.f;
                                        break;
                                    case 2:
                                        obs->body.angle -= 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus1.body,
                                               plus1.col,
                                               plus1.from_center);
                        renderer_add_queue_text(plus1.body.pos.x,
                                                plus1.body.pos.y,
                                                "+1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus5.body,
                                               plus5.col,
                                               plus5.from_center);
                        renderer_add_queue_text(plus5.body.pos.x,
                                                plus5.body.pos.y,
                                                "+5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus1.body,
                                               minus1.col,
                                               minus1.from_center);
                        renderer_add_queue_text(minus1.body.pos.x,
                                                minus1.body.pos.y,
                                                "-1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus5.body,
                                               minus5.col,
                                               minus5.from_center);
                        renderer_add_queue_text(minus5.body.pos.x,
                                                minus5.body.pos.y,
                                                "-5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);

                    } else {

                        if (input->mouse_left.clicked &&
                            rect_contains_point(button->body,
                                                input->mouseX,
                                                input->mouseY, true)) {
                            // Something useful was indeed clicked,
                            // so don't reset the seleciton
                            set_selected_to_null = false;

                            switch (option_button_index) {
                                case 3:
                                    obs->body.triangle = !obs->body.triangle;
                                    break;
                                case 4:
                                    obs->collide_plane = !obs->collide_plane;
                                    break;
                                case 5:
                                    obs->collide_rider = !obs->collide_rider;
                                    break;
                                default:
                                    break;
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    }


                }

                break;
            }
            case PR::GOAL_LINE_TYPE:
            {
                Rect *rect = (Rect *) level->selected;
                goal_line_render_info(rect, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);
                break;
            }
            case PR::P_START_POS_TYPE:
            {
                Rect *rect = (Rect *) level->selected;
                start_pos_render_info(rect, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);

                for(size_t option_button_index = 0;
                    option_button_index < SELECTED_START_POS_OPTIONS;
                    ++option_button_index) {
                    assert((option_button_index <
                                ARRAY_LENGTH(level->selected_options_buttons))
                            && "Selected options out of bound for obstacles");

                    PR::LevelButton *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index == 0) {
                        PR::LevelButton minus1;
                        minus1.from_center = true;
                        minus1.body.angle = 0.f;
                        minus1.body.triangle = false;
                        minus1.body.dim.x = button->body.dim.x * 0.2f;
                        minus1.body.dim.y = minus1.body.dim.x;
                        minus1.body.pos.x = button->body.pos.x -
                                            button->body.dim.x * 0.5f +
                                            minus1.body.dim.x * 0.5f;
                        minus1.body.pos.y = button->body.pos.y -
                                            button->body.dim.y * 0.5f;
                        minus1.col = button->col * 0.5f;
                        minus1.col.a = 1.f;

                        PR::LevelButton minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;

                        PR::LevelButton plus1;
                        plus1.from_center = true;
                        plus1.body.angle = 0.f;
                        plus1.body.triangle = false;
                        plus1.body.dim = minus1.body.dim;
                        plus1.body.pos.x = button->body.pos.x +
                                           button->body.dim.x * 0.5f -
                                           plus1.body.dim.x * 0.5f;
                        plus1.body.pos.y = button->body.pos.y -
                                           button->body.dim.y * 0.5f;
                        plus1.col = button->col * 0.5f;
                        plus1.col.a = 1.f;

                        PR::LevelButton plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;


                        if (input->mouse_left.clicked) {
                            if (rect_contains_point(button->body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                            }
                            if (rect_contains_point(plus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        rect->angle += 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(plus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        rect->angle += 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus1.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        rect->angle -= 1.f;
                                        break;
                                    default:
                                        break;
                                }
                            } else
                            if (rect_contains_point(minus5.body,
                                                    input->mouseX,
                                                    input->mouseY, true)) {
                                set_selected_to_null = false;
                                switch(option_button_index) {
                                    case 0:
                                        rect->angle -= 5.f;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        renderer_add_queue_uni(button->body,
                                               button->col,
                                               button->from_center);
                        // TODO: Selected an appropriate font for this
                        renderer_add_queue_text(button->body.pos.x,
                                                button->body.pos.y,
                                                button->text, glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus1.body,
                                               plus1.col,
                                               plus1.from_center);
                        renderer_add_queue_text(plus1.body.pos.x,
                                                plus1.body.pos.y,
                                                "+1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(plus5.body,
                                               plus5.col,
                                               plus5.from_center);
                        renderer_add_queue_text(plus5.body.pos.x,
                                                plus5.body.pos.y,
                                                "+5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus1.body,
                                               minus1.col,
                                               minus1.from_center);
                        renderer_add_queue_text(minus1.body.pos.x,
                                                minus1.body.pos.y,
                                                "-1", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                        renderer_add_queue_uni(minus5.body,
                                               minus5.col,
                                               minus5.from_center);
                        renderer_add_queue_text(minus5.body.pos.x,
                                                minus5.body.pos.y,
                                                "-5", glm::vec4(1.0f),
                                                &glob->rend_res.fonts[1], true);
                    } else {
                        // UNREACHABLE
                    }
                }

                break;
            }
        }
        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                           glob->rend_res.shaders[2]);
    }

    if (set_selected_to_null) level->selected = NULL;

    if (input->save_map.clicked) {
        if (save_map_to_file(level->file_path, level, win->w, win->h)) {
            std::cout << "[ERROR] Could not save the map in the file: "
                      << level->file_path << std::endl;
        } else {
            std::cout << "Saved current level to file: "
                      << level->file_path << std::endl;
        }
    }
}

// Utilities
inline void plane_update_animation(PR::Plane *p) {
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
        p->animation_countdown -= glob->state.delta_time;
    }
}

inline void portal_render(PR::Portal *portal) {
    PR::Camera *cam = &glob->current_level.camera;
    //PR::WinInfo *win = &glob->window;
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

inline void boostpad_render(PR::BoostPad *pad) {
    PR::Camera *cam = &glob->current_level.camera;
    PR::WinInfo *win = &glob->window;

    Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

    if (pad_in_cam_pos.pos.x < -((float)win->w) ||
        pad_in_cam_pos.pos.x > win->w * 2.f) return;

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

inline void obstacle_render(PR::Obstacle *obs) {
    PR::Camera *cam = &glob->current_level.camera;
    PR::WinInfo *win = &glob->window;
    Rect obs_in_cam_pos = rect_in_camera_space(obs->body, cam);

    if (obs_in_cam_pos.pos.x < -((float)win->w) ||
        obs_in_cam_pos.pos.x > win->w * 2.f) return;

    renderer_add_queue_uni(obs_in_cam_pos,
                          get_obstacle_color(obs),
                          false);
}

inline void portal_render_info(PR::Portal *portal, float tx, float ty) {
    char buffer[99];
    std::memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    std::sprintf(buffer, "PORTAL INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "pos: (%f, %f)",
                 (portal)->body.pos.x, (portal)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "dim: (%f, %f)",
                 (portal)->body.dim.x, (portal)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "type: %s",
                 get_portal_type_name((portal)->type));
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "enable_effect: %s",
                 (portal)->enable_effect ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

inline void boostpad_render_info(PR::BoostPad *boost, float tx, float ty) {
    char buffer[99];
    std::memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    std::sprintf(buffer, "BOOST INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "pos: (%f, %f)",
                 (boost)->body.pos.x, (boost)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "dim: (%f, %f)",
                 (boost)->body.dim.x, (boost)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "angle: %f",
                 (boost)->body.angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "boost_angle: %f",
                 (boost)->boost_angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "boost_power: %f",
                 (boost)->boost_power);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

inline void obstacle_render_info(PR::Obstacle *obstacle, float tx, float ty) {
    char buffer[99];
    std::memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    std::sprintf(buffer, "OBSTACLE INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "pos: (%f, %f)",
                 (obstacle)->body.pos.x, (obstacle)->body.pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "dim: (%f, %f)",
                 (obstacle)->body.dim.x, (obstacle)->body.dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "angle: %f",
                 (obstacle)->body.angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "collide_plane: %s",
                 (obstacle)->collide_plane ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "collide_rider: %s",
                 (obstacle)->collide_rider ? "true" : "false");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

inline void goal_line_render_info(Rect *rect, float tx, float ty) {
    char buffer[99];
    std::memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    std::sprintf(buffer, "GOAL LINE INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "pos: (%f, %f)",
                 rect->pos.x, rect->pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "dim: (%f, %f)",
                 rect->dim.x, rect->dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

inline void start_pos_render_info(Rect *rect, float tx, float ty) {
    char buffer[99];
    std::memset((void *)buffer, 0x00, sizeof(buffer));
    size_t index = 1;
    float spacing = OBJECT_INFO_FONT_SIZE;
    std::sprintf(buffer, "START POS INFO:");
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "pos: (%f, %f)",
                 rect->pos.x, rect->pos.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "dim: (%f, %f)",
                 rect->dim.x, rect->dim.y);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "angle: %f",
                 rect->angle);
    renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
                            &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
}

inline void apply_air_resistances(PR::Plane* p) {
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

inline void lerp_camera_x_to_rect(PR::Camera *cam, Rect *rec, bool center) {
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

inline void move_rider_to_plane(PR::Rider *rid, PR::Plane *p) {
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

inline Rect *get_selected_body(void *selected, PR::ObjectType selected_type) {
    Rect *b;
    switch(selected_type) {
        case PR::PORTAL_TYPE:
            b = &((PR::Portal *)selected)->body;
            break;
        case PR::BOOST_TYPE:
            b = &((PR::BoostPad *)selected)->body;
            break;
        case PR::OBSTACLE_TYPE:
            b = &((PR::Obstacle *)selected)->body;
            break;
        case PR::GOAL_LINE_TYPE:
            b = (Rect *) selected;
            break;
        case PR::P_START_POS_TYPE:
            b = (Rect *) selected;
            break;
        default:
            b = NULL;
            break;
    }
    return b;
}

void set_portal_option_buttons(PR::LevelButton *buttons) {
    // NOTE: Set up options buttons for the selected portal
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_PORTAL_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for portals");

        PR::LevelButton *button = buttons + option_button_index;

        button->from_center = true;
        button->body.pos.x = glob->window.w * (option_button_index+1) /
                             (SELECTED_PORTAL_OPTIONS+1);
        button->body.pos.y = glob->window.h * 9 / 10;
        button->body.dim.x = glob->window.w /
                             (SELECTED_PORTAL_OPTIONS+2);
        button->body.dim.y = glob->window.h / 10;

        switch(option_button_index) {
            case 0:
                std::snprintf(button->text,
                              std::strlen("WIDTH")+1,
                              "WIDTH");
                break;
            case 1:
                std::snprintf(button->text,
                              std::strlen("HEIGHT")+1,
                              "HEIGHT");
                break;
            case 2:
                std::snprintf(button->text,
                              std::strlen("TYPE")+1,
                              "TYPE");
                break;
            case 3:
                std::snprintf(button->text,
                              std::strlen("ENABLE_EFFECT")+1,
                              "ENABLE_EFFECT");
                break;
            default:
                std::snprintf(button->text,
                              std::strlen("UNDEFINED")+1,
                              "UNDEFINED");
                break;
        }

        button->col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);

    }
}

void set_boost_option_buttons(PR::LevelButton *buttons) {
    // NOTE: Set up options buttons for the selected boost
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_BOOST_OPTIONS;
        ++option_button_index) {

        assert((option_button_index <
                    SELECTED_MAX_OPTIONS)
                && "Selected options out of bounds for boosts");

        PR::LevelButton *button = buttons + option_button_index;

        button->from_center = true;
        button->body.pos.x = glob->window.w * (option_button_index+1) /
                             (SELECTED_BOOST_OPTIONS+1);
        button->body.pos.y = glob->window.h * 9 / 10;
        button->body.dim.x = glob->window.w /
                             (SELECTED_BOOST_OPTIONS+2);
        button->body.dim.y = glob->window.h / 10;

        switch(option_button_index) {
            case 0:
                std::snprintf(button->text,
                              std::strlen("WIDTH")+1,
                              "WIDTH");
                break;
            case 1:
                std::snprintf(button->text,
                              std::strlen("HEIGHT")+1,
                              "HEIGHT");
                break;
            case 2:
                std::snprintf(button->text,
                              std::strlen("ANGLE")+1,
                              "ANGLE");
                break;
            case 3:
                std::snprintf(button->text,
                              std::strlen("TRIANGLE")+1,
                              "TRIANGLE");
                break;
            case 4:
                std::snprintf(button->text,
                              std::strlen("BOOST_ANGLE")+1,
                              "BOOST_ANGLE");
                break;
            case 5:
                std::snprintf(button->text,
                              std::strlen("BOOST_POWER")+1,
                              "BOOST_POWER");
                break;
            default:
                std::snprintf(button->text,
                              std::strlen("UNDEFINED")+1,
                              "UNDEFINED");
                break;
        }

        button->col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);

    }
}

void set_obstacle_option_buttons(PR::LevelButton *buttons) {
    // NOTE: Set up options buttons for the selected obstacle
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_OBSTACLE_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for obstacle");

        PR::LevelButton *button = buttons + option_button_index;

        button->from_center = true;
        button->body.pos.x = glob->window.w * (option_button_index+1) /
                             (SELECTED_OBSTACLE_OPTIONS+1);
        button->body.pos.y = glob->window.h * 9 / 10;
        button->body.dim.x = glob->window.w /
                             (SELECTED_OBSTACLE_OPTIONS+2);
        button->body.dim.y = glob->window.h / 10;

        switch(option_button_index) {
            case 0:
                std::snprintf(button->text,
                              std::strlen("WIDTH")+1,
                              "WIDTH");
                break;
            case 1:
                std::snprintf(button->text,
                              std::strlen("HEIGHT")+1,
                              "HEIGHT");
                break;
            case 2:
                std::snprintf(button->text,
                              std::strlen("ANGLE")+1,
                              "ANGLE");
                break;
            case 3:
                std::snprintf(button->text,
                              std::strlen("TRIANGLE")+1,
                              "TRIANGLE");
                break;
            case 4:
                std::snprintf(button->text,
                              std::strlen("COLLIDE_PLANE")+1,
                              "COLLIDE_PLANE");
                break;
            case 5:
                std::snprintf(button->text,
                              std::strlen("COLLIDE_RIDER")+1,
                              "COLLIDE_RIDER");
                break;
            default:
                std::snprintf(button->text,
                              std::strlen("UNDEFINED")+1,
                              "UNDEFINED");
                break;
        }
        button->col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
    }
}

void set_start_pos_option_buttons(PR::LevelButton *buttons) {
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_START_POS_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for start_position");

        PR::LevelButton *button = buttons + option_button_index;

        button->from_center = true;
        button->body.pos.x = glob->window.w * (option_button_index+1) /
                             (SELECTED_START_POS_OPTIONS+1);
        button->body.pos.y = glob->window.h * 9 / 10;
        button->body.dim.x = glob->window.w /
                             (SELECTED_START_POS_OPTIONS+2);
        button->body.dim.y = glob->window.h / 10;

        switch(option_button_index) {
            case 0:
                std::snprintf(button->text,
                              std::strlen("ANGLE")+1,
                              "ANGLE");
                break;
            default:
                std::snprintf(button->text,
                              std::strlen("UNDEFINED")+1,
                              "UNDEFINED");
                break;
        }
        button->col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);

    }
}

inline Rect rect_in_camera_space(Rect r, PR::Camera *cam) {
    Rect res;

    res.pos = r.pos - cam->pos +
        glm::vec2(glob->window.w*0.5f, glob->window.h*0.5f);
    res.dim = r.dim;
    res.angle = r.angle;
    res.triangle = r.triangle;

    return res;
}

inline TexCoords texcoords_in_texture_space(float x, float y,
                                     float w, float h,
                                     Texture *tex, bool inverse) {
    TexCoords res;

    res.tx = x / tex->width;
    res.tw = w / tex->width;
    if (inverse) {
        res.th = -(h / tex->height);
        res.ty = (1.f - (y + h) / tex->height) - res.th;
    } else {
        res.ty = 1.f - (y + h) / tex->height;
        res.th = (h / tex->height);
    }

    /*std::cout << "--------------------"
              << "\ntx: " << res.tx
              << "\ntw: " << res.tw
              << "\nty: " << res.ty
              << "\nth: " << res.th
              << std::endl;*/
    
    return res;
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

