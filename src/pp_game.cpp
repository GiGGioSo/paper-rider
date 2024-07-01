#include <cassert>
#include <cstring>
#include <cmath>
#include <iostream>
#include <cstdio>
#include <cerrno>

#include "pp_game.h"
#include "pr_common.h"
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
//      - Update the README
//      - Better way to signal boost direction in boost pads
//      - Find/make textures
//      - Fine tune angle change velocity
//      - Profiling

// TODO(maybe):
//  - Make changing angle more or less difficult
//  - Make the plane change angle alone when the rider jumps off
//  - Make the boost change the plane angle


#define GRAVITY (630.f)

#define RIDER_GRAVITY (1300.f)
#define RIDER_VELOCITY_Y_LIMIT (1000.f)
#define RIDER_INPUT_VELOCITY_LIMIT (900.f)
#define RIDER_INPUT_VELOCITY_ACCELERATION (9000.f)
#define RIDER_FIRST_JUMP (700.f)
#define RIDER_SECOND_JUMP (400.f)

#define PLANE_VELOCITY_LIMIT (1300.f)

#define CAMERA_MAX_VELOCITY (1950.f)

#define START_BUTTON_DEFAULT_COLOR (glm::vec4(0.8f, 0.2f, 0.5f, 1.0f))
#define START_BUTTON_SELECTED_COLOR (glm::vec4(0.6, 0.0f, 0.3f, 1.0f))

#define EDIT_BUTTON_DEFAULT_COLOR (glm::vec4(0.9f, 0.4f, 0.6f, 1.0f))
#define EDIT_BUTTON_SELECTED_COLOR (glm::vec4(1.0, 0.6f, 0.8f, 1.0f))

#define DEL_BUTTON_DEFAULT_COLOR (glm::vec4(0.9f, 0.4f, 0.6f, 1.0f))
#define DEL_BUTTON_SELECTED_COLOR (glm::vec4(1.0, 0.6f, 0.8f, 1.0f))

#define LEVEL_BUTTON_DEFAULT_COLOR (glm::vec4(0.8f, 0.2f, 0.5f, 1.0f))
#define LEVEL_BUTTON_SELECTED_COLOR (glm::vec4(0.6, 0.0f, 0.3f, 1.0f))

#define SHOW_BUTTON_DEFAULT_COLOR (glm::vec4(0.8f, 0.2f, 0.5f, 1.0f))
#define SHOW_BUTTON_SELECTED_COLOR (glm::vec4(0.6, 0.0f, 0.3f, 1.0f))

#define OPTION_SLIDER_DEFAULT_COLOR (glm::vec4(1.0f))
#define OPTION_SLIDER_SELECTED_COLOR (glm::vec4(1.0f, 0.5f, 0.5f, 1.0f))

#define OPTION_BUTTON_DEFAULT_COLOR (glm::vec4(0.7f, 0.7f, 0.7f, 1.0f))
#define OPTION_BUTTON_SELECTED_COLOR (glm::vec4(0.4f, 0.4f, 0.4f, 1.0f))

// Utilities functions for code reuse
inline void
portal_render(PR::Portal *portal);
inline void
boostpad_render(PR::BoostPad *pad);
inline void
obstacle_render(PR::Obstacle *obs);
inline void
button_render(PR::Button but, glm::vec4 col, Font *font);
inline void
button_render_in_menu_camera(PR::Button but, glm::vec4 col, Font *font, PR::MenuCamera *cam);
inline void
portal_render_info(PR::Portal *portal, float tx, float ty);
inline void
boostpad_render_info(PR::BoostPad *boost, float tx, float ty);
inline void
obstacle_render_info(PR::Obstacle *obstacle, float tx, float ty);
inline void
goal_line_render_info(Rect *rect, float tx, float ty);
inline void
start_pos_render_info(Rect *rect, glm::vec2 vel, float tx, float ty);
inline void
plane_update_animation(PR::Plane *p);
inline void
plane_activate_crash_animation(PR::Plane *p);
inline void
level_reset_colors(PR::Level *);
inline void
level_shuffle_colors(PR::Level *level);
inline Rect
*get_selected_body(void *selected, PR::ObjectType selected_type);
void
button_set_position(PR::Button *button, size_t index);
void
button_edit_del_to_lb(PR::Button *reference, PR::Button *edit, PR::Button *del);
void
set_portal_option_buttons(PR::Button *buttons);
void
set_boost_option_buttons(PR::Button *buttons);
void
set_obstacle_option_buttons(PR::Button *buttons);
void
set_start_pos_option_buttons(PR::Button *buttons);
inline Rect
rect_in_camera_space(Rect r, PR::Camera *cam);
inline Rect
rect_in_menu_camera_space(Rect r, PR::MenuCamera *cam);
void
level_deactivate_edit_mode(PR::Level *level);
void
level_activate_edit_mode(PR::Level *level);
void
update_plane_physics_n_boost_collisions(PR::Level *level);

// Parallax stuff
void
parallax_init(PR::Parallax *px, float fc, TexCoords tc, float p_start_x, float p_start_y, float p_w, float p_h);
void
parallax_update_n_queue_render(PR::Parallax *px, float current_x);

// Animation stuff
void
animation_init(PR::Animation *a, Texture tex, size_t start_x, size_t start_y, size_t dim_x, size_t dim_y, size_t step_x, size_t step_y, size_t frame_number, float frame_duration, bool loop);
void
animation_step(PR::Animation *a);
void
animation_queue_render(Rect b, PR::Animation *a, bool inverse);
void
animation_reset(PR::Animation *a);

inline void
apply_air_resistances(PR::Plane* p);
inline void
lerp_camera_x_to_rect(PR::Camera *cam, Rect *rec, bool center);
inline void
move_rider_to_plane(PR::Rider *rid, PR::Plane *p);
inline void
rider_jump_from_plane(PR::Rider *rid, PR::Plane *p);

// Particle system functions
void
create_particle_plane_boost(PR::ParticleSystem *ps, PR::Particle *particle);
void
update_particle_plane_boost(PR::ParticleSystem *ps, PR::Particle *particle);
void
draw_particle_plane_boost(PR::ParticleSystem *ps, PR::Particle *particle);

void
create_particle_plane_crash(PR::ParticleSystem *ps, PR::Particle *particle);
void
update_particle_plane_crash(PR::ParticleSystem *ps, PR::Particle *particle);
void
draw_particle_plane_crash(PR::ParticleSystem *ps, PR::Particle *particle);

void
create_particle_rider_crash(PR::ParticleSystem *ps, PR::Particle *particle);
void
update_particle_rider_crash(PR::ParticleSystem *ps, PR::Particle *particle);
void
draw_particle_rider_crash(PR::ParticleSystem *ps, PR::Particle *particle);

// UI OptionSlider funcionts
void
option_slider_init_selection(PR::OptionSlider *slider, float pad);
void
option_slider_update_value(PR::OptionSlider *slider, float value);
void
option_slider_render(PR::OptionSlider *slider, glm::vec4 color);
void
option_slider_handle_mouse(PR::OptionSlider *slider, float mouseX, float mouseY, Key mouse_button);
void
options_menu_selection_handle_mouse(PR::OptionsMenu *opt, float mouseX, float mouseY, bool showing_general_pane);
void
options_menu_update_bindings(PR::OptionsMenu *opt, InputAction *actions);

void
display_mode_update(PR::WinInfo *win, PR::DisplayMode dm);

void
window_resolution_set(PR::WinInfo *win, PR::WindowResolution res);
PR::WindowResolution
window_resolution_prev(PR::WindowResolution res);
PR::WindowResolution
window_resolution_next(PR::WindowResolution res);
const char *
window_resolution_to_str(PR::WindowResolution res);
PR::WindowResolution
window_resolution_from_dim(int width, int height);
int
window_resolution_width(PR::WindowResolution res);
int
window_resolution_height(PR::WindowResolution res);

void free_all_cases(PR::PlayMenu *menu, PR::Level *level,
                    PR::StartMenu *start, PR::OptionsMenu *opt) {
    // Menu freeing
    da_clear(&menu->custom_buttons);

    // Level freeing
    da_clear(&level->portals);
    da_clear(&level->obstacles);
    da_clear(&level->boosts);
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(level->particle_systems);
        ++ps_index) {
        if (level->particle_systems[ps_index].particles) {
            std::free(level->particle_systems[ps_index].particles);
            level->particle_systems[ps_index].particles = NULL;
        }
    }

    // Start menu freeing
    // Nothing to free
    UNUSED(start);

    // Options menu freeing
    // Nothing to free
    UNUSED(opt);
}

void play_menu_set_to_null(PR::PlayMenu *menu) {
    menu->custom_buttons = {NULL, 0, 0};
}

void level_set_to_null(PR::Level *level) {
    level->portals = {NULL, 0, 0};
    level->obstacles = {NULL, 0, 0};
    level->boosts = {NULL, 0, 0};
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(level->particle_systems);
        ++ps_index) {
        level->particle_systems[ps_index].particles_number = 0;
        level->particle_systems[ps_index].particles = NULL;
    }
}

void start_menu_set_to_null(PR::StartMenu *start) {
    // Nothing to set to null
    UNUSED(start);
}

void options_menu_set_to_null(PR::OptionsMenu *opt) {
    // Nothing to set to null
    UNUSED(opt);
    glob->input.kb_binding = NULL;
    glob->input.gp_binding = NULL;
}

int load_map_from_file(const char *file_path,
                       PR::Obstacles *obstacles,
                       PR::BoostPads *boosts,
                       PR::Portals *portals,
                       float *start_x, float *start_y,
                       float *start_vel_x, float *start_vel_y,
                       float *start_angle,
                       float *goal_line) {
    
    int result = 0;
    FILE *map_file = NULL;

    {
        da_clear(obstacles);
        da_clear(portals);
        da_clear(boosts);

        map_file = std::fopen(file_path, "rb");
        if (map_file == NULL) return_defer(1);

        char tmp[256];
        std::fscanf(map_file, " %s ", tmp);
        if (std::ferror(map_file)) return_defer(1);

        size_t number_of_obstacles;
        std::fscanf(map_file, " %zu", &number_of_obstacles);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << number_of_obstacles
                  << " obstacles from the file " << file_path
                  << std::endl;

        // *obstacles = (PR::Obstacle *) std::malloc(sizeof(PR::Obstacle) *
        //                                           *number_of_obstacles);
        // if (*obstacles == NULL) {
        //     std::cout << "[ERROR] Buy more RAM!" << std::endl;
        //     return_defer(2);
        // }

        for (size_t obstacle_index = 0;
             obstacle_index < number_of_obstacles;
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
                std::cout << "[WARNING] Found only " << obstacle_index
                          << " obstacles in the map file" << std::endl;
                return_defer(1);
            }

            PR::Obstacle obs;
            obs.body.pos.x = x;
            obs.body.pos.y = y;
            obs.body.dim.x = w;
            obs.body.dim.y = h;
            obs.body.angle = r;
            obs.body.triangle = triangle;

            obs.collide_plane = collide_plane;
            obs.collide_rider = collide_rider;

            da_append(obstacles, obs, PR::Obstacle);

            std::cout << "x: " << x
                      << " y: " << y
                      << " w: " << w
                      << " h: " << h
                      << " r: " << r
                      << " tr: " << triangle
                      << " cp: " << collide_plane
                      << " cr: " << collide_rider
                      << std::endl;

        }

        // NOTE: Loading the boosts from memory
        size_t number_of_boosts;
        std::fscanf(map_file, " %zu", &number_of_boosts);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << number_of_boosts
                  << " boost pads from file " << file_path
                  << std::endl;

        // *boosts = (PR::BoostPad *) std::malloc(sizeof(PR::BoostPad) *
        //                            *number_of_boosts);
        // if (*boosts == NULL) {
        //     std::cout << "[ERROR] Buy more RAM!" << std::endl;
        //     return_defer(2);
        // }

        for(size_t boost_index = 0;
            boost_index < number_of_boosts;
            ++boost_index) {
            
            int triangle;
            float x, y, w, h, r, ba, bp;

            std::fscanf(map_file,
                        " %i %f %f %f %f %f %f %f",
                        &triangle, &x, &y, &w, &h, &r, &ba, &bp);

            if (std::ferror(map_file)) return_defer(1);
            if (std::feof(map_file)) {
                std::cout << "[WARNING] Found only " << boost_index
                          << " boosts in the map file" << std::endl;
                break;
            }

            PR::BoostPad pad;
            pad.body.pos.x = x;
            pad.body.pos.y = y;
            pad.body.dim.x = w;
            pad.body.dim.y = h;
            pad.body.angle = r;
            pad.body.triangle = triangle;
            pad.boost_angle = ba;
            pad.boost_power = bp;

            da_append(boosts, pad, PR::BoostPad);

            std::cout << "x: " << x
                      << " y: " << y
                      << " w: " << w
                      << " h: " << h
                      << " r: " << r
                      << " tr: " << triangle
                      << " ba: " << ba
                      << " bp: " << bp
                      << std::endl;


        }

        // NOTE: Loading the portals from memory
        size_t number_of_portals;
        std::fscanf(map_file, " %zu", &number_of_portals);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << number_of_portals
                  << " portals from file " << file_path
                  << std::endl;

        // *portals = (PR::Portal *) std::malloc(sizeof(PR::Portal) *
        //                            *number_of_portals);
        // if (*portals == NULL) {
        //     std::cout << "[ERROR] Buy more RAM!" << std::endl;
        //     return_defer(2);
        // }

        for(size_t portal_index = 0;
            portal_index < number_of_portals;
            ++portal_index) {

            float x, y, w, h;
            int type;
            int enable;

            std::fscanf(map_file,
                        " %i %i %f %f %f %f",
                        &type, &enable, &x, &y, &w, &h);
            if (std::ferror(map_file)) return_defer(1);
            if (std::feof(map_file)) {
                std::cout << "[WARNING] Found only " << portal_index
                          << " portals in the map file" << std::endl;
                break;
            }

            PR::Portal portal;
            switch(type) {
                case PR::INVERSE:
                {
                    portal.type = PR::INVERSE;
                    break;
                }
                case PR::SHUFFLE_COLORS:
                {
                    portal.type = PR::SHUFFLE_COLORS;
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

            portal.enable_effect = enable;
            portal.body.pos.x = x;
            portal.body.pos.y = y;
            portal.body.dim.x = w;
            portal.body.dim.y = h;
            portal.body.angle = 0.f;
            portal.body.triangle = false;

            da_append(portals, portal, PR::Portal);

            std::cout << "x: " << x
                      << " y: " << y
                      << " w: " << w
                      << " h: " << h
                      << " type: " << type
                      << " enable: " << enable
                      << std::endl;
        }

        std::fscanf(map_file, " %f", goal_line);
        if (std::ferror(map_file)) return_defer(1);
        *goal_line = *goal_line;
        
        std::cout << "[LOADING] goal_line set at: "
                  << *goal_line
                  << std::endl;

        std::fscanf(map_file, " %f %f %f %f %f",
                    start_x, start_y, start_vel_x, start_vel_y, start_angle);
        if (std::ferror(map_file)) return_defer(1);
        *start_x = *start_x;
        *start_y = *start_y;

        std::cout << "[LOADING] player start position set to"
                  << " x: " << *start_x
                  << " y: " << *start_y
                  << " with angle of: " << *start_angle
                  << std::endl;
    }

    defer:
    if (map_file) std::fclose(map_file);
    if (result != 0 && obstacles->count > 0) da_clear(obstacles);
    if (result != 0 && boosts->count > 0) da_clear(boosts);
    if (result != 0 && portals->count > 0) da_clear(portals);
    return result;
}

int save_map_to_file(const char *file_path,
                     PR::Level *level) {
    int result = 0;
    FILE *map_file = NULL;

    {
        map_file = std::fopen(file_path, "wb");
        if (map_file == NULL) return_defer(1);

        std::fprintf(map_file, "%s\n", level->name);
        if (std::ferror(map_file)) return_defer(1);

        std::fprintf(map_file, "%zu\n", level->obstacles.count);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t obs_index = 0;
            obs_index < level->obstacles.count;
            ++obs_index) {

            PR::Obstacle obs = level->obstacles.items[obs_index];
            Rect b = obs.body;
            std::fprintf(map_file,
                         "%i %i %i %f %f %f %f %f\n",
                         obs.collide_plane, obs.collide_rider,
                         b.triangle, b.pos.x, b.pos.y,
                         b.dim.x, b.dim.y, b.angle);
            if (std::ferror(map_file)) return_defer(1);

        }

        std::fprintf(map_file, "%zu\n", level->boosts.count);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t boost_index = 0;
            boost_index < level->boosts.count;
            ++boost_index) {

            PR::BoostPad pad = level->boosts.items[boost_index];
            Rect b = pad.body;
            std::fprintf(map_file,
                         "%i %f %f %f %f %f %f %f\n",
                         b.triangle, b.pos.x, b.pos.y,
                         b.dim.x, b.dim.y, b.angle,
                         pad.boost_angle, pad.boost_power);
            if (std::ferror(map_file)) return_defer(1);
        }

        std::fprintf(map_file, "%zu\n", level->portals.count);
        if (std::ferror(map_file)) return_defer(1);

        for(size_t portal_index = 0;
            portal_index < level->portals.count;
            ++portal_index) {

            PR::Portal portal = level->portals.items[portal_index];
            Rect b = portal.body;
            std::fprintf(map_file,
                        "%i %i %f %f %f %f\n",
                        portal.type, portal.enable_effect,
                        b.pos.x, b.pos.y, b.dim.x, b.dim.y);
            if (std::ferror(map_file)) return_defer(1);
        }

        // Goal line
        std::fprintf(map_file,
                     "%f\n",
                     level->goal_line.pos.x);
        if (std::ferror(map_file)) return_defer(1);

        // Player start position
        std::fprintf(map_file,
                     "%f %f %f %f %f",
                     level->start_pos.pos.x,
                     level->start_pos.pos.y,
                     level->start_vel.x, level->start_vel.y,
                     level->start_pos.angle);
        if (std::ferror(map_file)) return_defer(1);
    }

    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

int load_custom_buttons_from_dir(const char *dir_path,
                                 PR::CustomLevelButtons *buttons) {

    // PR::WinInfo *win = &glob->window;

    int result = 0;
    DIR *dir = NULL;
    FILE *map_file = NULL;

    {
        da_clear(buttons);

        dir = opendir(dir_path);
        if (dir == NULL) {
            std::cout << "[ERROR] Could not open directory: "
                      << dir_path << std::endl;
            return_defer(1);
        }

        dirent *dp = NULL;
        while ((dp = readdir(dir))) {

            const char *extension = std::strrchr(dp->d_name, '.');

            if (std::strcmp(extension, ".prmap") == 0) {

                char map_path[256] = "";
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

                char map_name[256] = {};

                map_file = std::fopen(map_path, "rb");
                if (std::ferror(map_file)) return_defer(1);

                std::fscanf(map_file, " %s", map_name);
                if (std::ferror(map_file)) return_defer(1);

                std::cout << "Found map_file: "
                          << map_name << " length: " << std::strlen(map_name)
                          << std::endl;

                PR::CustomLevelButton lb;
                lb.is_new_level = false;

                assert((std::strlen(map_name)+1 <=
                            ARRAY_LENGTH(lb.button.text))
                        && "Map name bigger than button text buffer!");
                std::snprintf(lb.button.text, std::strlen(map_name)+1,
                              "%s", map_name);

                assert((std::strlen(map_path)+1 <=
                            ARRAY_LENGTH(lb.mapfile_path))
                        && "Mapfile path bigger than button mapfile buffer!");
                std::snprintf(lb.mapfile_path, std::strlen(map_path)+1,
                              "%s", map_path);


                da_append(buttons, lb, PR::CustomLevelButton);

                size_t current_index = buttons->count-1;

                while(current_index > 0 &&
                      std::strcmp(
                          buttons->items[current_index-1].button.text,
                          buttons->items[current_index].button.text) > 0) {
                    da_swap(buttons,
                            current_index-1, current_index,
                            PR::CustomLevelButton);
                    current_index--;
                }
                button_set_position(&buttons->items[current_index].button,
                                    current_index);
                button_edit_del_to_lb(&buttons->items[current_index].button,
                                      &buttons->items[current_index].edit,
                                      &buttons->items[current_index].del);

                if (map_file) std::fclose(map_file);
                // NOTE: Always set to NULL the resource you free
                map_file = NULL;
                
            }
        }
    }

    defer:
    if (map_file) std::fclose(map_file);
    if (result != 0 && buttons->count > 0) da_clear(buttons);
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
    "./campaign_maps/level1.prmap",
    "./campaign_maps/level2.prmap"
};

int start_menu_prepare(PR::StartMenu *start) {
    start->play = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.2f),
            .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
            .angle = 0.f,
            .triangle = false
        },
        .col = START_BUTTON_DEFAULT_COLOR,
        .text = "PLAY"
    };

    start->options = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f),
            .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
            .angle = 0.f,
            .triangle = false
        },
        .col = START_BUTTON_DEFAULT_COLOR,
        .text = "OPTIONS"
    };

    start->quit = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.8f),
            .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
            .angle = 0.f,
            .triangle = false
        },
        .col = START_BUTTON_DEFAULT_COLOR,
        .text = "QUIT"
    };

    // NOTE: Make the cursor show
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    PR::Sound *sound = &glob->sound;
    if (!ma_sound_is_playing(&sound->menu_music)) {
        ma_sound_seek_to_pcm_frame(&sound->menu_music, 0);
        ma_sound_start(&sound->menu_music);
    }

    return 0;
}
void start_menu_update() {
    PR::StartMenu *start = &glob->current_start_menu;
    InputController *input = &glob->input;
    PR::Sound *sound = &glob->sound;

    // ### CHANGING SELECTION ###
    PR::StartMenuSelection before_selection = start->selection;
    // # Changing selection with keybindings #
    if (ACTION_CLICKED(PR_MENU_UP)) {
        if (start->selection == PR::START_BUTTON_QUIT) {
            start->selection = PR::START_BUTTON_OPTIONS;
        } else if (start->selection == PR::START_BUTTON_OPTIONS) {
            start->selection = PR::START_BUTTON_PLAY;
        }
    }
    if (ACTION_CLICKED(PR_MENU_DOWN)) {
        if (start->selection == PR::START_BUTTON_PLAY) {
            start->selection = PR::START_BUTTON_OPTIONS;
        } else if (start->selection == PR::START_BUTTON_OPTIONS) {
            start->selection = PR::START_BUTTON_QUIT;
        }
    }
    // # Changing selection with the mouse #
    if (start->selection != PR::START_BUTTON_PLAY &&
        input->was_mouse_moved &&
        rect_contains_point(start->play.body,
                            input->mouseX, input->mouseY,
                            start->play.from_center)) {
        start->selection = PR::START_BUTTON_PLAY;
    }
    if (start->selection != PR::START_BUTTON_OPTIONS &&
        input->was_mouse_moved &&
        rect_contains_point(start->options.body,
                            input->mouseX, input->mouseY,
                            start->options.from_center)) {
        start->selection = PR::START_BUTTON_OPTIONS;
    }
    if (start->selection != PR::START_BUTTON_QUIT &&
        input->was_mouse_moved &&
        rect_contains_point(start->quit.body,
                            input->mouseX, input->mouseY,
                            start->quit.from_center)) {
        start->selection = PR::START_BUTTON_QUIT;
    }

    // # Change button color based on selection #
    if (start->selection == PR::START_BUTTON_PLAY) {
        start->play.col = START_BUTTON_SELECTED_COLOR;
        start->options.col = START_BUTTON_DEFAULT_COLOR;
        start->quit.col = START_BUTTON_DEFAULT_COLOR;
    } else if (start->selection == PR::START_BUTTON_OPTIONS) {
        start->options.col = START_BUTTON_SELECTED_COLOR;
        start->play.col = START_BUTTON_DEFAULT_COLOR;
        start->quit.col = START_BUTTON_DEFAULT_COLOR;
    } else if (start->selection == PR::START_BUTTON_QUIT) {
        start->quit.col = START_BUTTON_SELECTED_COLOR;
        start->play.col = START_BUTTON_DEFAULT_COLOR;
        start->options.col = START_BUTTON_DEFAULT_COLOR;
    }

    // ### CLICKING THE SELECTION ###
    // # PLAY #
    if ((ACTION_CLICKED(PR_MENU_CLICK) &&
            start->selection == PR::START_BUTTON_PLAY) ||
        (input->mouse_left.clicked &&
            rect_contains_point(start->play.body,
                                input->mouseX, input->mouseY,
                                start->play.from_center))) {
        glob->current_play_menu.showing_campaign_buttons = true;
        ma_sound_seek_to_pcm_frame(&sound->click_selected, 0);
        ma_sound_start(&sound->click_selected);
        CHANGE_CASE_TO_PLAY_MENU(void());
    }
    // # OPTIONS #
    if ((ACTION_CLICKED(PR_MENU_CLICK) &&
            start->selection == PR::START_BUTTON_OPTIONS) ||
        (input->mouse_left.clicked &&
            rect_contains_point(start->options.body,
                                input->mouseX, input->mouseY,
                                start->options.from_center))) {
        ma_sound_seek_to_pcm_frame(&sound->click_selected, 0);
        ma_sound_start(&sound->click_selected);
        CHANGE_CASE_TO_OPTIONS_MENU(void());
    }
    // # QUIT #
    if ((ACTION_CLICKED(PR_MENU_CLICK) &&
            start->selection == PR::START_BUTTON_QUIT) ||
        (input->mouse_left.clicked &&
            rect_contains_point(start->quit.body,
                                input->mouseX, input->mouseY,
                                start->quit.from_center))) {
        ma_sound_seek_to_pcm_frame(&sound->click_selected, 0);
        ma_sound_start(&sound->click_selected);
        glfwSetWindowShouldClose(glob->window.glfw_win, true);
    }

    // ### SOUND ###
    if (start->selection != before_selection) {
        ma_sound_seek_to_pcm_frame(&sound->change_selection, 0);
        ma_sound_start(&sound->change_selection);
    }

    // ### RENDERING ###
    Rect full_screen = {
        .pos = glm::vec2(0.f, 0.f),
        .dim = glm::vec2(GAME_WIDTH, GAME_HEIGHT),
        .angle = 0.f,
        .triangle = false,
    };
    renderer_add_queue_uni(
            full_screen,
            glm::vec4(0.3f, 0.8f, 0.9f, 1.0f),
            false);
    button_render(start->play, glm::vec4(1.f), &glob->rend_res.fonts[0]);
    button_render(start->options, glm::vec4(1.f), &glob->rend_res.fonts[0]);
    button_render(start->quit, glm::vec4(1.f), &glob->rend_res.fonts[0]);
    // # Issue draw calls #
    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    // ### Test drawing array textures ###
    if (ACTION_PRESSED(PR_MENU_LEVEL_DELETE)) {
        // renderer_add_queue_array_tex(NULL, 100, 100, 400, 50, 0, false, PR_TEX1_FRECCIA);
        // renderer_draw_array_tex(glob->rend_res.shaders[4], NULL);
    }
}

int options_menu_prepare(PR::OptionsMenu *opt) {
    InputController *input = &glob->input;
    PR::MenuCamera *cam = &opt->camera;
    PR::WinInfo *win = &glob->window;

    opt->to_start_menu = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH / 30.f, GAME_HEIGHT / 22.5f),
            .dim = glm::vec2(GAME_WIDTH / 20.f, GAME_HEIGHT / 15.f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_DEFAULT_COLOR,
        .text = "<-",
    };

    opt->showing_general_pane = true;
    opt->to_general_pane = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.1f),
            .dim = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.125f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_SELECTED_COLOR,
        .text = "GENERAL\0",
    };
    opt->to_controls_pane = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.7f, GAME_HEIGHT * 0.1f),
            .dim = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.125f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_DEFAULT_COLOR,
        .text = "KEYBINDINGS\0",
    };

    // # Master volume slider initialization #
    opt->master_volume = {
        .background = {
            .pos = glm::vec2(GAME_WIDTH * 0.75f,
                             GAME_HEIGHT * (0.2f + (0.f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.05f),
            .angle = 0.f,
            .triangle = false,
        },
        .label = "MASTER VOLUME\0",
    };
    opt->master_volume.value = glob->sound.master_volume;
    option_slider_init_selection(&opt->master_volume, 0.1f);

    // # Sfx volume slider initialization #
    opt->sfx_volume = {
        .background = {
            .pos = glm::vec2(GAME_WIDTH * 0.75f,
                             GAME_HEIGHT * (0.2f + (0.8f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.05f),
            .angle = 0.f,
            .triangle = false,
        },
        .label = "SFX VOLUME\0",
    };
    opt->sfx_volume.value = glob->sound.sfx_volume;
    option_slider_init_selection(&opt->sfx_volume, 0.1f);

    // # Music volume slider initialization #
    opt->music_volume = {
        .background = {
            .pos = glm::vec2(GAME_WIDTH * 0.75f,
                             GAME_HEIGHT * (0.2f + (1.6f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.3f, GAME_HEIGHT * 0.05f),
            .angle = 0.f,
            .triangle = false,
        },
        .label = "MUSIC VOLUME\0",
    };
    opt->music_volume.value = glob->sound.music_volume;
    option_slider_init_selection(&opt->music_volume, 0.1f);

    // # Display mode buttons initialization #
    opt->display_mode_fullscreen = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * (0.5f + 1.f/12.f), 
                             GAME_HEIGHT * (0.2f + (2.4f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.12f, GAME_HEIGHT * 0.06f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = (win->display_mode == PR::FULLSCREEN) ?
                OPTION_BUTTON_SELECTED_COLOR : OPTION_BUTTON_DEFAULT_COLOR,
        .text = "FULLSCREEN\0",
    };
    opt->display_mode_borderless = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * (0.5f + 3.f/12.f), 
                             GAME_HEIGHT * (0.2f + (2.4f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.12f, GAME_HEIGHT * 0.06f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = (win->display_mode == PR::BORDERLESS) ?
                OPTION_BUTTON_SELECTED_COLOR : OPTION_BUTTON_DEFAULT_COLOR,
        .text = "BORDERLESS\0",
    };
    opt->display_mode_windowed = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * (0.5f + 5.f/12.f), 
                             GAME_HEIGHT * (0.2f + (2.4f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.12f, GAME_HEIGHT * 0.06f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = (win->display_mode == PR::WINDOWED) ?
                OPTION_BUTTON_SELECTED_COLOR : OPTION_BUTTON_DEFAULT_COLOR,
        .text = "WINDOWED\0",
    };

    // # Resolution button initialization
    opt->resolution_up = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * (0.5f + 0.4f), 
                             GAME_HEIGHT * (0.2f + (3.2f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.04f, GAME_HEIGHT * 0.06f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = OPTION_BUTTON_DEFAULT_COLOR,
        .text = ">\0",
    };
    opt->resolution_down = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * (0.5f + 0.1f), 
                             GAME_HEIGHT * (0.2f + (3.2f + 0.4f) / 5.f)),
            .dim = glm::vec2(GAME_WIDTH * 0.04f, GAME_HEIGHT * 0.06f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = OPTION_BUTTON_DEFAULT_COLOR,
        .text = "<\0",
    };

    cam->pos.x = GAME_WIDTH * 0.5f;
    cam->pos.y = GAME_HEIGHT * 0.5f;
    cam->speed_multiplier = 6.f;
    cam->goal_position = cam->pos.y;

    opt->resolution_selection =
        window_resolution_from_dim(glob->window.width, glob->window.height);

    // NOTE: Set up all of the changing keybinds buttons
    for(size_t bind_index = 0;
        bind_index < ARRAY_LENGTH(opt->change_kb_binds1);
        ++bind_index) {

        InputAction *action = &input->actions[bind_index];

        PR::Button *kb1 = &opt->change_kb_binds1[bind_index];
        PR::Button *kb2 = &opt->change_kb_binds2[bind_index];
        PR::Button *gp1 = &opt->change_gp_binds1[bind_index];
        PR::Button *gp2 = &opt->change_gp_binds2[bind_index];

        int y = (bind_index + 1) * ((float)GAME_HEIGHT / 6) +
                    ((float)GAME_HEIGHT / 12);
        int h = GAME_HEIGHT * 0.125f; //   / 8

        *kb1 = {
            .from_center = true,
            .body = {
                .pos =
                    glm::vec2(GAME_WIDTH * 9.f / 16.f, y),
                .dim = glm::vec2(GAME_WIDTH * 0.1f, h),
                .angle = 0.f,
                .triangle = false,
            },
            .col = OPTION_BUTTON_DEFAULT_COLOR,
        };
        const char *kb1_name = get_key_name(action->kb_binds[0].bind_index);
        std::strncpy(kb1->text,
                     (kb1_name != NULL) ? kb1_name : "...",
                     ARRAY_LENGTH(kb1->text)-1);

        *kb2 = {
            .from_center = true,
            .body = {
                .pos =
                    glm::vec2(GAME_WIDTH * 11.f / 16.f, y),
                .dim = glm::vec2(GAME_WIDTH * 0.1f, h),
                .angle = 0.f,
                .triangle = false,
            },
            .col = OPTION_BUTTON_DEFAULT_COLOR,
        };
        const char *kb2_name = get_key_name(action->kb_binds[1].bind_index);
        std::strncpy(kb2->text,
                     (kb2_name != NULL) ? kb2_name : "...",
                     ARRAY_LENGTH(kb2->text)-1);

        *gp1 = {
            .from_center = true,
            .body = {
                .pos =
                    glm::vec2(GAME_WIDTH * 13.f / 16.f, y),
                .dim = glm::vec2(GAME_WIDTH * 0.1f, h),
                .angle = 0.f,
                .triangle = false,
            },
            .col = OPTION_BUTTON_DEFAULT_COLOR,
        };
        const char *gp1_name =
            get_gamepad_button_name(action->gp_binds[0].bind_index,
                                    action->gp_binds[0].type);
        std::strncpy(gp1->text,
                     (gp1_name != NULL) ? gp1_name : "...",
                     ARRAY_LENGTH(gp1->text)-1);

        *gp2 = {
            .from_center = true,
            .body = {
                .pos =
                    glm::vec2(GAME_WIDTH * 15.f / 16.f, y),
                .dim = glm::vec2(GAME_WIDTH * 0.1f, h),
                .angle = 0.f,
                .triangle = false,
            },
            .col = OPTION_BUTTON_DEFAULT_COLOR,
        };
        const char *gp2_name =
            get_gamepad_button_name(action->gp_binds[1].bind_index,
                                    action->gp_binds[1].type);
        std::strncpy(gp2->text,
                     (gp2_name != NULL) ? gp2_name : "...",
                     ARRAY_LENGTH(gp2->text)-1);
    }

    // NOTE: Make the cursor show
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    PR::Sound *sound = &glob->sound;
    if (!ma_sound_is_playing(&sound->menu_music)) {
        ma_sound_seek_to_pcm_frame(&sound->menu_music, 0);
        ma_sound_start(&sound->menu_music);
    }

    return 0;
}
void options_menu_update() {
    PR::OptionsMenu *opt = &glob->current_options_menu;
    PR::Sound *sound = &glob->sound;
    PR::MenuCamera *cam = &opt->camera;
    InputController *input = &glob->input;
    PR::WinInfo *win = &glob->window;
    //float dt = glob->state.delta_time;

    // # Check if going back to the start menu #
    if (ACTION_CLICKED(PR_MENU_EXIT) ||
        (input->mouse_left.clicked &&
            rect_contains_point(opt->to_start_menu.body,
                                input->mouseX, input->mouseY,
                                opt->to_start_menu.from_center))) {
        ma_sound_seek_to_pcm_frame(&sound->to_start_menu, 0);
        ma_sound_start(&sound->to_start_menu);
        CHANGE_CASE_TO_START_MENU(void());
    }
    // # Check if changing options pane #
    if (!opt->showing_general_pane &&
        (ACTION_CLICKED(PR_MENU_PANE_LEFT) ||
          (input->mouse_left.clicked &&
           rect_contains_point(
                rect_in_menu_camera_space(opt->to_general_pane.body, cam),
                input->mouseX, input->mouseY,
                opt->to_general_pane.from_center)))) {
        opt->showing_general_pane = true;
        opt->to_general_pane.col = LEVEL_BUTTON_SELECTED_COLOR;
        opt->to_controls_pane.col = LEVEL_BUTTON_DEFAULT_COLOR;
        // Reset camera position
        cam->pos.x = GAME_WIDTH * 0.5f;
        cam->pos.y = GAME_HEIGHT * 0.5f;
        cam->speed_multiplier = 6.f;
        cam->goal_position = cam->pos.y;
        ma_sound_seek_to_pcm_frame(&sound->change_pane, 0);
        ma_sound_start(&sound->change_pane);
    }
    if (opt->showing_general_pane &&
        (ACTION_CLICKED(PR_MENU_PANE_RIGHT) ||
          (input->mouse_left.clicked &&
           rect_contains_point(
             rect_in_menu_camera_space(opt->to_controls_pane.body, cam),
             input->mouseX, input->mouseY,
             opt->to_controls_pane.from_center)))) {
        opt->showing_general_pane = false;
        opt->to_controls_pane.col = LEVEL_BUTTON_SELECTED_COLOR;
        opt->to_general_pane.col = LEVEL_BUTTON_DEFAULT_COLOR;
        ma_sound_seek_to_pcm_frame(&sound->change_pane, 0);
        ma_sound_start(&sound->change_pane);
    }


    if (input->was_mouse_moved) {
        options_menu_selection_handle_mouse(
                opt, input->mouseX, input->mouseY,
                opt->showing_general_pane);
    }

    // Will need it later when chaging the resolution
    Rect unsaved_resolution_background = {
        .pos = glm::vec2(GAME_WIDTH * 0.75f,
                opt->resolution_up.body.pos.y),
        .dim = glm::vec2(GAME_WIDTH * 0.25f, GAME_HEIGHT * 0.15f),
        .angle = 0.f,
        .triangle = false,
    };

    if (opt->showing_general_pane) {
        // NOTE: Change selection with keybindings
        if (ACTION_CLICKED(PR_MENU_DOWN)) {
            switch (opt->current_selection) {
                case PR::OPTION_NONE:
                    opt->current_selection = PR::OPTION_MASTER_VOLUME; break;
                case PR::OPTION_MASTER_VOLUME:
                    opt->current_selection = PR::OPTION_SFX_VOLUME; break;
                case PR::OPTION_SFX_VOLUME:
                    opt->current_selection = PR::OPTION_MUSIC_VOLUME; break;
                case PR::OPTION_MUSIC_VOLUME:
                    opt->current_selection = PR::OPTION_DISPLAY_MODE; break;
                case PR::OPTION_DISPLAY_MODE:
                    if (win->display_mode != PR::WINDOWED) break;
                    opt->current_selection = PR::OPTION_RESOLUTION; break;
                case PR::OPTION_RESOLUTION: break;
                default: break;
            }
        }
        if (ACTION_CLICKED(PR_MENU_UP)) {
            switch (opt->current_selection) {
                case PR::OPTION_NONE: break;
                case PR::OPTION_MASTER_VOLUME:
                    opt->current_selection = PR::OPTION_NONE; break;
                case PR::OPTION_SFX_VOLUME:
                    opt->current_selection = PR::OPTION_MASTER_VOLUME; break;
                case PR::OPTION_MUSIC_VOLUME:
                    opt->current_selection = PR::OPTION_SFX_VOLUME; break;
                case PR::OPTION_DISPLAY_MODE:
                    opt->current_selection = PR::OPTION_MUSIC_VOLUME; break;
                case PR::OPTION_RESOLUTION:
                    opt->current_selection = PR::OPTION_DISPLAY_MODE; break;
                default: break;
            }
        }

        // NOTE: Act on the selection
        // If the display option is no more selected, reset the selection (?)
        if (opt->current_selection != PR::OPTION_DISPLAY_MODE) {
            opt->display_mode_selection = win->display_mode;
        }
        if (opt->current_selection == PR::OPTION_MASTER_VOLUME) {
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                option_slider_update_value(&opt->master_volume,
                                           opt->master_volume.value - 0.05f);
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                option_slider_update_value(&opt->master_volume,
                                           opt->master_volume.value + 0.05f);
            }
            option_slider_handle_mouse(&opt->master_volume,
                                       input->mouseX, input->mouseY,
                                       input->mouse_left);
            ma_engine_set_volume(&sound->engine, opt->master_volume.value);
            sound->master_volume = opt->master_volume.value;
        } else if (opt->current_selection == PR::OPTION_SFX_VOLUME) {
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                option_slider_update_value(&opt->sfx_volume,
                                           opt->sfx_volume.value - 0.05f);
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                option_slider_update_value(&opt->sfx_volume,
                                           opt->sfx_volume.value + 0.05f);
            }
            option_slider_handle_mouse(&opt->sfx_volume,
                                       input->mouseX, input->mouseY,
                                       input->mouse_left);
            ma_sound_group_set_volume(&sound->sfx_group,
                                      opt->sfx_volume.value);
            sound->sfx_volume = opt->sfx_volume.value;
        } else if (opt->current_selection == PR::OPTION_MUSIC_VOLUME) {
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                option_slider_update_value(&opt->music_volume,
                                           opt->music_volume.value - 0.05f);
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                option_slider_update_value(&opt->music_volume,
                                           opt->music_volume.value + 0.05f);
            }
            option_slider_handle_mouse(&opt->music_volume,
                                       input->mouseX, input->mouseY,
                                       input->mouse_left);
            ma_sound_group_set_volume(&sound->music_group,
                                      opt->music_volume.value);
            sound->music_volume = opt->music_volume.value;
        } else if (opt->current_selection == PR::OPTION_DISPLAY_MODE) {
            PR::DisplayMode old_display_mode = win->display_mode;

            // Changing display mode selection with keybindings
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                switch (opt->display_mode_selection) {
                    case PR::FULLSCREEN: break;
                    case PR::BORDERLESS:
                        opt->display_mode_selection = PR::FULLSCREEN; break;
                    case PR::WINDOWED:
                        opt->display_mode_selection = PR::BORDERLESS; break;
                    default: break;
                }
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                switch (opt->display_mode_selection) {
                    case PR::FULLSCREEN:
                        opt->display_mode_selection = PR::BORDERLESS; break;
                    case PR::BORDERLESS:
                        opt->display_mode_selection = PR::WINDOWED; break;
                    case PR::WINDOWED: break;
                    default: break;
                }
            }

            if (ACTION_CLICKED(PR_MENU_CLICK)) {
                win->display_mode = opt->display_mode_selection;
            }

            // Changing display mode selection with the mouse
            if (input->was_mouse_moved || input->mouse_left.clicked) {
                if (rect_contains_point(opt->display_mode_fullscreen.body,
                            input->mouseX, input->mouseY, true)) {
                    opt->display_mode_selection = PR::FULLSCREEN;
                    if (input->mouse_left.clicked) {
                        win->display_mode = PR::FULLSCREEN;
                    }
                } else if(rect_contains_point(opt->display_mode_borderless.body,
                            input->mouseX, input->mouseY, true)) {
                    opt->display_mode_selection = PR::BORDERLESS;
                    if (input->mouse_left.clicked) {
                        win->display_mode = PR::BORDERLESS;
                    }
                } else if (rect_contains_point(opt->display_mode_windowed.body,
                            input->mouseX, input->mouseY, true)) {
                    opt->display_mode_selection = PR::WINDOWED;
                    if (input->mouse_left.clicked) {
                        win->display_mode = PR::WINDOWED;
                    }
                }
            }
            // Change the background color of the display mode buttons
            // based on which display mode is now in use
            opt->display_mode_fullscreen.col =
                (win->display_mode == PR::FULLSCREEN) ?
                OPTION_BUTTON_SELECTED_COLOR :
                OPTION_BUTTON_DEFAULT_COLOR;
            opt->display_mode_borderless.col =
                (win->display_mode == PR::BORDERLESS) ?
                OPTION_BUTTON_SELECTED_COLOR :
                OPTION_BUTTON_DEFAULT_COLOR;
            opt->display_mode_windowed.col =
                (win->display_mode == PR::WINDOWED) ?
                OPTION_BUTTON_SELECTED_COLOR :
                OPTION_BUTTON_DEFAULT_COLOR;

            // Update the display mode if it was changed
            if (win->display_mode != old_display_mode) {
                display_mode_update(win, win->display_mode);
                ma_sound_seek_to_pcm_frame(&sound->click_selected, 0);
                ma_sound_start(&sound->click_selected);
            }
        } else if (opt->current_selection == PR::OPTION_RESOLUTION) {
            // Changing window resolution with the keybindings
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                opt->resolution_selection =
                    window_resolution_prev(opt->resolution_selection);
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                opt->resolution_selection =
                    window_resolution_next(opt->resolution_selection);
            }

            // Changing window resolution with the mouse
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                if (rect_contains_point(opt->resolution_up.body,
                        input->mouseX, input->mouseY, true)) {
                    opt->resolution_selection =
                        window_resolution_next(opt->resolution_selection);
                } else if(rect_contains_point(opt->resolution_down.body,
                            input->mouseX, input->mouseY, true)) {
                    opt->resolution_selection =
                        window_resolution_prev(opt->resolution_selection);
                }
            }

            // Apply the changes when we click on the wanted resolution
            if ((ACTION_CLICKED(PR_MENU_CLICK) ||
                 (input->mouse_left.clicked &&
                  rect_contains_point(unsaved_resolution_background,
                     input->mouseX,
                     input->mouseY, true))) &&
                (opt->resolution_selection !=
                 window_resolution_from_dim(win->width, win->height))) {
                window_resolution_set(win, opt->resolution_selection);
            }
        } else if (opt->current_selection == PR::OPTION_NONE) {
            // do nothing
        }
    } else { // Update camera position
             // NOTE: Move menu camera based on input method used
        if (input->modified) {
            options_menu_update_bindings(opt, input->actions);
            int save_binds_ret = keybindings_save_to_file(
                        "./my_binds.prkeys",
                        input->actions,
                        (int) ARRAY_LENGTH(input->actions));
            if (save_binds_ret) {
                std::cout << "Could not save keybindings to file ./my_binds.prkeys: "
                          << save_binds_ret
                          << std::endl;
            } else {
                std::cout << "Saved keybindings to file ./my_binds.prkeys"
                          << std::endl;
            }
            input->modified = false;
        }

        bool was_selection_moved = false;

        int previous_selected_row = opt->selected_row;
        int previous_selected_column = opt->selected_column;

        if (!input->kb_binding && !input->gp_binding) {
            if (ACTION_CLICKED(PR_MENU_UP)) {
                if (opt->selected_row > 0) {
                    opt->selected_row--;
                }
            }
            if (ACTION_CLICKED(PR_MENU_DOWN)) {
                if (opt->selected_row < ARRAY_LENGTH(input->actions)-1) {
                    opt->selected_row++;
                }
            }
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                if (opt->selected_column > 0) {
                    opt->selected_column--;
                }
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                if (opt->selected_column < 4 - 1) {
                    opt->selected_column++;
                }
            }
        }

        for(int bind_index = 0;
            bind_index < ARRAY_LENGTH(opt->change_kb_binds1);
            ++bind_index) {

            PR::Button *kb1 = &opt->change_kb_binds1[bind_index];
            PR::Button *kb2 = &opt->change_kb_binds2[bind_index];
            PR::Button *gp1 = &opt->change_gp_binds1[bind_index];
            PR::Button *gp2 = &opt->change_gp_binds2[bind_index];

            InputAction *action = &input->actions[bind_index];

            if (!input->kb_binding && !input->gp_binding) {
                if (rect_contains_point(
                            rect_in_menu_camera_space(kb1->body, cam),
                            input->mouseX, input->mouseY,
                            kb1->from_center)) {

                    if (input->was_mouse_moved) {
                        opt->selected_row = bind_index;
                        opt->selected_column = 0;
                    }
                    if (input->mouse_left.clicked) {
                        input->kb_binding = &action->kb_binds[0];
                        glfwSetKeyCallback(win->glfw_win,
                                kb_change_binding_callback);
                    }
                }
                if (rect_contains_point(
                            rect_in_menu_camera_space(kb2->body, cam),
                            input->mouseX, input->mouseY,
                            kb2->from_center)) {

                    if (input->was_mouse_moved) {
                        opt->selected_row = bind_index;
                        opt->selected_column = 1;
                    }
                    if (input->mouse_left.clicked) {
                        input->kb_binding = &action->kb_binds[1];
                        glfwSetKeyCallback(win->glfw_win,
                                kb_change_binding_callback);
                    }
                }
                if (rect_contains_point(
                            rect_in_menu_camera_space(gp1->body, cam),
                            input->mouseX, input->mouseY,
                            gp1->from_center)) {

                    if (input->was_mouse_moved) {
                        opt->selected_row = bind_index;
                        opt->selected_column = 2;
                    }
                    if (input->mouse_left.clicked) {
                        input->gp_binding = &action->gp_binds[0];
                    }
                }
                if (rect_contains_point(
                            rect_in_menu_camera_space(gp2->body, cam),
                            input->mouseX, input->mouseY,
                            gp2->from_center)) {

                    if (input->was_mouse_moved) {
                        opt->selected_row = bind_index;
                        opt->selected_column = 3;
                    }
                    if (input->mouse_left.clicked) {
                        input->gp_binding = &action->gp_binds[1];
                    }
                }
            }

            kb1->col = OPTION_BUTTON_DEFAULT_COLOR;
            kb2->col = OPTION_BUTTON_DEFAULT_COLOR;
            gp1->col = OPTION_BUTTON_DEFAULT_COLOR;
            gp2->col = OPTION_BUTTON_DEFAULT_COLOR;

            if (opt->selected_row == bind_index) {
                switch (opt->selected_column) {
                    case 0:
                        kb1->col = OPTION_BUTTON_SELECTED_COLOR;
                        break;
                    case 1:
                        kb2->col = OPTION_BUTTON_SELECTED_COLOR;
                        break;
                    case 2:
                        gp1->col = OPTION_BUTTON_SELECTED_COLOR;
                        break;
                    case 3:
                        gp2->col = OPTION_BUTTON_SELECTED_COLOR;
                        break;
                }
            }

            if (opt->selected_row == bind_index &&
                    ACTION_CLICKED(PR_MENU_CLICK)) {
                switch (opt->selected_column) {
                    case 0:
                        input->kb_binding = &action->kb_binds[0];
                        glfwSetKeyCallback(win->glfw_win,
                                kb_change_binding_callback);
                        break;
                    case 1:
                        input->kb_binding = &action->kb_binds[1];
                        glfwSetKeyCallback(win->glfw_win,
                                kb_change_binding_callback);
                        break;
                    case 2:
                        input->gp_binding = &action->gp_binds[0];
                        break;
                    case 3:
                        input->gp_binding = &action->gp_binds[1];
                        break;
                    default: break;
                }
            }
        }

        if (opt->selected_row != previous_selected_row ||
                opt->selected_column != previous_selected_column) {
            was_selection_moved = true;
        }

        // ### Camera movement ###
        if (cam->follow_selection) {
            if (opt->selected_row != -1) {
                float selected_goal_position =
                    (opt->selected_row + 1) * ((float)GAME_HEIGHT / 6) +
                        ((float)GAME_HEIGHT / 12);
                cam->goal_position =
                    (selected_goal_position > GAME_HEIGHT * 0.5f) ?
                        selected_goal_position : GAME_HEIGHT * 0.5f;

            } else {
                cam->goal_position = GAME_HEIGHT * 0.5f;
            }
            if (input->was_mouse_moved) {
                cam->follow_selection = false;
            }
        } else {
            // NOTE: Consider the cursor only if it's inside the window
            if (0 < input->mouseX && input->mouseX < GAME_WIDTH &&
                    0 < input->mouseY && input->mouseY < GAME_HEIGHT) {

                // NOTE: 0.2f is an arbitrary amount
                if (cam->goal_position > GAME_HEIGHT*0.5f &&
                        input->mouseY < GAME_HEIGHT * 0.2f) {
                    float cam_velocity =
                        (1.f - (input->mouseY / (GAME_HEIGHT*0.20))) *
                        CAMERA_MAX_VELOCITY;
                    cam->goal_position -=
                        cam_velocity * glob->state.delta_time;
                }

                // NOTE: 15 because there can be 15 buttons in the screen together
                //       +2 because i do:
                //          +3 to count for the empty row left at the top
                //          -1 so that when a row is exactly filled at the end
                //              it doesn't count that as another row
                size_t last_row_y =
                    (ARRAY_LENGTH(input->actions) + 1) *
                        ((float)GAME_HEIGHT / 6) +
                    ((float)GAME_HEIGHT / 12);

                if (cam->goal_position < last_row_y &&
                        input->mouseY > GAME_HEIGHT * 0.8f) {
                    float cam_velocity =
                        ((input->mouseY - GAME_HEIGHT*0.8f) /
                            (GAME_HEIGHT*0.20)) *
                        CAMERA_MAX_VELOCITY;
                    cam->goal_position +=
                        cam_velocity * glob->state.delta_time;
                }
            }
            // NOTE: Check if you have to change camera movement mode
            if (was_selection_moved &&
                    (ACTION_CLICKED(PR_MENU_UP) ||
                     ACTION_CLICKED(PR_MENU_DOWN) ||
                     ACTION_CLICKED(PR_MENU_LEFT) ||
                     ACTION_CLICKED(PR_MENU_RIGHT))) {
                cam->follow_selection = true;
            }
        }
        cam->pos.y = lerp(cam->pos.y, cam->goal_position,
                glob->state.delta_time * cam->speed_multiplier);
    }

    // Rendering the background
    Rect full_screen = {
        .pos = glm::vec2(0.f, 0.f),
        .dim = glm::vec2(GAME_WIDTH, GAME_HEIGHT),
        .angle = 0.f,
        .triangle = false,
    };
    renderer_add_queue_uni(
            full_screen,
            glm::vec4(0.3f, 0.8f, 0.9f, 1.0f),
            false);

    // render the `go_to_start_menu` button
    button_render_in_menu_camera(opt->to_start_menu, glm::vec4(1.f),
                                 &glob->rend_res.fonts[0], cam);

    // render GENERAL and CONTROLS buttons
    button_render_in_menu_camera(opt->to_general_pane, glm::vec4(1.f),
                                 &glob->rend_res.fonts[0], cam);
    button_render_in_menu_camera(opt->to_controls_pane, glm::vec4(1.f),
                                 &glob->rend_res.fonts[0], cam);

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    if (opt->showing_general_pane) {
        glm::vec4 color = glm::vec4(1.f);
        // Master volume rendering
        color = (opt->current_selection == PR::OPTION_MASTER_VOLUME) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                opt->master_volume.background.pos.y,
                                opt->master_volume.label, color,
                                &glob->rend_res.fonts[0], true);
        option_slider_render(&opt->master_volume, color);
        // Sfx volume rendering
        color = (opt->current_selection == PR::OPTION_SFX_VOLUME) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                opt->sfx_volume.background.pos.y,
                                opt->sfx_volume.label, color,
                                &glob->rend_res.fonts[0], true);
        option_slider_render(&opt->sfx_volume, color);
        // Music volume rendering
        color = (opt->current_selection == PR::OPTION_MUSIC_VOLUME) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                opt->music_volume.background.pos.y,
                                opt->music_volume.label, color,
                                &glob->rend_res.fonts[0], true);
        option_slider_render(&opt->music_volume, color);

        // Resolution changing rendering
        color = (opt->current_selection == PR::OPTION_RESOLUTION) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                opt->resolution_up.body.pos.y,
                                "RESOLUTION", color,
                                &glob->rend_res.fonts[0], true);
        renderer_add_queue_text(
                GAME_WIDTH * 0.75f,
                opt->resolution_up.body.pos.y,
                window_resolution_to_str(opt->resolution_selection),
                color,
                &glob->rend_res.fonts[0], true);
        if (opt->resolution_selection !=
                window_resolution_from_dim(win->width, win->height)) {
            renderer_add_queue_uni(unsaved_resolution_background,
                                   OPTION_BUTTON_DEFAULT_COLOR,
                                   true);
        }
        button_render(opt->resolution_up,
                      color, &glob->rend_res.fonts[0]);
        button_render(opt->resolution_down,
                      color, &glob->rend_res.fonts[0]);

        // Display mode rendering
        color = (opt->current_selection == PR::OPTION_DISPLAY_MODE) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                opt->display_mode_fullscreen.body.pos.y,
                                "DISPLAY MODE", color,
                                &glob->rend_res.fonts[0], true);
        // Draw stuff now because I'm gonna change the font
        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

        color = (opt->current_selection == PR::OPTION_DISPLAY_MODE &&
                 opt->display_mode_selection == PR::FULLSCREEN) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        button_render(opt->display_mode_fullscreen,
                      color, &glob->rend_res.fonts[1]);
        color = (opt->current_selection == PR::OPTION_DISPLAY_MODE &&
                 opt->display_mode_selection == PR::BORDERLESS) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        button_render(opt->display_mode_borderless,
                      color, &glob->rend_res.fonts[1]);
        color = (opt->current_selection == PR::OPTION_DISPLAY_MODE &&
                 opt->display_mode_selection == PR::WINDOWED) ?
                OPTION_SLIDER_SELECTED_COLOR : OPTION_SLIDER_DEFAULT_COLOR;
        button_render(opt->display_mode_windowed,
                      color, &glob->rend_res.fonts[1]);
        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[1], glob->rend_res.shaders[2]);
    } else {
        // renderer_add_queue_text(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f,
        //                         "CONTROLS PANE", glm::vec4(1.0f),
        //                         &glob->rend_res.fonts[0], true);

        // NOTE: Render action labels
        for(size_t bind_index = 0;
            bind_index < ARRAY_LENGTH(opt->change_kb_binds1);
            ++bind_index) {

            PR::Button *kb1 = &opt->change_kb_binds1[bind_index];
            Rect in_cam_rect = rect_in_menu_camera_space(kb1->body, cam);

            renderer_add_queue_text(GAME_WIDTH * 0.25f,
                                    in_cam_rect.pos.y,
                                    get_action_name(bind_index),
                                    glm::vec4(1.0f),
                                    &glob->rend_res.fonts[ACTION_NAME_FONT],
                                    true);
        }
        renderer_draw_text(&glob->rend_res.fonts[ACTION_NAME_FONT],
                           glob->rend_res.shaders[2]);

        // NOTE: Render action buttons and text inside of them
        for(size_t bind_index = 0;
            bind_index < ARRAY_LENGTH(opt->change_kb_binds1);
            ++bind_index) {

            PR::Button *kb1 = &opt->change_kb_binds1[bind_index];
            PR::Button *kb2 = &opt->change_kb_binds2[bind_index];
            PR::Button *gp1 = &opt->change_gp_binds1[bind_index];
            PR::Button *gp2 = &opt->change_gp_binds2[bind_index];

            button_render_in_menu_camera(*kb1, glm::vec4(1.f),
                                         &glob->rend_res.fonts[1], cam);
            button_render_in_menu_camera(*kb2, glm::vec4(1.f),
                                         &glob->rend_res.fonts[1], cam);
            button_render_in_menu_camera(*gp1, glm::vec4(1.f),
                                         &glob->rend_res.fonts[1], cam);
            button_render_in_menu_camera(*gp2, glm::vec4(1.f),
                                         &glob->rend_res.fonts[1], cam);
        }
        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[1],
                            glob->rend_res.shaders[2]);
    }
}

int play_menu_prepare(PR::PlayMenu *menu) {
    // NOTE: By not setting this, it will remain the same as before
    //          starting the level
    //menu->showing_campaign_buttons = true;

    menu->to_start_menu = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH / 30.f, GAME_HEIGHT / 22.5f),
            .dim = glm::vec2(GAME_WIDTH / 20.f, GAME_HEIGHT / 15.f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_DEFAULT_COLOR,
        .text = "<-"
    };

    menu->selected_campaign_button = 0;
    menu->selected_custom_button = 0;

    // NOTE: Button to select which buttons to show
    PR::Button *campaign = &menu->show_campaign_button;
    campaign->body.pos.x = (GAME_WIDTH * 3.f) / 10.f;
    campaign->body.pos.y = GAME_HEIGHT / 10.f;
    campaign->body.dim.x = GAME_WIDTH / 3.f;
    campaign->body.dim.y = GAME_HEIGHT / 8.f;
    campaign->body.angle = 0.f;
    campaign->body.triangle = false;
    campaign->col =
        menu->showing_campaign_buttons ? SHOW_BUTTON_SELECTED_COLOR :
                                         SHOW_BUTTON_DEFAULT_COLOR;
    campaign->from_center = true;
    std::snprintf(campaign->text, strlen("CAMPAIGN")+1, "CAMPAIGN");

    PR::Button *custom = &menu->show_custom_button;
    custom->body.pos.x = (GAME_WIDTH * 7.f) / 10.f;
    custom->body.pos.y = GAME_HEIGHT / 10.f;
    custom->body.dim.x = GAME_WIDTH / 3.f;
    custom->body.dim.y = GAME_HEIGHT / 8.f;
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

        PR::LevelButton *lb =
            &menu->campaign_buttons[levelbutton_index];

        button_set_position(&lb->button, levelbutton_index);

        lb->is_new_level = false;

        int size = std::snprintf(nullptr, 0, "LEVEL %zu", levelbutton_index+1);
        assert((size+1 <= ARRAY_LENGTH(lb->button.text))
                && "Text buffer is too little!");
        std::snprintf(lb->button.text, size+1,
                      "LEVEL %zu", levelbutton_index+1);
        if (levelbutton_index < ARRAY_LENGTH(campaign_levels_filepath)) {
            assert((std::strlen(campaign_levels_filepath[levelbutton_index]) <
                        ARRAY_LENGTH(lb->mapfile_path))
                    && "Map file too big for button mapfile buffer!");
            std::snprintf(
                    lb->mapfile_path,
                    std::strlen(campaign_levels_filepath[levelbutton_index])+1,
                    "%s", campaign_levels_filepath[levelbutton_index]);
        } else {
            std::snprintf(lb->mapfile_path, 1, "");
        }
    }

    // NOTE: Custom levels buttons
    if (menu->showing_campaign_buttons) {
        da_clear(&menu->custom_buttons);
    } else {
        int result = 0;
        result =
            load_custom_buttons_from_dir("./custom_maps/",
                                         &menu->custom_buttons);
        if (result != 0) {
            std::cout << "[ERROR] Could not load custom map files"
                      << std::endl;

            return result;
        }
        PR::Button *add_level = &menu->add_custom_button;
        button_set_position(add_level, menu->custom_buttons.count);
        assert(std::strlen("+")+1 <= ARRAY_LENGTH(add_level->text)
                && "Text bigger than button text buffer!");
        std::snprintf(add_level->text, std::strlen("+")+1, "%s", "+");
    }

    PR::MenuCamera *cam = &menu->camera;
    cam->pos.x = GAME_WIDTH * 0.5f;
    cam->pos.y = GAME_HEIGHT * 0.5f;
    cam->speed_multiplier = 6.f;
    cam->goal_position = cam->pos.y;

    menu->deleting_frame = {
        .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f),
        .dim = glm::vec2(GAME_WIDTH * 0.6f, GAME_HEIGHT * 0.6f),
        .angle = 0.f,
        .triangle = false,
    };

    PR::Button *yes = &menu->delete_yes;
    *yes = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.35f, GAME_HEIGHT * 0.6f),
            .dim = glm::vec2(GAME_WIDTH * 0.2f, GAME_HEIGHT * 0.1f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_DEFAULT_COLOR,
    };
    std::snprintf(yes->text, std::strlen("YES")+1, "%s", "YES");

    PR::Button *no = &menu->delete_no;
    *no = {
        .from_center = true,
        .body = {
            .pos = glm::vec2(GAME_WIDTH * 0.65f, GAME_HEIGHT * 0.6f),
            .dim = glm::vec2(GAME_WIDTH * 0.2f, GAME_HEIGHT * 0.1f),
            .angle = 0.f,
            .triangle = false,
        },
        .col = LEVEL_BUTTON_DEFAULT_COLOR,
    };
    std::snprintf(no->text, std::strlen("NO")+1, "%s", "NO");

    // NOTE: Make the cursor show
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    PR::Sound *sound = &glob->sound;
    if (!ma_sound_is_playing(&sound->menu_music)) {
        ma_sound_seek_to_pcm_frame(&sound->menu_music, 0);
        ma_sound_start(&sound->menu_music);
    }
    return 0;
}
void play_menu_update(void) {
    InputController *input = &glob->input;
    PR::PlayMenu *menu = &glob->current_play_menu;
    PR::MenuCamera *cam = &menu->camera;
    PR::Sound *sound = &glob->sound;

    if (!menu->showing_campaign_buttons &&
         (ACTION_CLICKED(PR_MENU_PANE_LEFT) ||
          (input->mouse_left.clicked &&
           rect_contains_point(
                rect_in_menu_camera_space(menu->show_campaign_button.body, cam),
                input->mouseX, input->mouseY,
                menu->show_campaign_button.from_center)))) {
        menu->showing_campaign_buttons = true;
        menu->show_campaign_button.col = SHOW_BUTTON_SELECTED_COLOR;
        menu->show_custom_button.col = SHOW_BUTTON_DEFAULT_COLOR;
        cam->goal_position = GAME_HEIGHT * 0.5f;
        menu->deleting_level = false;
        ma_sound_seek_to_pcm_frame(&sound->change_pane, 0);
        ma_sound_start(&sound->change_pane);
    }
    if (ACTION_CLICKED(PR_MENU_PANE_RIGHT) ||
        (input->mouse_left.clicked &&
         rect_contains_point(
             rect_in_menu_camera_space(menu->show_custom_button.body, cam),
             input->mouseX, input->mouseY,
             menu->show_custom_button.from_center))) {
        int result = 0;
        PR::CustomLevelButtons temp_custom_buttons = {NULL, 0, 0};
        result =
            load_custom_buttons_from_dir("./custom_maps/",
                                         &temp_custom_buttons);
        if (result == 0) {
            // NOTE: Free the previous present custom buttons
            da_clear(&menu->custom_buttons);
            // NOTE: Reassign the updated and checked values
            menu->custom_buttons = temp_custom_buttons;
            menu->showing_campaign_buttons = false;
            menu->show_campaign_button.col = SHOW_BUTTON_DEFAULT_COLOR;
            menu->show_custom_button.col = SHOW_BUTTON_SELECTED_COLOR;
            cam->goal_position = GAME_HEIGHT * 0.5f;

            if (menu->selected_custom_button >
                    (int)menu->custom_buttons.count) {
                menu->selected_custom_button = menu->custom_buttons.count;
            }

            PR::Button *add_level = &menu->add_custom_button;
            button_set_position(add_level, menu->custom_buttons.count);
            assert(std::strlen("+")+1 <= ARRAY_LENGTH(add_level->text)
                    && "Text bigger than button text buffer!");
            std::snprintf(add_level->text, std::strlen("+")+1, "%s", "+");
        } else {
            std::cout << "[ERROR] Could not load custom map files"
                      << std::endl;
        }
        // if (ma_sound_is_playing(&sound->change_pane)) {
        //     ma_sound_seek_to_pcm_frame(&sound->change_pane, 0);
        // }
        ma_sound_seek_to_pcm_frame(&sound->change_pane, 0);
        ma_sound_start(&sound->change_pane);
    }

    bool was_selection_moved = false;

    if (menu->showing_campaign_buttons) {
        // NOTE: The keybings are put before because
        //       the mouse has the priority if it was moved

        int previous_campaign_selection = menu->selected_campaign_button;
        // Keybinding
        if (ACTION_CLICKED(PR_MENU_UP)) {
            if (menu->selected_campaign_button-3 >= 0) {
                menu->selected_campaign_button -= 3;
            } else {
                menu->selected_campaign_button = 0;
            }
        }
        if (ACTION_CLICKED(PR_MENU_DOWN)) {
            if (menu->selected_campaign_button+3 <
                    ARRAY_LENGTH(menu->campaign_buttons)) {
                menu->selected_campaign_button += 3;
            } else {
                menu->selected_campaign_button = ARRAY_LENGTH(menu->campaign_buttons)-1;
            }
        }
        if (ACTION_CLICKED(PR_MENU_LEFT)) {
            if (menu->selected_campaign_button-1 >= 0) {
                menu->selected_campaign_button--;
            }
        }
        if (ACTION_CLICKED(PR_MENU_RIGHT)) {
            if (menu->selected_campaign_button+1 <
                    ARRAY_LENGTH(menu->campaign_buttons)) {
                menu->selected_campaign_button++;
            }
        }
        for(int levelbutton_index = 0;
            levelbutton_index < ARRAY_LENGTH(menu->campaign_buttons);
            ++levelbutton_index) {

            PR::LevelButton *lb =
                &menu->campaign_buttons[levelbutton_index];

            // Mouse
            if (input->was_mouse_moved) {
                if (rect_contains_point(rect_in_menu_camera_space(lb->button.body,
                                cam),
                            input->mouseX, input->mouseY,
                            lb->button.from_center)) {
                    menu->selected_campaign_button = levelbutton_index;
                } else if (menu->selected_campaign_button >= 0 &&
                        menu->selected_campaign_button == levelbutton_index) {
                    // NOTE: If this is uncommented then if the mouse
                    //          is moved outside of a button, no button will
                    //          be selected
                    // menu->selected_campaign_button = -1;
                }
            }
            // Keybinding, with selection
            if (menu->selected_campaign_button >= 0 &&
                menu->selected_campaign_button == levelbutton_index) {

                lb->button.col = LEVEL_BUTTON_SELECTED_COLOR;
                if (ACTION_CLICKED(PR_MENU_CLICK) ||
                    (input->mouse_left.clicked &&
                     rect_contains_point(rect_in_menu_camera_space(lb->button.body,
                                                              cam),
                                         input->mouseX, input->mouseY,
                                         lb->button.from_center))) {
                    CHANGE_CASE_TO_LEVEL(
                            lb->mapfile_path, lb->button.text,
                            false, lb->is_new_level, void());
                }
            } else {
                lb->button.col = LEVEL_BUTTON_DEFAULT_COLOR;
            }
        }
        if (menu->selected_campaign_button != -1 &&
            previous_campaign_selection != menu->selected_campaign_button) {

            was_selection_moved = true;
            ma_sound_seek_to_pcm_frame(&sound->change_selection, 0);
            ma_sound_start(&sound->change_selection);
        }
    } else {
        if (menu->deleting_level) {
            PR::CustomLevelButton deleted_lb =
                menu->custom_buttons.items[menu->deleting_index];

            // mouse selection
            if (rect_contains_point(menu->delete_yes.body,
                                    input->mouseX, input->mouseY,
                                    menu->delete_yes.from_center)) {
                menu->delete_selection = PR::BUTTON_YES;
            }
            if (input->was_mouse_moved &&
                rect_contains_point(menu->delete_no.body,
                                    input->mouseX, input->mouseY,
                                    menu->delete_no.from_center)) {
                menu->delete_selection = PR::BUTTON_NO;
            }

            // keybinding selection
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                menu->delete_selection = PR::BUTTON_YES;
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                menu->delete_selection = PR::BUTTON_NO;
            }

            // acting on the selection
            if (menu->delete_selection == PR::BUTTON_YES) {
                menu->delete_yes.col = LEVEL_BUTTON_SELECTED_COLOR;
                menu->delete_no.col = LEVEL_BUTTON_DEFAULT_COLOR;
                if (ACTION_CLICKED(PR_MENU_CLICK) ||
                    (input->mouse_left.clicked &&
                     rect_contains_point(menu->delete_yes.body,
                                         input->mouseX, input->mouseY,
                                         menu->delete_yes.from_center))) {
                    if (!deleted_lb.is_new_level &&
                            std::remove(deleted_lb.mapfile_path)) {
                        std::cout << "[ERROR]: Could not delete the mapfile: "
                                  << deleted_lb.mapfile_path
                                  << std::endl;
                    }

                    std::cout << "REMOVED LEVEL "
                              << menu->deleting_index << std::endl;

                    da_remove(&menu->custom_buttons, menu->deleting_index);

                    for(size_t repos_index = 0;
                        repos_index < menu->custom_buttons.count;
                        ++repos_index) {

                        PR::CustomLevelButton *but =
                            &menu->custom_buttons.items[repos_index];

                        button_set_position(&but->button, repos_index);
                        button_edit_del_to_lb(&but->button,
                                              &but->edit, &but->del);
                    }

                    PR::Button *add_level = &menu->add_custom_button;
                    button_set_position(add_level, menu->custom_buttons.count);

                    menu->deleting_level = false;

                    if (menu->selected_custom_button >=
                            (int) menu->custom_buttons.count) {
                        menu->selected_custom_button =
                            menu->custom_buttons.count-1;
                    }

                    // TODO: Sound when a level is deleted
                    ma_sound_seek_to_pcm_frame(&sound->delete_level, 0);
                    ma_sound_start(&sound->delete_level);
                }
            } else if (menu->delete_selection == PR::BUTTON_NO) {
                menu->delete_no.col = LEVEL_BUTTON_SELECTED_COLOR;
                menu->delete_yes.col = LEVEL_BUTTON_DEFAULT_COLOR;
                if (ACTION_CLICKED(PR_MENU_CLICK) ||
                    (input->mouse_left.clicked &&
                     rect_contains_point(menu->delete_no.body,
                                         input->mouseX, input->mouseY,
                                         menu->delete_no.from_center))) {
                    menu->deleting_level = false;
                }
            }
        } else {
            int previous_custom_selection = menu->selected_custom_button;
            // Keybinding
            if (ACTION_CLICKED(PR_MENU_UP)) {
                if (menu->selected_custom_button-3 >= 0) {
                    menu->selected_custom_button -= 3;
                } else {
                    menu->selected_custom_button = 0;
                }
            }
            if (ACTION_CLICKED(PR_MENU_DOWN)) {
                if (menu->selected_custom_button+3 <
                        (int) menu->custom_buttons.count+1) {
                    menu->selected_custom_button += 3;
                } else {
                    menu->selected_custom_button = menu->custom_buttons.count;
                }
            }
            if (ACTION_CLICKED(PR_MENU_LEFT)) {
                if (menu->selected_custom_button-1 >= 0) {
                    menu->selected_custom_button--;
                }
            }
            if (ACTION_CLICKED(PR_MENU_RIGHT)) {
                if (menu->selected_custom_button+1 <
                        (int)menu->custom_buttons.count+1) {
                    menu->selected_custom_button++;
                }
            }
            for(size_t custombutton_index = 0;
                custombutton_index < menu->custom_buttons.count;
                ++custombutton_index) {

                PR::Button *edit_lb =
                    &menu->custom_buttons.items[custombutton_index].edit;
                PR::Button *del_lb =
                    &menu->custom_buttons.items[custombutton_index].del;
                PR::CustomLevelButton *lb =
                    &menu->custom_buttons.items[custombutton_index];

                if (rect_contains_point(
                            rect_in_menu_camera_space(del_lb->body, cam),
                            input->mouseX, input->mouseY,
                            del_lb->from_center)) {
                    if (input->was_mouse_moved) {
                        del_lb->col = DEL_BUTTON_SELECTED_COLOR;
                        edit_lb->col = EDIT_BUTTON_DEFAULT_COLOR;
                        menu->selected_custom_button = custombutton_index;
                    }

                    if (input->mouse_left.clicked) {
                        menu->deleting_level = true;
                        menu->deleting_index = custombutton_index;
                        menu->delete_selection = PR::BUTTON_NO;
                    }
                    
                } else if (rect_contains_point(
                            rect_in_menu_camera_space(edit_lb->body, cam),
                            input->mouseX, input->mouseY,
                            edit_lb->from_center)) {
                    
                    if (input->was_mouse_moved) {
                        edit_lb->col = EDIT_BUTTON_SELECTED_COLOR;
                        del_lb->col = DEL_BUTTON_DEFAULT_COLOR;
                        menu->selected_custom_button = custombutton_index;
                    }

                    if (input->mouse_left.clicked) {
                        CHANGE_CASE_TO_LEVEL(
                                lb->mapfile_path, lb->button.text,
                                true, lb->is_new_level, void());
                    }
                } else {
                    edit_lb->col = EDIT_BUTTON_DEFAULT_COLOR;
                    del_lb->col = DEL_BUTTON_DEFAULT_COLOR;
                }

                if (input->was_mouse_moved) {
                    if (rect_contains_point(
                                rect_in_menu_camera_space(lb->button.body, cam),
                                input->mouseX, input->mouseY,
                                lb->button.from_center)) {
                        
                        menu->selected_custom_button = custombutton_index;
                    } else {
                        // NOTE: if this is uncommented then if the mouse
                        //          is moved outside of a button,
                        //          no button will be selected
                        // menu->selected_custom_button = -1;
                    }
                }
                if (menu->selected_custom_button >= 0 &&
                    menu->selected_custom_button ==
                        (int)custombutton_index) {
                    
                    lb->button.col = LEVEL_BUTTON_SELECTED_COLOR;
                    if (ACTION_CLICKED(PR_MENU_CLICK) ||
                        (input->mouse_left.clicked &&
                         rect_contains_point(
                            rect_in_menu_camera_space(lb->button.body, cam),
                            input->mouseX, input->mouseY,
                            lb->button.from_center))) {

                        CHANGE_CASE_TO_LEVEL(
                                lb->mapfile_path, lb->button.text,
                                false, lb->is_new_level, void());
                    } else if (ACTION_CLICKED(PR_MENU_LEVEL_DELETE)){
                        menu->deleting_level = true;
                        menu->deleting_index = custombutton_index;
                        menu->delete_selection = PR::BUTTON_NO;
                    } else if (ACTION_CLICKED(PR_MENU_LEVEL_EDIT)) {
                        CHANGE_CASE_TO_LEVEL(
                                lb->mapfile_path, lb->button.text,
                                true, lb->is_new_level, void());
                    }
                } else {
                    lb->button.col = LEVEL_BUTTON_DEFAULT_COLOR;
                }
            }
            // NOTE: Checking if the add custom level button is pressed
            PR::Button *add_level = &menu->add_custom_button;

            if (input->was_mouse_moved) {
                if (rect_contains_point(
                        rect_in_menu_camera_space(add_level->body, cam),
                        input->mouseX, input->mouseY,
                        add_level->from_center)) {
                    menu->selected_custom_button = menu->custom_buttons.count;
                } else {
                    // NOTE: if this is uncommented then if the mouse
                    //          is moved outside of a button,
                    //          no button will be selected
                    // menu->selected_custom_button = -1;
                }
            }
            if (menu->selected_custom_button ==
                    (int) menu->custom_buttons.count) {
                add_level->col = LEVEL_BUTTON_SELECTED_COLOR;
                if (ACTION_CLICKED(PR_MENU_CLICK) ||
                    (input->mouse_left.clicked &&
                     rect_contains_point(
                         rect_in_menu_camera_space(add_level->body, cam),
                         input->mouseX, input->mouseY,
                         add_level->from_center))) {
                    bool found = true;
                    size_t map_number = 1;
                    int map_name_size;
                    char map_name[99];
                    do {
                        found = true;
                        map_name_size =
                            std::snprintf(nullptr, 0, "map_%zu", map_number);
                        std::snprintf(map_name, map_name_size+1,
                                      "map_%zu", map_number);
                        for(size_t lb_index = 0;
                            lb_index < menu->custom_buttons.count;
                            ++lb_index) {

                            if (std::strcmp(map_name,
                                 menu->custom_buttons.items[lb_index].button.text) == 0) {
                                found = false;
                                break;
                            }
                        }
                    } while (!found && map_number++ < 1000);

                    if (map_number < 1000) {
                        PR::CustomLevelButton new_lb;
                        button_set_position(&new_lb.button,
                                            menu->custom_buttons.count);
                        new_lb.is_new_level = true;

                        std::snprintf(new_lb.button.text, map_name_size+1,
                                      "%s", map_name);

                        uint32_t random_id = (uint32_t)
                            (((float)std::rand() / (float)RAND_MAX) * 999999) +1;

                        // Level map file path
                        int path_size =
                            std::snprintf(nullptr, 0,
                                          "./custom_maps/map%zu-%.06u.prmap",
                                          map_number, random_id);
                        assert((path_size+1 <=
                                    ARRAY_LENGTH(new_lb.mapfile_path))
                                && "Mapfile path buffer is too little!");
                        std::snprintf(new_lb.mapfile_path, path_size+1,
                                      "./custom_maps/map%zu-%.06u.prmap",
                                      map_number, random_id);

                        button_edit_del_to_lb(&new_lb.button,
                                              &new_lb.edit, &new_lb.del);

                        da_append(&menu->custom_buttons,
                                  new_lb, PR::CustomLevelButton);

                        button_set_position(add_level,
                                            menu->custom_buttons.count);
                    }
                }
            } else {
                add_level->col = LEVEL_BUTTON_DEFAULT_COLOR;
            }

            if (menu->selected_custom_button != -1 &&
                previous_custom_selection != menu->selected_custom_button) {

                was_selection_moved = true;
                ma_sound_seek_to_pcm_frame(&sound->change_selection, 0);
                ma_sound_start(&sound->change_selection);
            }
        }
    }

    // NOTE: Move menu camera based on input method used
    if (cam->follow_selection) {
        int selected_index =
            menu->showing_campaign_buttons ? menu->selected_campaign_button :
                                             menu->selected_custom_button;

        if (selected_index != -1) {
            size_t row = selected_index / 3;
            float selected_goal_position =
                (row * 2.f + 3.f) / 10.f * GAME_HEIGHT;
            cam->goal_position =
                (selected_goal_position > GAME_HEIGHT * 0.5f) ?
                    selected_goal_position : GAME_HEIGHT * 0.5f;

        } else {
            cam->goal_position = GAME_HEIGHT * 0.5f;
        }
        if (input->was_mouse_moved) {
            cam->follow_selection = false;
        }
    } else {
        size_t buttons_shown_number =
            menu->showing_campaign_buttons ? CAMPAIGN_LEVELS_NUMBER :
                                             menu->custom_buttons.count;
        // NOTE: Consider the cursor only if it's inside the window
        if (0 < input->mouseX && input->mouseX < GAME_WIDTH &&
            0 < input->mouseY && input->mouseY < GAME_HEIGHT) {

            // NOTE: 0.2f is an arbitrary amount
            if (cam->goal_position > GAME_HEIGHT*0.5f &&
                input->mouseY < GAME_HEIGHT * 0.2f) {
                float cam_velocity =
                    (1.f - (input->mouseY / (GAME_HEIGHT*0.20))) *
                                     CAMERA_MAX_VELOCITY;
                cam->goal_position -=
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

            if (cam->goal_position < GAME_HEIGHT*screens_of_buttons &&
                input->mouseY > GAME_HEIGHT * 0.8f) {
                float cam_velocity =
                    ((input->mouseY - GAME_HEIGHT*0.8f) / (GAME_HEIGHT*0.20)) *
                    CAMERA_MAX_VELOCITY;
                cam->goal_position +=
                    cam_velocity * glob->state.delta_time;
            }
        }
        // NOTE: Check if you have to change camera movement mode
        if (was_selection_moved &&
                (ACTION_CLICKED(PR_MENU_UP) ||
                 ACTION_CLICKED(PR_MENU_DOWN) ||
                 ACTION_CLICKED(PR_MENU_LEFT) ||
                 ACTION_CLICKED(PR_MENU_RIGHT))) {
            cam->follow_selection = true;
        }
    }
    cam->pos.y = lerp(cam->pos.y, cam->goal_position,
         glob->state.delta_time * cam->speed_multiplier);

    // # Check if going back to the start menu #
    if (ACTION_CLICKED(PR_MENU_EXIT) ||
        (input->mouse_left.clicked &&
            rect_contains_point(menu->to_start_menu.body,
                                input->mouseX, input->mouseY,
                                menu->to_start_menu.from_center))) {
        ma_sound_seek_to_pcm_frame(&sound->to_start_menu, 0);
        ma_sound_start(&sound->to_start_menu);
        CHANGE_CASE_TO_START_MENU(void());
    }

    // ### DRAWING ###
    Rect full_screen = {
        .pos = glm::vec2(0.f, 0.f),
        .dim = glm::vec2(GAME_WIDTH, GAME_HEIGHT),
        .angle = 0.f,
        .triangle = false,
    };
    renderer_add_queue_uni(
            full_screen,
            glm::vec4(0.3f, 0.8f, 0.9f, 1.0f),
            false);

    button_render_in_menu_camera(menu->show_campaign_button,
                                 glm::vec4(1.0f),
                                 &glob->rend_res.fonts[0],
                                 cam);

    button_render_in_menu_camera(menu->show_custom_button,
                                 glm::vec4(1.0f),
                                 &glob->rend_res.fonts[0],
                                 cam);

    if (menu->showing_campaign_buttons) {
        for(size_t levelbutton_index = 0;
            levelbutton_index < CAMPAIGN_LEVELS_NUMBER;
            ++levelbutton_index) {

            PR::LevelButton *lb =
                &menu->campaign_buttons[levelbutton_index];

            button_render_in_menu_camera(lb->button,
                                         glm::vec4(1.0f),
                                         &glob->rend_res.fonts[0],
                                         cam);
        }
    } else {
        for(size_t custombutton_index = 0;
            custombutton_index < menu->custom_buttons.count;
            ++custombutton_index) {

            PR::CustomLevelButton lb =
                menu->custom_buttons.items[custombutton_index];

            Rect button_rend_rect = rect_in_menu_camera_space(lb.button.body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   lb.button.col,
                                   lb.button.from_center);
            renderer_add_queue_text(button_rend_rect.pos.x,
                                    button_rend_rect.pos.y,
                                    lb.button.text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);

            button_rend_rect = rect_in_menu_camera_space(lb.edit.body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   lb.edit.col,
                                   lb.edit.from_center);
            renderer_add_queue_text(button_rend_rect.pos.x+
                                        button_rend_rect.dim.x*0.5f,
                                    button_rend_rect.pos.y+
                                        button_rend_rect.dim.y*0.5f,
                                    lb.edit.text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);

            button_rend_rect = rect_in_menu_camera_space(lb.del.body, cam);

            renderer_add_queue_uni(button_rend_rect,
                                   lb.del.col,
                                   lb.del.from_center);
            renderer_add_queue_text(button_rend_rect.pos.x,
                                    button_rend_rect.pos.y,
                                    lb.del.text, glm::vec4(1.0f),
                                    &glob->rend_res.fonts[0], true);
        }

        // NOTE: Drawing the button to add a level
        PR::Button add_level = menu->add_custom_button;
        Rect button_rend_rect = rect_in_menu_camera_space(add_level.body, cam);
        renderer_add_queue_uni(button_rend_rect,
                               add_level.col,
                               add_level.from_center);
        renderer_add_queue_text(button_rend_rect.pos.x,
                                button_rend_rect.pos.y,
                                add_level.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);
    }

    // render the `go_to_start_menu` button
    button_render(menu->to_start_menu, glm::vec4(1.f),
                  &glob->rend_res.fonts[0]);

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    if (menu->deleting_level) {
        renderer_add_queue_uni(menu->deleting_frame,
                               glm::vec4(0.9f, 0.9f, 0.9f, 1.f), true);
        button_render(menu->delete_yes, glm::vec4(1.f),
                      &glob->rend_res.fonts[0]);
        button_render(menu->delete_no, glm::vec4(1.f),
                      &glob->rend_res.fonts[0]);
        renderer_add_queue_text(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.3f,
                                "DELETE THE LEVEL:",
                                glm::vec4(0.0f, 0.0f, 0.0f, 1.f),
                                &glob->rend_res.fonts[0], true);
        renderer_add_queue_text(
            GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.45f,
            menu->custom_buttons.items[menu->deleting_index].button.text,
            glm::vec4(0.0f, 0.0f, 0.0f, 1.f),
            &glob->rend_res.fonts[0], true);
    }
    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    return; 
}

int level_prepare(PR::Level *level,
                  const char *mapfile_path, bool is_new_level) {

    PR::WinInfo *win = &glob->window;

    PR::Plane *p = &level->plane;
    PR::Rider *rid = &level->rider;

    PR::Camera *cam = &level->camera;
    // cam->pos.x is set afterwards
    cam->pos.y = GAME_HEIGHT * 0.5f;
    cam->speed_multiplier = 3.8f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

    level->is_new = is_new_level;

    level->colors_shuffled = false;
    level_reset_colors(level);

    level->editing_now = false;

    level->adding_now = false;

    level->game_over = false;
    level->game_won = false;

    level->pause_now = false;
    level->finish_time = 0;

    level->selected = NULL;

    level->goal_line.pos.y = 0.f;
    level->goal_line.dim.x = 30.f;
    level->goal_line.dim.y = GAME_HEIGHT;
    level->goal_line.triangle = false;
    level->goal_line.angle = 0.f;

    p->crashed = false;
    p->crash_position.x = 0.f;
    p->crash_position.y = 0.f;
    p->body.dim.y = 27.f;
    p->body.dim.x = p->body.dim.y * 3.f;
    p->body.angle = 0.f;
    p->body.triangle = true;
    p->render_zone.dim.y = 32.f * 2.f;
    p->render_zone.dim.x = p->render_zone.dim.y * 0.5f * 3.f;
    p->render_zone.pos = p->body.pos +
                         (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;
    p->render_zone.triangle = false;
    p->acc.x = 0.f;
    p->acc.y = 0.f;
    p->mass = 0.008f; // kg
    // TODO: The alar surface should be somewhat proportional
    //       to the dimension of the actual rectangle
    p->alar_surface = 0.15f; // m squared
    p->current_animation = PR::Plane::IDLE_ACC;
    p->animation_countdown = 0.f;
    p->inverse = false;

    rid->crashed = false;
    rid->crash_position.x = 0.f;
    rid->crash_position.y = 0.f;
    rid->body.dim.x = 38.f;
    rid->body.dim.y = 64.f;
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
    rid->mass = 0.013f;
    rid->jump_time_elapsed = 0.f;
    rid->attach_time_elapsed = 0.f;
    rid->air_friction_acc = 120.f;
    rid->base_velocity = 0.f;
    rid->input_velocity = 0.f;
    rid->inverse = false;
    std::snprintf(level->file_path,
                  std::strlen(mapfile_path)+1,
                  "%s", mapfile_path);

    level->start_pos.dim = p->body.dim;
    level->start_pos.triangle = false;

    if (std::strcmp(mapfile_path, "")) {

        if (is_new_level) {
            level->portals = {NULL, 0, 0};
            level->obstacles = {NULL, 0, 0};
            level->boosts = {NULL, 0, 0};
            level->start_pos.pos.x = 0.f;
            level->start_pos.pos.y = GAME_HEIGHT * 0.5f;
            level->start_vel = glm::vec2(0.f);
            level->start_pos.angle = 0.f;
            level->goal_line.pos.x = GAME_WIDTH * 0.4f;
        } else {
            int loading_result =
                load_map_from_file(
                    mapfile_path,
                    &level->obstacles,
                    &level->boosts,
                    &level->portals,
                    &level->start_pos.pos.x, &level->start_pos.pos.y,
                    &level->start_vel.x, &level->start_vel.y,
                    &level->start_pos.angle,
                    &level->goal_line.pos.x);
            if (loading_result != 0) return loading_result;
        }

        p->body.pos = level->start_pos.pos;
        p->vel = level->start_vel;
        p->body.angle = level->start_pos.angle;
        cam->pos.x = p->body.pos.x;

    } else {
        return 1;
        /*
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
        */
    }

    PR::ParticleSystem *boost_ps = &level->particle_systems[0];
    boost_ps->particles_number = 200;
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
    boost_ps->time_between_particles = 0.01f;
    boost_ps->time_elapsed = 0.f;
    boost_ps->frozen = false;
    boost_ps->active = false;
    boost_ps->all_inactive = true;
    boost_ps->create_particle = create_particle_plane_boost;
    boost_ps->update_particle = update_particle_plane_boost;
    boost_ps->draw_particle = draw_particle_plane_boost;
    for(size_t particle_index = 0;
        particle_index < boost_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = boost_ps->particles + particle_index;
        (boost_ps->create_particle)(boost_ps, particle);
    }

    PR::ParticleSystem *plane_crash_ps = &level->particle_systems[1];
    plane_crash_ps->particles_number = 100;
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
    plane_crash_ps->time_between_particles = 0.02f;
    plane_crash_ps->time_elapsed = 0.f;
    plane_crash_ps->frozen = false;
    plane_crash_ps->active = false;
    plane_crash_ps->all_inactive = true;
    plane_crash_ps->create_particle = create_particle_plane_crash;
    plane_crash_ps->update_particle = update_particle_plane_crash;
    plane_crash_ps->draw_particle = draw_particle_plane_crash;
    for(size_t particle_index = 0;
        particle_index < plane_crash_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = plane_crash_ps->particles + particle_index;
        (plane_crash_ps->create_particle)(plane_crash_ps, particle);
    }

    PR::ParticleSystem *rider_crash_ps = &level->particle_systems[2];
    rider_crash_ps->particles_number = 100;
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
    rider_crash_ps->time_between_particles = 0.02f;
    rider_crash_ps->time_elapsed = 0.f;
    rider_crash_ps->frozen = false;
    rider_crash_ps->active = false;
    rider_crash_ps->all_inactive = true;
    rider_crash_ps->create_particle = create_particle_rider_crash;
    rider_crash_ps->update_particle = update_particle_rider_crash;
    rider_crash_ps->draw_particle = draw_particle_rider_crash;
    for(size_t particle_index = 0;
        particle_index < rider_crash_ps->particles_number;
        ++particle_index) {

        PR::Particle *particle = rider_crash_ps->particles +
                                 particle_index;
        (rider_crash_ps->create_particle)(rider_crash_ps, particle);
    }

    // Initializing the parallaxes
    parallax_init(&level->parallaxs[0], 0.9f,
                   texcoords_in_texture_space(
                       0, 275, 1920, 265,
                       glob->rend_res.global_sprite, false),
                  -((float)GAME_WIDTH), 0.f,
                  GAME_WIDTH, GAME_HEIGHT * 0.25f);
    parallax_init(&level->parallaxs[1], 0.85f,
                  texcoords_in_texture_space(
                      0, 815, 1920, 265,
                      glob->rend_res.global_sprite, false),
                  -((float)GAME_WIDTH), GAME_HEIGHT * 0.75f,
                  GAME_WIDTH, GAME_HEIGHT * 0.25f);
    parallax_init(&level->parallaxs[2], 0.75f,
                   texcoords_in_texture_space(
                       0, 545, 1920, 265,
                       glob->rend_res.global_sprite, false),
                  -((float)GAME_WIDTH), GAME_HEIGHT * 0.75f,
                  GAME_WIDTH, GAME_HEIGHT * 0.25f);

    // Initializing plane animation
    animation_init(&p->anim, glob->rend_res.global_sprite,
                   128, 64, 32, 16, 32, 0, 7, 0.06f, false);

    // NOTE: Hide the cursor only if it succeded in doing everything else
    glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    if (level->editing_available) {
        level_activate_edit_mode(level);
    }

    PR::Sound *sound = &glob->sound;
    ma_sound_stop(&sound->menu_music);
    return 0;
}
void level_update(void) {
    Rect full_screen = {
        .pos = glm::vec2(0.f, 0.f),
        .dim = glm::vec2(GAME_WIDTH, GAME_HEIGHT),
        .angle = 0.f,
        .triangle = false,
    };
    renderer_add_queue_uni(
            full_screen,
            glm::vec4(0.3f, 0.8f, 0.9f, 1.0f),
            false);
    renderer_draw_uni(glob->rend_res.shaders[0]);

    // Level stuff
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    PR::Obstacles *obstacles = &glob->current_level.obstacles;
    PR::BoostPads *boosts = &glob->current_level.boosts;
    PR::Portals *portals = &glob->current_level.portals;

    PR::Level *level = &glob->current_level;

    PR::ParticleSystem *boost_ps =
        &glob->current_level.particle_systems[0];
    PR::ParticleSystem *plane_crash_ps =
        &glob->current_level.particle_systems[1];
    PR::ParticleSystem *rider_crash_ps =
        &glob->current_level.particle_systems[2];

    // Global stuff
    InputController *input = &glob->input;
    float dt = glob->state.delta_time;

    level->old_selected = level->selected;
    if (level->editing_available &&
            ACTION_CLICKED(PR_EDIT_TOGGLE_MODE)) {
        if (level->editing_now) {
            level_deactivate_edit_mode(level);
        } else {
            level_activate_edit_mode(level);
            level_reset_colors(level);
        }
    }

    #if 0
    // Structure of the update loop of the level
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

    float rider_left_right =
        ACTION_VALUE(PR_PLAY_RIDER_RIGHT) - ACTION_VALUE(PR_PLAY_RIDER_LEFT);
    float plane_up_down =
        ACTION_VALUE(PR_PLAY_PLANE_DOWN) - ACTION_VALUE(PR_PLAY_PLANE_UP);

    if (!p->crashed && !level->pause_now && !level->game_over) {
        // #### START PLANE STUFF

        if (level->editing_now) { // EDITING
            float velx = 1000.f;
            float vely = 600.f;
            if (level->selected) {
                Rect *b = get_selected_body(level->selected,
                                            level->selected_type);
                if (b) {
                    if (level->selected_type != PR::GOAL_LINE_TYPE) {
                        if (ACTION_PRESSED(PR_EDIT_MOVE_UP))
                            b->pos.y -= vely * dt;
                        if (ACTION_PRESSED(PR_EDIT_MOVE_DOWN))
                            b->pos.y += vely * dt;
                    }
                    if (ACTION_PRESSED(PR_EDIT_MOVE_LEFT))
                        b->pos.x -= velx * dt;
                    if (ACTION_PRESSED(PR_EDIT_MOVE_RIGHT))
                        b->pos.x += velx * dt;
                } else {
                    std::cout << "Could not get object body of type: "
                              << level->selected_type
                              << std::endl;
                }
            } else {
                if (ACTION_PRESSED(PR_EDIT_MOVE_UP))
                    p->body.pos.y -= vely * dt;
                if (ACTION_PRESSED(PR_EDIT_MOVE_DOWN))
                    p->body.pos.y += vely * dt;
                if (ACTION_PRESSED(PR_EDIT_MOVE_LEFT))
                    p->body.pos.x -= velx * dt;
                if (ACTION_PRESSED(PR_EDIT_MOVE_RIGHT))
                    p->body.pos.x += velx * dt;
            }
        } else { // PLAYING
            update_plane_physics_n_boost_collisions(level);
            // Propulsion
            // TODO: Could this be a "powerup" or something?
            //if (glob->input.boost.pressed &&
            //    !rid->crashed && rid->attached) {
            //    float propulsion = 8.f;
            //    p->acc.x += propulsion * cos(glm::radians(p->body.angle));
            //    p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
            //    boost_ps->active = true;
            //}
        }

        // #### END PLANE STUFF
        if (!rid->crashed) {
            if (rid->attached) {
                move_rider_to_plane(rid, p);
                // NOTE: Changing plane angle based on input
                if (plane_up_down != 0.f) {
                    p->body.angle -= p->inverse ?
                                        -150.f * plane_up_down * dt :
                                        150.f * plane_up_down * dt;
                }
                // NOTE: Limiting the angle of the plane
                if (p->body.angle > 360.f) {
                    p->body.angle -= 360.f;
                }
                if (p->body.angle < -360) {
                    p->body.angle += 360.f;
                }
                rid->attach_time_elapsed += dt;
                // NOTE: Make the rider jump based on input
                if (!level->editing_now &&
                    ACTION_CLICKED(PR_PLAY_RIDER_JUMP) &&
                    rid->attach_time_elapsed > 0.5f) { // PLAYING
                    rider_jump_from_plane(rid, p);
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
                if (rider_left_right) {
                    rid->input_velocity +=
                        rid->inverse ?
                            -RIDER_INPUT_VELOCITY_ACCELERATION *
                                rider_left_right * dt :
                            RIDER_INPUT_VELOCITY_ACCELERATION *
                                rider_left_right * dt;
                } else {
                    rid->input_velocity +=
                        RIDER_INPUT_VELOCITY_ACCELERATION *
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
                // If the player moves in the opposite direction of the
                // base velocity, remove a net amount based on the intensity
                // of the movement
                if (glm::abs(rider_left_right) > 0 &&
                        glm::sign(rider_left_right) !=
                            glm::sign(rid->base_velocity)) {
                    rid->base_velocity -= glm::sign(rid->base_velocity) *
                                          glm::abs(rider_left_right) *
                                          3000 * dt;
                }

                rid->vel.x = rid->base_velocity + rid->input_velocity;
                rid->vel.y += rid->inverse ? -RIDER_GRAVITY * dt :
                                             RIDER_GRAVITY * dt;

                // NOTE: Rider double jump if available
                if(rid->second_jump && ACTION_CLICKED(PR_PLAY_RIDER_JUMP)) {
                    if ((!rid->inverse && rid->vel.y < 0) ||
                         (rid->inverse && rid->vel.y > 0)) {
                        rid->vel.y += rid->inverse ? RIDER_SECOND_JUMP :
                                                     -RIDER_SECOND_JUMP;
                    } else {
                        rid->vel.y = rid->inverse ? RIDER_SECOND_JUMP:
                                                    -RIDER_SECOND_JUMP;
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
                    rid->attach_time_elapsed = 0;
                }

                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        } else { // rid->crashed
            if (rid->attached) {
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached
                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        }
    } else if (!level->pause_now && !level->game_over) { // p->crashed

        if (!rid->crashed) {
            if (rid->attached) {
                move_rider_to_plane(rid, p);
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached

                // NOTE: Modify accelleration based on input
                if (rider_left_right) {
                    rid->input_velocity +=
                        rid->inverse ?
                            -RIDER_INPUT_VELOCITY_ACCELERATION *
                                rider_left_right * dt :
                            RIDER_INPUT_VELOCITY_ACCELERATION *
                                rider_left_right * dt;
                } else {
                    rid->input_velocity +=
                        RIDER_INPUT_VELOCITY_ACCELERATION *
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
                rid->vel.y += rid->inverse ? -RIDER_GRAVITY * dt :
                                             RIDER_GRAVITY * dt;

                // NOTE: Rider double jump if available
                if(rid->second_jump && ACTION_CLICKED(PR_PLAY_RIDER_JUMP)) {
                    if ((!rid->inverse && rid->vel.y < 0) ||
                         (rid->inverse && rid->vel.y > 0)) {
                        rid->vel.y += rid->inverse ? RIDER_SECOND_JUMP:
                                                     -RIDER_SECOND_JUMP;
                    } else {
                        rid->vel.y = rid->inverse ? RIDER_SECOND_JUMP:
                                                    -RIDER_SECOND_JUMP;
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
            if (rid->attached) {
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached
                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        }
    }

    if (p->crashed) plane_crash_ps->active = true;
    if (rid->crashed) rider_crash_ps->active = true;

    // After the crash animation, the plane falls on the floor
    if (p->crashed && p->anim.finished &&
            p->anim.current == p->anim.frame_number-1) {
        plane_crash_ps->active = false;
        p->body.angle = 0.f;
        p->render_zone.angle = 0.f;
        if (glm::abs(p->render_zone.pos.y +
                     p->render_zone.dim.y*0.75f - GAME_HEIGHT) < 0.03f &&
                glm::length(p->vel) < 0.03f) {
            p->render_zone.pos.y = (float)GAME_HEIGHT -
                                    p->render_zone.dim.y*0.75f;
            p->vel.y = 0.f;
        } else {
            p->vel.y += (p->inverse ? -GRAVITY : GRAVITY) * 1.5f * dt;
            if (glm::abs(p->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
                p->vel.y = glm::sign(p->vel.y) * RIDER_VELOCITY_Y_LIMIT;
            }
            p->render_zone.pos.y += p->vel.y * dt;
            if (p->render_zone.pos.y + p->render_zone.dim.y*0.75f >
                    GAME_HEIGHT) {
                p->render_zone.pos.y = (float)GAME_HEIGHT -
                                        p->render_zone.dim.y*0.75f;
                p->vel.y = -p->vel.y * 0.5f;
            }
        }
    }

    // NOTE: Checking collision with the goal line
    if (!rid->crashed && !level->editing_now && !level->game_over &&
        rect_are_colliding(rid->body, level->goal_line, NULL, NULL)) {
        if (level->editing_available) {
            level_activate_edit_mode(level);
        } else {
            level->game_over = true;
            level->gamemenu_selected = PR::BUTTON_RESTART;
            level->game_won = true;
            rid->attached = false;
            level->text_wave_time = 0.f;
            glfwSetInputMode(glob->window.glfw_win,
                             GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    for(size_t px_index = 0;
        px_index < ARRAY_LENGTH(level->parallaxs);
        ++px_index) {
        parallax_update_n_queue_render(&level->parallaxs[px_index], cam->pos.x);
    }
    renderer_draw_tex(glob->rend_res.shaders[1], 
                      &glob->rend_res.global_sprite);

    // NOTE: Update the `render_zone`s based on the `body`s
    // p->render_zone.pos.x = p->body.pos.x;
    // p->render_zone.pos.y = p->inverse ? p->body.pos.y+p->body.dim.y :
    //                                     p->body.pos.y;

    if (!p->crashed) {
        p->render_zone.pos = p->body.pos +
                               (p->body.dim - p->render_zone.dim) * 0.5f;

        p->render_zone.angle = p->body.angle;
        plane_update_animation(p);
    }

    if (!rid->crashed) {
        rid->render_zone.pos = rid->body.pos +
                               (rid->body.dim - rid->render_zone.dim) * 0.5f;
        rid->render_zone.angle = rid->body.angle;
    }


    if (!level->editing_available && !level->pause_now && !level->game_over) {
        level->finish_time += dt;
    }

    // NOTE: If you click the left mouse button when you have something
    //          selected and you are not clicking on an option
    //          button, the object will be deselected
    bool set_selected_to_null = false;
    if (input->mouse_left.clicked && level->selected) {
        set_selected_to_null = true;
    }

    if (level->editing_now) { // EDITING

        for(size_t portal_index = 0;
            portal_index < portals->count;
            ++portal_index) {

            PR::Portal *portal = &portals->items[portal_index];

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
             boost_index < boosts->count;
             ++boost_index) {

            PR::BoostPad *pad = &boosts->items[boost_index];

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

        for (size_t obstacle_index = 0;
             obstacle_index < obstacles->count;
             obstacle_index++) {

            PR::Obstacle *obs = &obstacles->items[obstacle_index];

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

        if (ACTION_CLICKED(PR_EDIT_OBJ_CREATE)) {
            level->adding_now = true;
            level->selected = NULL;
        }

        if (ACTION_CLICKED(PR_EDIT_OBJ_DUPLICATE) && level->selected) {
            switch(level->selected_type) {
                case PR::PORTAL_TYPE:
                {
                    PR::Portal *portal = (PR::Portal *) level->selected;
                    int index = portal - portals->items;
                    da_append(portals, portals->items[index], PR::Portal);
                    level->selected = (void *) &da_last(portals);
                    break;
                }
                case PR::BOOST_TYPE:
                {
                    PR::BoostPad *pad = (PR::BoostPad *) level->selected;
                    int index = pad - boosts->items;
                    da_append(boosts, boosts->items[index], PR::BoostPad);
                    level->selected = (void *) &da_last(boosts);
                    break;
                }
                case PR::OBSTACLE_TYPE:
                {
                    PR::Obstacle *obs = (PR::Obstacle *) level->selected;
                    int index = obs - obstacles->items;
                    da_append(obstacles, obstacles->items[index], PR::Obstacle);
                    level->selected = (void *) &da_last(obstacles);
                    break;
                }
                default: { break; }
            }
        }

        if (level->selected == NULL && ACTION_CLICKED(PR_EDIT_PLANE_RESET)) {
            p->body.pos = level->start_pos.pos;
            p->body.angle = level->start_pos.angle;
        }

        renderer_add_queue_text(GAME_WIDTH * 0.9f, GAME_HEIGHT * 0.03f,
                                "EDITING", glm::vec4(1.f),
                                &glob->rend_res.fonts[0], true);

        // NOTE: Loop over window edged pacman style,
        //       but only on the top and bottom
        if (p->body.pos.y + p->body.dim.y * 0.5f > GAME_HEIGHT) {
            p->body.pos.y -= GAME_HEIGHT;
        }
        if (p->body.pos.y + p->body.dim.y * 0.5f < 0) {
            p->body.pos.y += GAME_HEIGHT;
        }
        
    } else { // PLAYING
        // NOTE: The portal can be activated only by the rider.
        //       If the rider is attached, then, by extensions, also
        //          the plane will activate the portal.
        //       Even if the rider is not attached, the effect is also
        //          applied to the plane.
        for(size_t portal_index = 0;
            portal_index < portals->count;
            ++portal_index) {

            PR::Portal *portal = &portals->items[portal_index];

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
                        if (level->colors_shuffled != portal->enable_effect) {

                            level->colors_shuffled = portal->enable_effect;

                            if (portal->enable_effect) {
                                level_shuffle_colors(level);
                            } else {
                                level_reset_colors(level);
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
             boost_index < boosts->count;
             ++boost_index) {

            PR::BoostPad *pad = &boosts->items[boost_index];
            boostpad_render(pad);
        }

        // NOTE: Checking collision with obstacles
        for (size_t obstacle_index = 0;
             obstacle_index < obstacles->count;
             obstacle_index++) {

            PR::Obstacle *obs = &obstacles->items[obstacle_index];

            obstacle_render(obs);

            if (!p->crashed && obs->collide_plane &&
                rect_are_colliding(p->body, obs->body,
                                   &p->crash_position.x,
                                   &p->crash_position.y)) {
                // NOTE: Plane colliding with an obstacle

                if (rid->attached) {
                    rider_jump_from_plane(rid, p);
                }
                p->crashed = true;
                plane_activate_crash_animation(p);
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

                if (level->editing_available) {
                    level_activate_edit_mode(level);
                } else {
                    level->game_over = true;
                    level->gamemenu_selected = PR::BUTTON_RESTART;
                    glfwSetInputMode(glob->window.glfw_win,
                                     GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
        }

        Rect p_body_camera_space = rect_in_camera_space(p->body, cam);
        Rect rid_body_camera_space = rect_in_camera_space(rid->body, cam);
        // NOTE: Collide with the ceiling and the floor
        Rect plane_ceiling;
        plane_ceiling.pos.x = p_body_camera_space.pos.x +
                              p_body_camera_space.dim.x * 0.5f -
                              ((float) GAME_WIDTH);
        plane_ceiling.dim.x = GAME_WIDTH*2.f;
        plane_ceiling.pos.y = -((float) GAME_HEIGHT);
        plane_ceiling.dim.y = GAME_HEIGHT;
        plane_ceiling.angle = 0.f;
        plane_ceiling.triangle = false;
        Rect rider_ceiling = plane_ceiling;
        rider_ceiling.pos.x = rid_body_camera_space.pos.x +
                              rid_body_camera_space.dim.x * 0.5f -
                              ((float) GAME_WIDTH);

        Rect plane_floor;
        plane_floor.pos.x = plane_ceiling.pos.x;
        plane_floor.pos.y = (float) GAME_HEIGHT;
        plane_floor.dim.x = GAME_WIDTH*2.f;
        plane_floor.dim.y = GAME_HEIGHT;
        plane_floor.angle = 0.f;
        plane_floor.triangle = false;
        Rect rider_floor = plane_floor;
        rider_floor.pos.x = rider_ceiling.pos.x;

        // NOTE: Collisions with the ceiling
        if (!p->crashed &&
            rect_are_colliding(p_body_camera_space, plane_ceiling,
                               &p->crash_position.x,
                               &p->crash_position.y)) {

            p->crash_position +=
                cam->pos - glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);

            if (rid->attached) {
                rider_jump_from_plane(rid, p);
            }
            p->crashed = true;
            plane_activate_crash_animation(p);
            p->acc *= 0.f;
            p->vel *= 0.f;

            // TODO: Debug flag
            std::cout << "Plane collided with the ceiling" << std::endl;
        }
        if (!rid->crashed &&
            rect_are_colliding(rid_body_camera_space, rider_ceiling,
                               &rid->crash_position.x,
                               &rid->crash_position.y)) {

            if (level->editing_available) {
                level_activate_edit_mode(level);
            } else {
                rid->crash_position +=
                    cam->pos -
                    glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);

                rid->crashed = true;
                rid->attached = false;
                level->game_over = true;
                level->gamemenu_selected = PR::BUTTON_RESTART;
                glfwSetInputMode(glob->window.glfw_win,
                                 GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;
            }

            // TODO: Debug flag
            std::cout << "Rider collided with the ceiling" << std::endl;
        }

        // NOTE: Collisions with the floor
        if (!p->crashed &&
            rect_are_colliding(p_body_camera_space, plane_floor,
                               &p->crash_position.x,
                               &p->crash_position.y)) {

            p->crash_position +=
                cam->pos -
                glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);

            if (rid->attached) {
                rider_jump_from_plane(rid, p);
            }
            p->crashed = true;
            plane_activate_crash_animation(p);
            p->acc *= 0.f;
            p->vel *= 0.f;

            // TODO: Debug flag
            std::cout << "Plane collided with the floor" << std::endl;
        }
        if (!rid->crashed &&
            rect_are_colliding(rid_body_camera_space, rider_floor,
                               &rid->crash_position.x,
                               &rid->crash_position.y)) {

            if (level->editing_available) {
                level_activate_edit_mode(level);
            } else {
                rid->crash_position +=
                    cam->pos -
                    glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);

                level->game_over = true;
                level->gamemenu_selected = PR::BUTTON_RESTART;
                glfwSetInputMode(glob->window.glfw_win,
                                 GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                rid->crashed = true;
                rid->attached = false;
                rid->vel *= 0.f;
                rid->base_velocity = 0.f;
                rid->input_velocity = 0.f;
            }

            // TODO: Debug flag
            std::cout << "Rider collided with the floor" << std::endl;
        }
    }

    // NOTE: Rendering goal line
    renderer_add_queue_uni(rect_in_camera_space(level->goal_line, cam),
                           glm::vec4(1.0f), false);

    // Actually issuing the render calls
    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_tex(glob->rend_res.shaders[1],
                      &glob->rend_res.global_sprite);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

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

        if (ps->active) ps->all_inactive = false;

        if (!ps->active && !ps->all_inactive) {

            ps->all_inactive = true;
            for(size_t particle_index = 0;
                particle_index < ps->particles_number;
                ++particle_index) {

                PR::Particle *particle = ps->particles +
                                         particle_index;

                if (particle->active) {
                    ps->all_inactive = false;
                    break;
                }
            }
        }

        if (ps->all_inactive) {
            continue;
        }

        if (!ps->frozen) {
            ps->time_elapsed += dt;
            while(ps->time_elapsed > ps->time_between_particles) {
                ps->time_elapsed -= ps->time_between_particles;

                PR::Particle *particle = ps->particles +
                                         ps->current_particle;
                ps->current_particle = (ps->current_particle + 1) %
                                             ps->particles_number;

                (ps->create_particle)(ps, particle);
            }
        }
        for (size_t particle_index = 0;
             particle_index < ps->particles_number;
             ++particle_index) {

            PR::Particle *particle = ps->particles + particle_index;

            if (!ps->frozen) {
                (ps->update_particle)(ps, particle);
            }

            (ps->draw_particle)(ps, particle);
        }
    }
    // NOTE: Rendering the plane hitbox
    // renderer_add_queue_uni(rect_in_camera_space(p->body, cam),
    //                        glm::vec4(1.0f, 1.0f, 1.0f, 1.f),
    //                        false);

    // NOTE: Rendering plane texture
    if (!level->game_over && !level->pause_now) {
        animation_step(&p->anim);
    }
    animation_queue_render(rect_in_camera_space(p->render_zone, cam),
                           &p->anim, p->inverse);
    // renderer_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
    //                        texcoords_in_texture_space(
    //                             p->current_animation * 32, 0.f,
    //                             32, 8,
    //                             glob->rend_res.global_sprite, p->inverse),
    //                        false);

    // NOTE: Rendering the rider
    renderer_add_queue_uni(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);
    // NOTE: Issuing draw call for plane/rider and particles
    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_tex(glob->rend_res.shaders[1],
                      &glob->rend_res.global_sprite);

    if (level->adding_now) {
        PR::Button add_portal;
        add_portal.from_center = true;
        add_portal.body.pos.x = GAME_WIDTH * 0.25f;
        add_portal.body.pos.y = GAME_HEIGHT * 0.5f;
        add_portal.body.dim.x = GAME_WIDTH * 0.2f;
        add_portal.body.dim.y = GAME_HEIGHT * 0.3f;
        add_portal.body.triangle = false;
        add_portal.body.angle = 0.f;
        add_portal.col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        std::snprintf(add_portal.text,
                      std::strlen("ADD PORTAL")+1,
                      "ADD PORTAL");

        PR::Button add_boost;
        add_boost.from_center = true;
        add_boost.body.pos.x = GAME_WIDTH * 0.5f;
        add_boost.body.pos.y = GAME_HEIGHT * 0.5f;
        add_boost.body.dim.x = GAME_WIDTH * 0.2f;
        add_boost.body.dim.y = GAME_HEIGHT * 0.3f;
        add_boost.body.triangle = false;
        add_boost.body.angle = 0.f;
        add_boost.col = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        std::snprintf(add_boost.text,
                      std::strlen("ADD BOOST")+1,
                      "ADD BOOST");

        PR::Button add_obstacle;
        add_obstacle.from_center = true;
        add_obstacle.body.pos.x = GAME_WIDTH * 0.75f;
        add_obstacle.body.pos.y = GAME_HEIGHT * 0.5f;
        add_obstacle.body.dim.x = GAME_WIDTH * 0.2f;
        add_obstacle.body.dim.y = GAME_HEIGHT * 0.3f;
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

                PR::Portal portal;
                portal.body.pos = cam->pos;
                portal.body.dim.x = GAME_WIDTH * 0.2f;
                portal.body.dim.y = GAME_HEIGHT * 0.2f;
                portal.body.triangle = false;
                portal.body.angle = 0.f;
                portal.type = PR::SHUFFLE_COLORS;
                portal.enable_effect = true;

                da_append(portals, portal, PR::Portal);

                level->selected = (void *) &da_last(portals);
                level->selected_type = PR::PORTAL_TYPE;
                set_portal_option_buttons(level->selected_options_buttons);

            } else
            if (rect_contains_point(add_boost.body,
                                    input->mouseX, input->mouseY,
                                    add_boost.from_center)) {

                PR::BoostPad pad;
                pad.body.pos = cam->pos;
                pad.body.dim.x = GAME_WIDTH * 0.2f;
                pad.body.dim.y = GAME_HEIGHT * 0.2f;
                pad.body.triangle = false;
                pad.body.angle = 0.f;
                pad.boost_angle = 0.f;
                pad.boost_power = 0.f;

                da_append(boosts, pad, PR::BoostPad);

                level->selected = (void *) &da_last(boosts);
                level->selected_type = PR::BOOST_TYPE;
                set_boost_option_buttons(level->selected_options_buttons);

            } else
            if (rect_contains_point(add_obstacle.body,
                                    input->mouseX, input->mouseY,
                                    add_obstacle.from_center)) {

                PR::Obstacle obs;
                obs.body.pos = cam->pos;
                obs.body.dim.x = GAME_WIDTH * 0.2f;
                obs.body.dim.y = GAME_HEIGHT * 0.2f;
                obs.body.triangle = false;
                obs.body.angle = 0.f;
                obs.collide_plane = false;
                obs.collide_rider = false;

                da_append(obstacles, obs, PR::Obstacle);

                level->selected = (void *) &da_last(obstacles);
                level->selected_type = PR::OBSTACLE_TYPE;
                set_obstacle_option_buttons(level->selected_options_buttons);

            }
        }

        // TODO: Selected an appropriate font for this
        button_render(add_portal, glm::vec4(1.f),
                      &glob->rend_res.fonts[1]);
        button_render(add_boost, glm::vec4(1.f),
                      &glob->rend_res.fonts[1]);
        button_render(add_obstacle, glm::vec4(1.f),
                      &glob->rend_res.fonts[1]);

        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                           glob->rend_res.shaders[2]);
    }

    if (level->selected && ACTION_CLICKED(PR_EDIT_OBJ_DELETE)) {
        switch(level->selected_type) {
            case PR::PORTAL_TYPE:
            {
                PR::Portal *portal = (PR::Portal *) level->selected;

                int index = portal - portals->items;
                da_remove(portals, index);
                std::cout << "Removed portal n. " << index << std::endl;
                
                level->selected = NULL;

                break;
            }
            case PR::BOOST_TYPE:
            {
                PR::BoostPad *pad = (PR::BoostPad *) level->selected;

                int index = pad - boosts->items;
                da_remove(boosts, index);
                std::cout << "Removed boost n. " << index << std::endl;
                
                level->selected = NULL;

                break;
            }
            case PR::OBSTACLE_TYPE:
            {
                PR::Obstacle *obs = (PR::Obstacle *) level->selected;

                int index = obs - obstacles->items;
                da_remove(obstacles, index);
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
    if (level->selected != NULL && level->editing_now &&
        level->selected == level->old_selected &&
        !level->adding_now) {
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

                    PR::Button *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index <= 1) {
                        PR::Button minus1;
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
                        std::snprintf(minus1.text, strlen("-1")+1, "-1");

                        PR::Button minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;
                        std::snprintf(minus5.text, strlen("-5")+1, "-5");

                        PR::Button plus1;
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
                        std::snprintf(plus1.text, strlen("+1")+1, "+1");

                        PR::Button plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;
                        std::snprintf(plus5.text, strlen("+5")+1, "+5");


                        if (input->mouse_left.clicked ||
                            input->mouse_right.pressed) {

                            float one = input->mouse_right.pressed ?
                                        5.f * dt :
                                        1.f;
                            float five = input->mouse_right.pressed ?
                                        200.f * dt :
                                        5.f;

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
                                        portal->body.dim.x += one;
                                        break;
                                    case 1:
                                        portal->body.dim.y += one;
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
                                        portal->body.pos.x -= five * 0.5f;
                                        portal->body.dim.x += five;
                                        break;
                                    case 1:
                                        portal->body.pos.y -= five * 0.5f;
                                        portal->body.dim.y += five;
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
                                        portal->body.pos.x += one * 0.5f;
                                        portal->body.dim.x -= one;
                                        break;
                                    case 1:
                                        portal->body.pos.y += one * 0.5f;
                                        portal->body.dim.y -= one;
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
                                        portal->body.pos.x += five * 0.5f;
                                        portal->body.dim.x -= five;
                                        break;
                                    case 1:
                                        portal->body.pos.y += five * 0.5f;
                                        portal->body.dim.y -= five;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        // TODO: Selected an appropriate font for this
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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

                        // TODO: Selected an appropriate font for this
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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

                    PR::Button *button =
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
                        PR::Button minus1;
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
                        std::snprintf(minus1.text, strlen("-1")+1, "-1");

                        PR::Button minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;
                        std::snprintf(minus5.text, strlen("-5")+1, "-5");

                        PR::Button plus1;
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
                        std::snprintf(plus1.text, strlen("+1")+1, "+1");

                        PR::Button plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;
                        std::snprintf(plus5.text, strlen("+5")+1, "+5");

                        if (input->mouse_left.clicked ||
                            input->mouse_right.pressed) {

                            float one = input->mouse_right.pressed ?
                                        5.f * dt :
                                        1.f;
                            float five = input->mouse_right.pressed ?
                                        200.f * dt :
                                        5.f;

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
                                        pad->body.pos.x -= one * 0.5f;
                                        pad->body.dim.x += one;
                                        break;
                                    case 1:
                                        pad->body.pos.y -= one * 0.5f;
                                        pad->body.dim.y += one;
                                        break;
                                    case 2:
                                        pad->body.angle += one;
                                        break;
                                    case 4:
                                        pad->boost_angle += one;
                                        break;
                                    case 5:
                                        pad->boost_power += one;
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
                                        pad->body.pos.x -= five * 0.5f;
                                        pad->body.dim.x += five;
                                        break;
                                    case 1:
                                        pad->body.pos.y -= five * 0.5f;
                                        pad->body.dim.y += five;
                                        break;
                                    case 2:
                                        pad->body.angle += five;
                                        break;
                                    case 4:
                                        pad->boost_angle += five;
                                        break;
                                    case 5:
                                        pad->boost_power += five;
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
                                        pad->body.pos.x += one * 0.5f;
                                        pad->body.dim.x -= one;
                                        break;
                                    case 1:
                                        pad->body.pos.y += one * 0.5f;
                                        pad->body.dim.y -= one;
                                        break;
                                    case 2:
                                        pad->body.angle -= one;
                                        break;
                                    case 4:
                                        pad->boost_angle -= one;
                                        break;
                                    case 5:
                                        pad->boost_power -= one;
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
                                        pad->body.pos.x += five * 0.5f;
                                        pad->body.dim.x -= five;
                                        break;
                                    case 1:
                                        pad->body.pos.y += five * 0.5f;
                                        pad->body.dim.y -= five;
                                        break;
                                    case 2:
                                        pad->body.angle -= five;
                                        break;
                                    case 4:
                                        pad->boost_angle -= five;
                                        break;
                                    case 5:
                                        pad->boost_power -= five;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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

                    PR::Button *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index <= 2) {
                        PR::Button minus1;
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
                        std::snprintf(minus1.text, strlen("-1")+1, "-1");

                        PR::Button minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;
                        std::snprintf(minus5.text, strlen("-5")+1, "-5");

                        PR::Button plus1;
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
                        std::snprintf(plus1.text, strlen("+1")+1, "+1");

                        PR::Button plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;
                        std::snprintf(plus5.text, strlen("+5")+1, "+5");


                        if (input->mouse_left.clicked ||
                            input->mouse_right.pressed) {

                            float one = input->mouse_right.pressed ?
                                        5.f * dt :
                                        1.f;
                            float five = input->mouse_right.pressed ?
                                        200.f * dt :
                                        5.f;

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
                                        obs->body.pos.x -= one * 0.5f;
                                        obs->body.dim.x += one;
                                        break;
                                    case 1:
                                        obs->body.pos.y -= one * 0.5f;
                                        obs->body.dim.y += one;
                                        break;
                                    case 2:
                                        obs->body.angle += one;
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
                                        obs->body.pos.x -= five * 0.5f;
                                        obs->body.dim.x += five;
                                        break;
                                    case 1:
                                        obs->body.pos.y -= five * 0.5f;
                                        obs->body.dim.y += five;
                                        break;
                                    case 2:
                                        obs->body.angle += five;
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
                                        obs->body.pos.x += one * 0.5f;
                                        obs->body.dim.x -= one;
                                        break;
                                    case 1:
                                        obs->body.pos.y += one * 0.5f;
                                        obs->body.dim.y -= one;
                                        break;
                                    case 2:
                                        obs->body.angle -= one;
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
                                        obs->body.pos.x += five * 0.5f;
                                        obs->body.dim.x -= five;
                                        break;
                                    case 1:
                                        obs->body.pos.y += five * 0.5f;
                                        obs->body.dim.y -= five;
                                        break;
                                    case 2:
                                        obs->body.angle -= five;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        // TODO: Selected an appropriate font for this
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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
                        // TODO: Selected an appropriate font for this
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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
                start_pos_render_info(rect, level->start_vel, 5.f, 0.f);
                renderer_draw_text(&glob->rend_res.fonts[OBJECT_INFO_FONT],
                                   glob->rend_res.shaders[2]);

                for(size_t option_button_index = 0;
                    option_button_index < SELECTED_START_POS_OPTIONS;
                    ++option_button_index) {
                    assert((option_button_index <
                                ARRAY_LENGTH(level->selected_options_buttons))
                            && "Selected options out of bound for obstacles");

                    PR::Button *button =
                        &level->selected_options_buttons[option_button_index];

                    if (option_button_index <= 2) {
                        PR::Button minus1;
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
                        std::snprintf(minus1.text, strlen("-1")+1, "-1");

                        PR::Button minus5 = minus1;
                        minus5.body.pos.x += minus5.body.dim.x * 1.2f;
                        std::snprintf(minus5.text, strlen("-5")+1, "-5");

                        PR::Button plus1;
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
                        std::snprintf(plus1.text, strlen("+1")+1, "+1");

                        PR::Button plus5 = plus1;
                        plus5.body.pos.x -= plus5.body.dim.x * 1.2f;
                        std::snprintf(plus5.text, strlen("+5")+1, "+5");


                        if (input->mouse_left.clicked ||
                            input->mouse_right.pressed) {

                            float one = input->mouse_right.pressed ?
                                        5.f * dt :
                                        1.f;
                            float five = input->mouse_right.pressed ?
                                        200.f * dt :
                                        5.f;

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
                                        rect->angle += one;
                                        break;
                                    case 1:
                                        level->start_vel.x += one;
                                        break;
                                    case 2:
                                        level->start_vel.y += one;
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
                                        rect->angle += five;
                                        break;
                                    case 1:
                                        level->start_vel.x += five;
                                        break;
                                    case 2:
                                        level->start_vel.y += five;
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
                                        rect->angle -= one;
                                        break;
                                    case 1:
                                        level->start_vel.x -= one;
                                        break;
                                    case 2:
                                        level->start_vel.y -= one;
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
                                        rect->angle -= five;
                                        break;
                                    case 1:
                                        level->start_vel.x -= five;
                                        break;
                                    case 2:
                                        level->start_vel.y -= five;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        // TODO: Selected an appropriate font for this
                        button_render(*button, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(plus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus1, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
                        button_render(minus5, glm::vec4(1.f),
                                      &glob->rend_res.fonts[1]);
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


    if (level->game_over) {
        if (rid->attached) {
            // NOTE: Making the camera move to the plane
            lerp_camera_x_to_rect(cam, &p->body, true);
        } else { // !rid->attached
            // NOTE: Make the camera follow the rider
            lerp_camera_x_to_rect(cam, &rid->body, true);
        }
        
        // NOTE: If the rider crashes I still want to simulate
        //       the plane physics
        if (!p->crashed) {
            update_plane_physics_n_boost_collisions(level);
        }

        // NOTE: Game is over
        PR::Button b_restart = {
            .from_center = true,
            .body = {
                .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.4f),
                .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
                .angle = 0.f,
                .triangle = false,
            },
            .col = LEVEL_BUTTON_DEFAULT_COLOR,
            .text = "RESTART",
        };
        PR::Button b_quit = {
            .from_center = true,
            .body = {
                .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.7f),
                .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
                .angle = 0.f,
                .triangle = false,
            },
            .col = LEVEL_BUTTON_DEFAULT_COLOR,
            .text = "QUIT",
        };

        // ### Mouse change selections ###
        // ## RESTART
        if (input->was_mouse_moved &&
            rect_contains_point(b_restart.body,
                                input->mouseX, input->mouseY,
                                b_restart.from_center)) {
            level->gamemenu_selected = PR::BUTTON_RESTART;
        }
        // ## QUIT
        if (input->was_mouse_moved &&
            rect_contains_point(b_quit.body,
                                input->mouseX, input->mouseY,
                                b_quit.from_center)) {
            level->gamemenu_selected = PR::BUTTON_QUIT;
        }
        // ### Keybinding change selection ###
        if (ACTION_CLICKED(PR_MENU_UP)) {
            level->gamemenu_selected = PR::BUTTON_RESTART;
        }
        if (ACTION_CLICKED(PR_MENU_DOWN)) {
            level->gamemenu_selected = PR::BUTTON_QUIT;
        }

        // ### All ways to click ###
        // ## RESTART
        if (ACTION_CLICKED(PR_PLAY_RESTART) ||
            (ACTION_CLICKED(PR_MENU_CLICK) &&
                level->gamemenu_selected == PR::BUTTON_RESTART) ||
            (input->mouse_left.clicked &&
                rect_contains_point(b_restart.body,
                                    input->mouseX, input->mouseY,
                                    b_restart.from_center))) {
            CHANGE_CASE_TO_LEVEL(
                    level->file_path, level->name,
                    level->editing_available, level->is_new, void());
        }
        // ## QUIT
        if (ACTION_CLICKED(PR_PLAY_QUIT) ||
            (ACTION_CLICKED(PR_MENU_CLICK) &&
                level->gamemenu_selected == PR::BUTTON_QUIT) ||
            (input->mouse_left.clicked &&
                rect_contains_point(b_quit.body,
                                    input->mouseX, input->mouseY,
                                    b_quit.from_center))) {
            CHANGE_CASE_TO_PLAY_MENU(void());
        }

        // ### Highlight selection ###
        if (level->gamemenu_selected == PR::BUTTON_RESTART) {
            b_restart.col = LEVEL_BUTTON_SELECTED_COLOR;
            b_quit.col = LEVEL_BUTTON_DEFAULT_COLOR;
        }
        if (level->gamemenu_selected == PR::BUTTON_QUIT) {
            b_quit.col = LEVEL_BUTTON_SELECTED_COLOR;
            b_restart.col = LEVEL_BUTTON_DEFAULT_COLOR;
        }

        // ### Display winner text ###
        if (level->game_won) {
            level->text_wave_time += dt;
            shaderer_set_float(glob->rend_res.shaders[3], "time",
                               level->text_wave_time);
            char congratulations[] = "CONGRATULATIONS!";
            renderer_add_queue_text(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.1f,
                                    congratulations,
                                    glm::vec4(0.f, 0.f, 0.f, 1.f),
                                    &glob->rend_res.fonts[0], true);
            renderer_draw_text(&glob->rend_res.fonts[0],
                               glob->rend_res.shaders[3]);
            char time_recap[99];
            int size = std::snprintf(nullptr, 0,
                                     "You finished the level in %.3f seconds!",
                                     level->finish_time);
            std::snprintf(time_recap, size+1,
                          "You finished the level in %.3f seconds!",
                          level->finish_time);
            renderer_add_queue_text(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.2f,
                                    time_recap,
                                    glm::vec4(0.f, 0.f, 0.f, 1.f),
                                    &glob->rend_res.fonts[1], true);
            renderer_draw_text(&glob->rend_res.fonts[1],
                               glob->rend_res.shaders[2]);
        }
        renderer_add_queue_uni(b_restart.body,
                               b_restart.col,
                               b_restart.from_center);
        renderer_add_queue_text(b_restart.body.pos.x,
                                b_restart.body.pos.y,
                                b_restart.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);
        renderer_add_queue_uni(b_quit.body,
                               b_quit.col,
                               b_quit.from_center);
        renderer_add_queue_text(b_quit.body.pos.x,
                                b_quit.body.pos.y,
                                b_quit.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);

        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[DEFAULT_FONT],
                           glob->rend_res.shaders[2]);

    } else if (level->pause_now) {
        // NOTE: Game in pause mode
        PR::Button b_resume = {
            .from_center = true,
            .body = {
                .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.2f),
                .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
                .angle = 0.f,
                .triangle = false,
            },
            .col = LEVEL_BUTTON_DEFAULT_COLOR,
            .text = "RESUME",
        };
        PR::Button b_restart = {
            .from_center = true,
            .body = {
                .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f),
                .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
                .angle = 0.f,
                .triangle = false,
            },
            .col = LEVEL_BUTTON_DEFAULT_COLOR,
            .text = "RESTART",
        };
        PR::Button b_quit = {
            .from_center = true,
            .body = {
                .pos = glm::vec2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.8f),
                .dim = glm::vec2(GAME_WIDTH * 0.4f, GAME_HEIGHT * 0.2f),
                .angle = 0.f,
                .triangle = false,
            },
            .col = LEVEL_BUTTON_DEFAULT_COLOR,
            .text = "QUIT",
        };

        // ### Mouse change selections ###
        // ## RESUME
        if (input->was_mouse_moved &&
            rect_contains_point(b_resume.body,
                                input->mouseX, input->mouseY,
                                b_resume.from_center)) {
            level->gamemenu_selected = PR::BUTTON_RESUME;
        }
        // ## RESTART
        if (input->was_mouse_moved &&
            rect_contains_point(b_restart.body,
                                input->mouseX, input->mouseY,
                                b_restart.from_center)) {
            level->gamemenu_selected = PR::BUTTON_RESTART;
        }
        // ## QUIT
        if (input->was_mouse_moved &&
            rect_contains_point(b_quit.body,
                                input->mouseX, input->mouseY,
                                b_quit.from_center)) {
            level->gamemenu_selected = PR::BUTTON_QUIT;
        }

        // ### Keybinding change selection ###
        if (ACTION_CLICKED(PR_MENU_UP)) {
            if (level->gamemenu_selected == PR::BUTTON_RESTART) {
                level->gamemenu_selected = PR::BUTTON_RESUME;
            } else if (level->gamemenu_selected == PR::BUTTON_QUIT) {
                level->gamemenu_selected = PR::BUTTON_RESTART;
            }
        }
        if (ACTION_CLICKED(PR_MENU_DOWN)) {
            if (level->gamemenu_selected == PR::BUTTON_RESUME) {
                level->gamemenu_selected = PR::BUTTON_RESTART;
            } else if (level->gamemenu_selected == PR::BUTTON_RESTART) {
                level->gamemenu_selected = PR::BUTTON_QUIT;
            }
        }

        // ### All ways to click ###
        // ## RESUME
        if (ACTION_CLICKED(PR_PLAY_RESUME) ||
            (ACTION_CLICKED(PR_MENU_CLICK) &&
                level->gamemenu_selected == PR::BUTTON_RESUME) ||
            (input->mouse_left.clicked &&
                rect_contains_point(b_resume.body,
                                    input->mouseX, input->mouseY,
                                    b_resume.from_center))) {
            level->pause_now = false;
            for(size_t ps_index = 0;
                ps_index < ARRAY_LENGTH(level->particle_systems);
                ++ps_index) {
                level->particle_systems[ps_index].frozen = false;
            }
            if (!level->editing_now) {
                glfwSetInputMode(glob->window.glfw_win,
                                 GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            }
        }
        // ## RESTART
        if (ACTION_CLICKED(PR_PLAY_RESTART) ||
            (ACTION_CLICKED(PR_MENU_CLICK) &&
                level->gamemenu_selected == PR::BUTTON_RESTART) ||
            (input->mouse_left.clicked &&
                rect_contains_point(b_restart.body,
                                    input->mouseX, input->mouseY,
                                    b_restart.from_center))) {
            CHANGE_CASE_TO_LEVEL(
                    level->file_path, level->name,
                    level->editing_available, level->is_new, void());
        }
        // ## QUIT
        if (ACTION_CLICKED(PR_PLAY_QUIT) ||
            (ACTION_CLICKED(PR_MENU_CLICK) &&
                level->gamemenu_selected == PR::BUTTON_QUIT) ||
            (input->mouse_left.clicked &&
                rect_contains_point(b_quit.body,
                                    input->mouseX, input->mouseY,
                                    b_quit.from_center))) {
            CHANGE_CASE_TO_PLAY_MENU(void());
        }

        // ### Highlight selection ###
        if (level->gamemenu_selected == PR::BUTTON_RESUME) {
            b_resume.col = LEVEL_BUTTON_SELECTED_COLOR;
            b_restart.col = LEVEL_BUTTON_DEFAULT_COLOR;
            b_quit.col = LEVEL_BUTTON_DEFAULT_COLOR;
        }
        if (level->gamemenu_selected == PR::BUTTON_RESTART) {
            b_restart.col = LEVEL_BUTTON_SELECTED_COLOR;
            b_resume.col = LEVEL_BUTTON_DEFAULT_COLOR;
            b_quit.col = LEVEL_BUTTON_DEFAULT_COLOR;
        }
        if (level->gamemenu_selected == PR::BUTTON_QUIT) {
            b_quit.col = LEVEL_BUTTON_SELECTED_COLOR;
            b_resume.col = LEVEL_BUTTON_DEFAULT_COLOR;
            b_restart.col = LEVEL_BUTTON_DEFAULT_COLOR;
        }

        renderer_add_queue_uni(b_resume.body,
                               b_resume.col,
                               b_resume.from_center);
        renderer_add_queue_text(b_resume.body.pos.x,
                                b_resume.body.pos.y,
                                b_resume.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);
        renderer_add_queue_uni(b_restart.body,
                               b_restart.col,
                               b_restart.from_center);
        renderer_add_queue_text(b_restart.body.pos.x,
                                b_restart.body.pos.y,
                                b_restart.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);
        renderer_add_queue_uni(b_quit.body,
                               b_quit.col,
                               b_quit.from_center);
        renderer_add_queue_text(b_quit.body.pos.x,
                                b_quit.body.pos.y,
                                b_quit.text, glm::vec4(1.0f),
                                &glob->rend_res.fonts[0], true);

        renderer_draw_uni(glob->rend_res.shaders[0]);
        renderer_draw_text(&glob->rend_res.fonts[DEFAULT_FONT],
                           glob->rend_res.shaders[2]);
    } else {
        if (ACTION_CLICKED(PR_PLAY_PAUSE) &&
            !level->pause_now &&
            !level->game_over) {

            std::cout << "Pausing" << std::endl;
            level->pause_now = true;
            for(size_t ps_index = 0;
                ps_index < ARRAY_LENGTH(level->particle_systems);
                ++ps_index) {
                level->particle_systems[ps_index].frozen = true;
            }
            level->gamemenu_selected = PR::BUTTON_RESUME;
            glfwSetInputMode(glob->window.glfw_win,
                             GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (ACTION_CLICKED(PR_EDIT_SAVE_MAP)) {
        if (save_map_to_file(level->file_path, level)) {
            std::cout << "[ERROR] Could not save the map in the file: "
                      << level->file_path << std::endl;
        } else {
            std::cout << "Saved current level to file: "
                      << level->file_path << std::endl;
            level->is_new = false;
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

inline void plane_activate_crash_animation(PR::Plane *p) {
    p->anim.active = true;
    p->anim.current = 1;
    float speed = glm::length(p->vel);
    if (speed == 0) {
        p->anim.frame_stop = 0;
    } else
    if (0 < speed && speed <= PLANE_VELOCITY_LIMIT * 0.15f) {
        p->anim.frame_stop = 1;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.15f < speed &&
            speed <= PLANE_VELOCITY_LIMIT * 0.3f) {
        p->anim.frame_stop = 2;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.3f < speed &&
            speed <= PLANE_VELOCITY_LIMIT * 0.45f) {
        p->anim.frame_stop = 3;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.45f < speed &&
            speed <= PLANE_VELOCITY_LIMIT * 0.6f) {
        p->anim.frame_stop = 4;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.6f < speed &&
            speed <= PLANE_VELOCITY_LIMIT * 0.75f) {
        p->anim.frame_stop = 5;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.75f < speed &&
            speed <= PLANE_VELOCITY_LIMIT * 0.9f) {
        p->anim.frame_stop = 6;
    } else
    if (PLANE_VELOCITY_LIMIT * 0.9f < speed) {
        p->anim.frame_stop = 7;
    }

    if (p->anim.frame_stop) {
        p->anim.frame_duration = 0.4f / (float)p->anim.frame_stop;
    }
}

inline void portal_render(PR::Portal *portal) {
    PR::Camera *cam = &glob->current_level.camera;
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

    Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

    if (glm::abs(pad_in_cam_pos.pos.x) >
            glm::abs(pad_in_cam_pos.dim.x) + glm::abs(pad_in_cam_pos.dim.y) +
            glm::abs(GAME_WIDTH) + glm::abs(GAME_HEIGHT)
     || glm::abs(pad_in_cam_pos.pos.y) >
            glm::abs(pad_in_cam_pos.dim.x) + glm::abs(pad_in_cam_pos.dim.y) +
            glm::abs(GAME_WIDTH) + glm::abs(GAME_HEIGHT)
    ) return;

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
                               0, 8, 96, 32,
                               glob->rend_res.global_sprite, false),
                           true);
}

inline void obstacle_render(PR::Obstacle *obs) {
    PR::Camera *cam = &glob->current_level.camera;
    Rect obs_in_cam_pos = rect_in_camera_space(obs->body, cam);

    if (glm::abs(obs_in_cam_pos.pos.x) >
            glm::abs(obs_in_cam_pos.dim.x) + glm::abs(obs_in_cam_pos.dim.y) +
            glm::abs(GAME_WIDTH) + glm::abs(GAME_HEIGHT)
     || glm::abs(obs_in_cam_pos.pos.y) >
            glm::abs(obs_in_cam_pos.dim.x) + glm::abs(obs_in_cam_pos.dim.y) +
            glm::abs(GAME_WIDTH) + glm::abs(GAME_HEIGHT)
    ) return;

    renderer_add_queue_uni(obs_in_cam_pos,
                          get_obstacle_color(obs),
                          false);
}

inline void button_render(PR::Button but, glm::vec4 col, Font *font) {
    renderer_add_queue_uni(but.body, but.col, but.from_center);
    renderer_add_queue_text(but.body.pos.x, but.body.pos.y,
                            but.text, col, font, true);
}

inline void button_render_in_menu_camera(PR::Button but, glm::vec4 col, Font *font, PR::MenuCamera *cam) {

    Rect in_cam_rect = rect_in_menu_camera_space(but.body, cam);
    renderer_add_queue_uni(in_cam_rect, but.col, but.from_center);
    renderer_add_queue_text(in_cam_rect.pos.x, in_cam_rect.pos.y,
                            but.text, col, font, true);
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

inline void start_pos_render_info(Rect *rect, glm::vec2 vel,
                                  float tx, float ty) {
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
    // std::sprintf(buffer, "dim: (%f, %f)",
    //              rect->dim.x, rect->dim.y);
    // renderer_add_queue_text(tx, ty+(spacing*index++), buffer, glm::vec4(1.f),
    //                         &glob->rend_res.fonts[OBJECT_INFO_FONT], false);
    std::sprintf(buffer, "vel: (%f, %f)",
                 vel.x, vel.y);
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

inline void rider_jump_from_plane(PR::Rider *rid, PR::Plane *p) {
    rid->attached = false;
    rid->second_jump = true;
    rid->base_velocity = p->vel.x;
    rid->vel.y = rid->inverse ? p->vel.y*0.5f + RIDER_FIRST_JUMP :
                                p->vel.y*0.5f - RIDER_FIRST_JUMP;
    rid->body.angle = 0.f;
    rid->jump_time_elapsed = 0.f;
}

inline void level_reset_colors(PR::Level *level) {
    level->current_red = PR::RED;
    level->current_blue = PR::BLUE;
    level->current_gray = PR::GRAY;
    level->current_white = PR::WHITE;
}

inline void level_shuffle_colors(PR::Level *level) {
    int8_t shuffled_colors[] = {-1, -1, -1, -1};
    for(size_t i = 0;
        i < ARRAY_LENGTH(shuffled_colors);
        ++i) {
        int tmp_r;
        bool present;
        do {
            tmp_r = (int) (((float) rand() / (float)RAND_MAX) *
                    ARRAY_LENGTH(shuffled_colors));
            present = false;
            for(size_t j = 0; j < i; ++j) {
                if ((shuffled_colors[j] == tmp_r) ||
                    (i < ARRAY_LENGTH(shuffled_colors)-1 &&
                        (size_t)tmp_r == i)) {
                    present = true;
                    break;
                }
            }
        } while (present);
        shuffled_colors[i] = tmp_r;
    }
    level->current_red = (PR::ObstacleColorIndex)
        shuffled_colors[0];
    level->current_white = (PR::ObstacleColorIndex)
        shuffled_colors[1];
    level->current_blue = (PR::ObstacleColorIndex) 
        shuffled_colors[2];
    level->current_gray = (PR::ObstacleColorIndex)
        shuffled_colors[3];
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

void button_set_position(PR::Button *button, size_t index) {
    size_t row = index / 3;
    size_t col = index % 3;

    button->body.pos.x = (col * 2.f + 1.f) / 6.f * GAME_WIDTH;
    button->body.pos.y = (row * 2.f + 3.f) / 10.f * GAME_HEIGHT;
    button->body.dim.x = GAME_WIDTH / 4.f;
    button->body.dim.y = GAME_HEIGHT / 8.f;
    button->body.angle = 0.f;
    button->body.triangle = false;
    button->col = LEVEL_BUTTON_DEFAULT_COLOR;
    button->from_center = true;
}

void button_edit_del_to_lb(PR::Button *reference,
                           PR::Button *edit, PR::Button *del) {

    edit->body.pos.x = reference->body.pos.x +
                       reference->body.dim.x * 0.05f;
    edit->body.pos.y = reference->body.pos.y +
                       reference->body.dim.y * 0.25f;
    edit->body.dim.x = reference->body.dim.x * 0.4f;
    edit->body.dim.y = reference->body.dim.y * 0.5f;
    edit->body.angle = 0.f;
    edit->body.triangle = false;
    edit->col = EDIT_BUTTON_DEFAULT_COLOR;
    edit->from_center = false;
    std::snprintf(edit->text, std::strlen("EDIT")+1, "EDIT");

    del->body.pos.x = reference->body.pos.x +
                        reference->body.dim.x * 0.5f;
    del->body.pos.y = reference->body.pos.y -
                        reference->body.dim.y * 0.5f;
    del->body.dim.x = reference->body.dim.y * 0.3f;
    del->body.dim.y = reference->body.dim.y * 0.3f;
    del->body.angle = 0.f;
    del->body.triangle = false;
    del->col = DEL_BUTTON_DEFAULT_COLOR;
    del->from_center = true;
    std::snprintf(del->text, std::strlen("-")+1, "-");
}

void set_portal_option_buttons(PR::Button *buttons) {
    // NOTE: Set up options buttons for the selected portal
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_PORTAL_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for portals");

        PR::Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_PORTAL_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH /
                             (SELECTED_PORTAL_OPTIONS+2);
        button->body.dim.y = GAME_HEIGHT / 10;

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

void set_boost_option_buttons(PR::Button *buttons) {
    // NOTE: Set up options buttons for the selected boost
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_BOOST_OPTIONS;
        ++option_button_index) {

        assert((option_button_index <
                    SELECTED_MAX_OPTIONS)
                && "Selected options out of bounds for boosts");

        PR::Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_BOOST_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH / (SELECTED_BOOST_OPTIONS+2);
        button->body.dim.y = GAME_HEIGHT / 10;

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

void set_obstacle_option_buttons(PR::Button *buttons) {
    // NOTE: Set up options buttons for the selected obstacle
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_OBSTACLE_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for obstacle");

        PR::Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.triangle = false;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_OBSTACLE_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH / (SELECTED_OBSTACLE_OPTIONS+2);
        button->body.dim.y = GAME_HEIGHT / 10;

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

void set_start_pos_option_buttons(PR::Button *buttons) {
    for(size_t option_button_index = 0;
        option_button_index < SELECTED_START_POS_OPTIONS;
        ++option_button_index) {
        assert((option_button_index <
                 SELECTED_MAX_OPTIONS)
                && "Selected options out of bound for start_position");

        PR::Button *button = buttons + option_button_index;

        button->from_center = true;
        button->body.pos.x = GAME_WIDTH * (option_button_index+1) /
                             (SELECTED_START_POS_OPTIONS+1);
        button->body.pos.y = GAME_HEIGHT * 9 / 10;
        button->body.dim.x = GAME_WIDTH / (SELECTED_START_POS_OPTIONS+2);
        button->body.dim.y = GAME_HEIGHT / 10;

        switch(option_button_index) {
            case 0:
                std::snprintf(button->text,
                              std::strlen("ANGLE")+1,
                              "ANGLE");
                break;
            case 1:
                std::snprintf(button->text,
                              std::strlen("VEL X")+1,
                              "VEL X");
                break;
            case 2:
                std::snprintf(button->text,
                              std::strlen("VEL Y")+1,
                              "VEL Y");
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
        glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);
    res.dim = r.dim;
    res.angle = r.angle;
    res.triangle = r.triangle;

    return res;
}

inline Rect rect_in_menu_camera_space(Rect r, PR::MenuCamera *cam) {
    Rect res;

    res.pos = r.pos - cam->pos +
        glm::vec2(GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f);
    res.dim = r.dim;
    res.angle = r.angle;
    res.triangle = r.triangle;

    return res;
}

void level_activate_edit_mode(PR::Level *level) {
    std::cout << "Activating edit mode!" << std::endl;
    level->editing_now = true;
    level->game_over = false;
    if (level->plane.inverse) {
        level->plane.body.pos.y += level->plane.body.dim.y;
        level->plane.body.dim.y = -level->plane.body.dim.y;
    }
    level->plane.inverse = false;
    level->plane.crashed = false;
    animation_reset(&level->plane.anim);
    level->plane.anim.frame_stop = level->plane.anim.frame_number - 1;
    level->plane.vel = glm::vec2(0.f);
    if (level->rider.inverse) {
        level->rider.body.dim.y = -level->rider.body.dim.y;
    }
    level->rider.inverse = false;
    level->rider.crashed = false;
    level->rider.attached = true;
    level->rider.vel = glm::vec2(0.f);
    level->colors_shuffled = false;
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void level_deactivate_edit_mode(PR::Level *level) {
    std::cout << "Deactivating edit mode!" << std::endl;
    level->selected = NULL;
    level->editing_now = false;
    // animation_reset(&level->plane.anim);
    // level->plane.anim.frame_stop = level->plane.anim.frame_number - 1;
    level->camera.pos.x = level->plane.body.pos.x;
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void update_plane_physics_n_boost_collisions(PR::Level *level) {
    PR::ParticleSystem *boost_ps =
        &glob->current_level.particle_systems[0];

    PR::Plane *p = &level->plane;
    PR::Rider *rid = &level->rider;
    float dt = glob->state.delta_time;

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;
    apply_air_resistances(p);
    // NOTE: Checking collision with boost_pads
    for (size_t boost_index = 0;
         boost_index < level->boosts.count;
         ++boost_index) {

        PR::BoostPad pad = level->boosts.items[boost_index];

        if (rect_are_colliding(p->body, pad.body, NULL, NULL)) {

            boost_ps->active = true;
            
            p->acc.x += pad.boost_power *
                        cos(glm::radians(pad.boost_angle));
            p->acc.y += pad.boost_power *
                        -sin(glm::radians(pad.boost_angle));

        }
    }
    // Propulsion
    // TODO: Could this be a "powerup" or something?
    //if (glob->input.boost.pressed &&
    //    !rid->crashed && rid->attached) {
    //    float propulsion = 8.f;
    //    p->acc.x += propulsion * cos(glm::radians(p->body.angle));
    //    p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
    //    boost_ps->active = true;
    //}
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

// Parallax system
void parallax_init(PR::Parallax *px, float fc, TexCoords tc,
                   float p_start_x, float p_start_y,
                   float p_w, float p_h) {

    px->tex_coords = tc;
    px->follow_coeff = fc;
    // NOTE: The reference is the middle of the middle piece at the start
    px->reference_point = p_start_x + p_w * 1.5f;

    for(size_t piece_index = 0;
        piece_index < ARRAY_LENGTH(px->pieces);
        ++piece_index) {

        PR::ParallaxPiece *piece = px->pieces + piece_index;

        *piece = {
            .base_pos_x = p_start_x + (p_w * piece_index),
            .body = {
                .pos = glm::vec2(p_start_x + (p_w * piece_index), p_start_y),
                .dim = glm::vec2(p_w, p_h),
                .angle = 0.f,
                .triangle = false,
            }
        };
    }
}

void parallax_update_n_queue_render(PR::Parallax *px, float current_x) {
    for(size_t piece_index = 0;
        piece_index < ARRAY_LENGTH(px->pieces);
        ++piece_index) {

        PR::ParallaxPiece *piece = px->pieces + piece_index;
        piece->body.pos.x = piece->base_pos_x +
                            (px->reference_point - current_x) *
                            (1.f - px->follow_coeff);

        // NOTE: px->reference_point - piece->body.dim.x * 1.5f
        //          would be the starting pos.x of the first piece.
        //       px->reference_point - piece->body.dim.x * 1.5f
        //          means half the width to the left of the first piece
        //       In that case we change that piece so that piece
        //          becomes the last one
        if (piece->body.pos.x <
                px->reference_point - piece->body.dim.x * 2.f) {
            piece->base_pos_x += piece->body.dim.x * 3.f;
        }
        if (piece->body.pos.x + piece->body.dim.x >
                px->reference_point + piece->body.dim.x * 2.f) {
            piece->base_pos_x -= piece->body.dim.x * 3.f;
        }
        renderer_add_queue_tex(piece->body, px->tex_coords, false);
    }
}

// Animation system
void animation_init(PR::Animation *a, Texture tex,
                    size_t start_x, size_t start_y,
                    size_t dim_x, size_t dim_y,
                    size_t step_x, size_t step_y,
                    size_t frame_number, float frame_duration, bool loop) {
    a->loop = loop;
    a->frame_number = frame_number;
    a->frame_stop = frame_number - 1;
    a->frame_duration = frame_duration;

    if (a->tc) std::free(a->tc);
    a->tc = (TexCoords *) std::malloc(sizeof(TexCoords) * frame_number);

    for(size_t i = 0; i < frame_number; ++i) {
        a->tc[i] = texcoords_in_texture_space(start_x + (step_x * i),
                                              start_y + (step_y * i),
                                              dim_x, dim_y,
                                              tex, false);
    }

    a->current = 0;
    a->active = false;
    a->finished = false;
    a->frame_elapsed = 0.f;
}
void animation_step(PR::Animation *a) {
    float dt = glob->state.delta_time;

    if (!a->active) return;

    a->frame_elapsed += dt;
    if (a->frame_elapsed > a->frame_duration) {
        a->frame_elapsed -= a->frame_duration;
        if (a->loop) {
            a->current = (a->current + 1) % a->frame_number;
        } else {
            if (a->current < a->frame_number-1 &&
                 a->current < a->frame_stop) {
                a->current++;
            } else {
                a->finished = true;
                a->active = false;
            }
        }
    }
}
void animation_queue_render(Rect b, PR::Animation *a, bool inverse) {
    TexCoords tc = a->tc[a->current];
    if (inverse) {
        tc.ty += tc.th;
        tc.th = -tc.th;
    }
    renderer_add_queue_tex(b, tc, false);
}
void animation_reset(PR::Animation *a) {
    a->current = 0;
    a->active = false;
    a->finished = false;
    a->frame_elapsed = 0.f;
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
        particle->active = true;
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
        particle->active = false;
    }
}
void update_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->vel *= (1.f - dt); 
    particle->color.a -= particle->color.a * dt * 3.0f;
    particle->body.pos += particle->vel * dt;
}
void draw_particle_plane_boost(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_uni(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                            particle->color, true);
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
        particle->active = true;
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
        particle->active = false;
    }
}
void update_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos += particle->vel * dt;
    particle->body.angle -=
        glm::sign(particle->vel.x) *
        lerp(0.f, 720.f, glm::abs(particle->vel.x)/150.f) * dt;
}
void draw_particle_plane_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_tex(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                           texcoords_in_texture_space(
                                730, 315, 90, 80,
                                glob->rend_res.global_sprite, false),
                           false);
    // renderer_add_queue_uni(rect_in_camera_space(particle->body,
    //                                             &glob->current_level.camera),
    //                         particle->color, true);
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
        particle->active = true;
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
        particle->active = false;
    }
}
void update_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos += particle->vel * dt;
    particle->body.angle -=
        glm::sign(particle->vel.x) *
        lerp(0.f, 720.f, glm::abs(particle->vel.x)/150.f) * dt;
}
void draw_particle_rider_crash(PR::ParticleSystem *ps,
                                 PR::Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_uni(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                            particle->color, true);
}

// UI OptionSlider functions
void option_slider_init_selection(PR::OptionSlider *slider, float pad) {
    if (pad < 0) {
        pad = 0.2f;
    }
    float pad_dim = slider->background.dim.y * pad;
    slider->selection.pos.x = slider->background.pos.x -
                                slider->background.dim.x * 0.5f +
                                pad_dim;
    slider->selection.pos.y = slider->background.pos.y -
                                slider->background.dim.y * 0.5f +
                                pad_dim;
    slider->selection.dim.y = slider->background.dim.y - pad_dim * 2.f;
    slider->selection.angle = 0.f;
    slider->selection.triangle = false;
    slider->selection.dim.x =
        (slider->background.dim.x - pad_dim * 2.f) * slider->value;

    snprintf(slider->value_text, ARRAY_LENGTH(slider->value_text),
             "%.2f", slider->value);
}
void option_slider_update_value(PR::OptionSlider *slider, float value) {
    if (value < 0.01f) value = 0.f;
    else if (value > 0.99f) value = 1.f;
    float pad_dim = glm::abs(slider->selection.pos.x -
                                slider->background.pos.x +
                                slider->background.dim.x * 0.5f);
    slider->value = value;
    slider->selection.dim.x =
        (slider->background.dim.x - pad_dim * 2.f) * value;
    snprintf(slider->value_text, ARRAY_LENGTH(slider->value_text),
             "%.2f", slider->value);
}
void option_slider_handle_mouse(PR::OptionSlider *slider,
                                float mouseX, float mouseY, Key mouse_button) {
    // Update mouse_hooked
    if (mouse_button.clicked &&
        !slider->mouse_hooked &&
        rect_contains_point(slider->background, mouseX, mouseY, true)) {
        slider->mouse_hooked = true;
    }
    if (!mouse_button.pressed) {
        slider->mouse_hooked = false;
    }
    // If still hooked: update. Otherwise skip
    if (slider->mouse_hooked) {
        float pad_dim = glm::abs(slider->selection.pos.x -
                                    slider->background.pos.x +
                                    slider->background.dim.x * 0.5f);
        float new_value =
            (float)(mouseX - slider->selection.pos.x) /
                (float)(slider->background.dim.x - pad_dim * 2.f);
        option_slider_update_value(slider, new_value);
    }
}
void option_slider_render(PR::OptionSlider *slider, glm::vec4 color) {
    renderer_add_queue_uni(slider->background,
                           glm::vec4(0.f, 0.f, 0.f, 1.f), true);
    renderer_add_queue_uni(slider->selection,
                           color, false);
    renderer_add_queue_text(
            slider->background.pos.x -
                slider->background.dim.x * 0.5f -
                70.f,
            slider->background.pos.y,
            slider->value_text,
            color,
            &glob->rend_res.fonts[0],
            true);
}

void options_menu_selection_handle_mouse(PR::OptionsMenu *opt,
                                         float mouseX, float mouseY,
                                         bool showing_general_pane) {

    if (mouseX < 0 || mouseX > GAME_WIDTH) return;

    if (showing_general_pane) {
        if (opt->master_volume.mouse_hooked ||
            opt->sfx_volume.mouse_hooked ||
            opt->music_volume.mouse_hooked) return;

        float position = (mouseY - GAME_HEIGHT * 0.2f) / (GAME_HEIGHT * 0.8f);

        if (position < 0) {
            opt->current_selection = PR::OPTION_NONE;
        } else if (position <= 0.2f) {
            opt->current_selection = PR::OPTION_MASTER_VOLUME;
        } else if (position <= 0.4f) {
            opt->current_selection = PR::OPTION_SFX_VOLUME;
        } else if (position <= 0.6f) {
            opt->current_selection = PR::OPTION_MUSIC_VOLUME;
        } else if (position <= 0.8f) {
            opt->current_selection = PR::OPTION_DISPLAY_MODE;
        } else {
            if (glob->window.display_mode == PR::WINDOWED) {
                opt->current_selection = PR::OPTION_RESOLUTION;
            }
        }
    } else {
        // TODO: Keybindings pane
    }
}

void options_menu_update_bindings(PR::OptionsMenu *opt, InputAction *actions) {
    for(size_t bind_index = 0;
        bind_index < ARRAY_LENGTH(opt->change_kb_binds1);
        ++bind_index) {

        InputAction *action = &actions[bind_index];

        PR::Button *kb1 = &opt->change_kb_binds1[bind_index];
        PR::Button *kb2 = &opt->change_kb_binds2[bind_index];
        PR::Button *gp1 = &opt->change_gp_binds1[bind_index];
        PR::Button *gp2 = &opt->change_gp_binds2[bind_index];

        const char *kb1_name = get_key_name(action->kb_binds[0].bind_index);
        std::strncpy(kb1->text,
                     (kb1_name != NULL) ? kb1_name : "...",
                     ARRAY_LENGTH(kb1->text)-1);

        const char *kb2_name = get_key_name(action->kb_binds[1].bind_index);
        std::strncpy(kb2->text,
                     (kb2_name != NULL) ? kb2_name : "...",
                     ARRAY_LENGTH(kb2->text)-1);

        const char *gp1_name =
            get_gamepad_button_name(action->gp_binds[0].bind_index,
                                    action->gp_binds[0].type);
        std::strncpy(gp1->text,
                     (gp1_name != NULL) ? gp1_name : "...",
                     ARRAY_LENGTH(gp1->text)-1);

        const char *gp2_name =
            get_gamepad_button_name(action->gp_binds[1].bind_index,
                                    action->gp_binds[1].type);
        std::strncpy(gp2->text,
                     (gp2_name != NULL) ? gp2_name : "...",
                     ARRAY_LENGTH(gp2->text)-1);
    }
}

void display_mode_update(PR::WinInfo *win, PR::DisplayMode dm) {
    std::cout << "New display mode: " << dm << std::endl;

    GLFWmonitor* main_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(main_monitor);

    if (dm == PR::FULLSCREEN) {
        glfwSetWindowMonitor(win->glfw_win, main_monitor, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR::BORDERLESS) {
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, false);
        glfwSetWindowMonitor(win->glfw_win, NULL, 0, 0,
                             mode->width, mode->height,
                             GLFW_DONT_CARE);
    } else if (dm == PR::WINDOWED) {
        int windowed_w = window_resolution_width(win->windowed_resolution);
        int windowed_h = window_resolution_height(win->windowed_resolution);
        glfwSetWindowAttrib(win->glfw_win, GLFW_DECORATED, true);
        glfwSetWindowMonitor(win->glfw_win, NULL,
                             (mode->width - windowed_w) * 0.5f,
                             (mode->height - windowed_h) * 0.5f,
                             windowed_w, windowed_h,
                             GLFW_DONT_CARE);
    } else {
        std::cout << "[ERROR] Unknown window mode: " << dm << std::endl;
    }
}

void window_resolution_set(PR::WinInfo *win, PR::WindowResolution res) {
    if (win->display_mode != PR::WINDOWED || res == PR::R_NONE) return;

    win->windowed_resolution = res;

    glfwSetWindowSize(win->glfw_win, window_resolution_width(res),
                      window_resolution_height(res));
}
PR::WindowResolution window_resolution_prev(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return PR::R1280X960;
        case PR::R1280X960: return PR::R1200x900;
        case PR::R1200x900: return PR::R960x720;
        case PR::R960x720: return PR::R800x600;
        case PR::R800x600: return PR::R640x480;
        case PR::R640x480: return PR::R400x300;
        case PR::R400x300: return PR::R320x240;
        case PR::R320x240: return PR::R320x240;
        default:
            std::cout << "Unknown window resolution: " << res << std::endl;
            return PR::R_NONE;
    }
}
PR::WindowResolution window_resolution_next(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return PR::R1440x1080;
        case PR::R1280X960: return PR::R1440x1080;
        case PR::R1200x900: return PR::R1280X960;
        case PR::R960x720: return PR::R1200x900;
        case PR::R800x600: return PR::R960x720;
        case PR::R640x480: return PR::R800x600;
        case PR::R400x300: return PR::R640x480;
        case PR::R320x240: return PR::R400x300;
        default:
            std::cout << "Unknown window resolution: " << res << std::endl;
            return PR::R_NONE;
    }
}
const char *window_resolution_to_str(PR::WindowResolution res) {
    switch(res) {
        case PR::R1440x1080: return "1440x1080";
        case PR::R1280X960: return "1280x960";
        case PR::R1200x900: return "1200x900";
        case PR::R960x720: return "960x720";
        case PR::R800x600: return "800x600";
        case PR::R640x480: return "640x480";
        case PR::R400x300: return "400x300";
        case PR::R320x240: return "320x240";
        default: return "UKNOWN";
    }
}
PR::WindowResolution window_resolution_from_dim(int width, int height) {
    if (width == 1440 && height == 1080) {
        return PR::R1440x1080;
    } else if (width == 1280 && height == 960) {
        return PR::R1280X960;
    } else if (width == 1200 && height == 900) {
        return PR::R1200x900;
    } else if (width == 960 && height == 720) {
        return PR::R960x720;
    } else if (width == 800 && height == 600) {
        return PR::R800x600;
    } else if (width == 640 && height == 480) {
        return PR::R640x480;
    } else if (width == 400 && height == 300) {
        return PR::R400x300;
    } else if (width == 320 && height == 240) {
        return PR::R320x240;
    } else {
        return PR::R_NONE;
    }
}
int window_resolution_width(PR::WindowResolution res) {
    return (int)(res / 3);
}
int window_resolution_height(PR::WindowResolution res) {
    return (int)(res / 4);
}

