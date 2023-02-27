#include <cassert>
#include <cstring>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <cerrno>

#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_renderer.h"
#include "pp_rect.h"

// TODO:
//  - Find textures
//  - UI
//  - Make changing angle more or less difficult
//  - Make the plane change angle alone when the rider jumps off (maybe)

// TODO:
//  - Maybe make the boost change the plane angle

#define GRAVITY (9.81f * 50.f)

#define PLANE_VELOCITY_LIMIT (1000.f)
#define RIDER_VELOCITY_Y_LIMIT (600.f)
#define RIDER_INPUT_VELOCITY_LIMIT (550.f)

#define CHANGE_CASE_TO(new_case, prepare_func)  do {\
        PR::Level temp_level = glob->current_level;\
        int preparation_result = (prepare_func)(&temp_level);\
        if (preparation_result == 0) {\
            free_level(&glob->current_level);\
            glob->current_level = temp_level;\
            glob->state.current_case = new_case;\
            return;\
        } else {\
            std::cout << "[ERROR] Could not prepare case: "\
                      << get_case_name(new_case)\
                      << std::endl;\
        }\
    } while(0)

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

void free_level(PR::Level *level) {
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

#define return_defer(ret) do { result = ret; goto defer; } while(0)
int load_map_from_file(const char *file_path,
                       PR::Obstacle **obstacles,
                       size_t *number_of_obstacles,
                       PR::BoostPad **boosts,
                       size_t *number_of_boosts,
                       float width, float height) {

    // NOTE: The screen resolution
    width = width / SCREEN_WIDTH_PROPORTION;
    height = height / SCREEN_HEIGHT_PROPORTION;
    
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
            obs->body.pos.x = x * width;
            obs->body.pos.y = y * height;
            obs->body.dim.x = w * width;
            obs->body.dim.y = h * height;
            obs->body.angle = r;
            obs->body.triangle = triangle;

            obs->collide_plane = collide_plane;
            obs->collide_rider = collide_rider;

            std::cout << "x: " << x * width
                      << " y: " << y * height
                      << " w: " << w * width
                      << " h: " << h * height
                      << " r: " << r
                      << std::endl;

        }

        // NOTE: Loading the boosts from memory
        std::fscanf(map_file, " %zu", number_of_boosts);
        if (std::ferror(map_file)) return_defer(1);

        std::cout << "[LOADING] " << *number_of_boosts
                  << " boost pads from file " << file_path
                  << std::endl;

        *boosts = (PR::BoostPad *) malloc(sizeof(PR::BoostPad) *
                                   *number_of_boosts);

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
            pad->body.pos.x = x * width;
            pad->body.pos.y = y * height;
            pad->body.dim.x = w * width;
            pad->body.dim.y = h * height;
            pad->body.angle = r;
            pad->body.triangle = triangle;
            pad->boost_angle = ba;
            pad->boost_power = bp;

            std::cout << "x: " << x * width
                      << " y: " << y * height
                      << " w: " << w * width
                      << " h: " << h * height
                      << " r: " << r
                      << " ba: " << ba
                      << " bp: " << bp
                      << std::endl;


        }

    }

    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

glm::vec4 get_obstacle_color(PR::Obstacle *obs) {
    glm::vec4 col;

    if (obs->collide_rider && obs->collide_plane) {
        col.r = 0.8f;
        col.g = 0.3f;
        col.b = 0.3f;
        col.a = 1.0f;
    } else
    if (obs->collide_rider) {
        col.r = 0.8f;
        col.g = 0.8f;
        col.b = 0.8f;
        col.a = 1.0f;
    } else
    if (obs->collide_plane) {
        col.r = 0.3f;
        col.g = 0.3f;
        col.b = 0.8f;
        col.a = 1.0f;
    } else {
        col.r = 0.4f;
        col.g = 0.4f;
        col.b = 0.4f;
        col.a = 1.0f;
    }

    return col;
}

struct Button {
    Rect body;
    glm::vec4 col;
    bool from_center;
    const char *text;
};

Button button_lvl1;
Button button_lvl2;

int menu_prepare(PR::Level *level) {
    PR::WinInfo *win = &glob->window;

    // NOTE: Buttons
    button_lvl1.body.pos.x = win->w * 0.5f - 200.f;
    button_lvl1.body.pos.y = win->h * 0.5f;
    button_lvl1.body.dim.x = 300.f;
    button_lvl1.body.dim.y = 100.f;
    button_lvl1.col.r = 0.8f;
    button_lvl1.col.g = 0.2f;
    button_lvl1.col.b = 0.5f;
    button_lvl1.col.a = 1.0f;
    button_lvl1.from_center = true;
    button_lvl1.text = "LEVEL 1";

    button_lvl2.body.pos.x = win->w * 0.5f + 200.f;
    button_lvl2.body.pos.y = win->h * 0.5f;
    button_lvl2.body.dim.x = 300.f;
    button_lvl2.body.dim.y = 100.f;
    button_lvl2.col.r = 0.2f;
    button_lvl2.col.g = 0.5f;
    button_lvl2.col.b = 0.8f;
    button_lvl2.col.a = 1.0f;
    button_lvl2.from_center = true;
    button_lvl2.text = "LEVEL 2";

    level->obstacles = NULL;
    level->boosts = NULL;
    for(size_t ps_index = 0;
        ps_index < ARRAY_LENGTH(level->particle_systems);
        ++ps_index) {
        level->particle_systems[ps_index].particles = NULL;
    }

    // NOTE: Make the cursor show
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    return 0;
}

void menu_update() {
    InputController *input = &glob->input;
    if (input->level1.clicked) {
        CHANGE_CASE_TO(PR::LEVEL1, level1_prepare);
    } else
    if (input->level2.clicked) {
        CHANGE_CASE_TO(PR::LEVEL2, level2_prepare);
    }

    if (rect_contains_point(&button_lvl1.body,
                            input->mouseX, input->mouseY,
                            button_lvl1.from_center)) {
        button_lvl1.col.r = 0.6f;
        button_lvl1.col.g = 0.0f;
        button_lvl1.col.b = 0.3f;
        button_lvl1.col.a = 1.0f;
        if (input->mouse_left.clicked) {
            CHANGE_CASE_TO(PR::LEVEL1, level1_prepare);
        }
    } else {
        button_lvl1.col.r = 0.8f;
        button_lvl1.col.g = 0.2f;
        button_lvl1.col.b = 0.5f;
        button_lvl1.col.a = 1.0f;
    }

    if (rect_contains_point(&button_lvl2.body,
                            input->mouseX, input->mouseY,
                            button_lvl2.from_center)) {
        button_lvl2.col.r = 0.0f;
        button_lvl2.col.g = 0.3f;
        button_lvl2.col.b = 0.6f;
        button_lvl2.col.a = 1.0f;
        if (input->mouse_left.clicked) {
            CHANGE_CASE_TO(PR::LEVEL2, level2_prepare);
        }
    } else {
        button_lvl2.col.r = 0.2f;
        button_lvl2.col.g = 0.5f;
        button_lvl2.col.b = 0.8f;
        button_lvl2.col.a = 1.0f;
    }


    return; 
}

void menu_draw(void) {
    if (glob->state.current_case != PR::MENU) return;
    renderer_add_queue_uni(button_lvl1.body,
                          button_lvl1.col,
                          button_lvl1.from_center);
    renderer_add_queue_uni(button_lvl2.body,
                          button_lvl2.col,
                          button_lvl2.from_center);

    renderer_add_queue_text(button_lvl1.body.pos.x, button_lvl1.body.pos.y,
                            button_lvl1.text, glm::vec4(1.0f),
                            &glob->rend_res.fonts[0], true);
    renderer_add_queue_text(button_lvl2.body.pos.x, button_lvl2.body.pos.y,
                            button_lvl2.text, glm::vec4(1.0f),
                            &glob->rend_res.fonts[0], true);

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_text(&glob->rend_res.fonts[0], glob->rend_res.shaders[2]);

    return;
}

enum PortalType {
    INVERSE,
    SHUFFLE_COLORS,
};
struct Portal {
    Rect body;
    PortalType type;
    bool enable_effect;
};

Portal inverse_portal;
Portal reverse_portal;

int level1_prepare(PR::Level *level) {
    PR::WinInfo *win = &glob->window;

    glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    PR::Plane *p = &level->plane;
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

    inverse_portal.body.pos.x = 400.f;
    inverse_portal.body.pos.y = 300.f;
    inverse_portal.body.dim.x = 70.f;
    inverse_portal.body.dim.y = 400.f;
    inverse_portal.body.angle = 0.f;
    inverse_portal.body.triangle = false;
    inverse_portal.type = INVERSE;
    inverse_portal.enable_effect = true;

    reverse_portal.body.pos.x = 4300.f;
    reverse_portal.body.pos.y = 300.f;
    reverse_portal.body.dim.x = 70.f;
    reverse_portal.body.dim.y = 400.f;
    reverse_portal.body.angle = 0.f;
    reverse_portal.body.triangle = false;
    inverse_portal.type = INVERSE;
    reverse_portal.enable_effect = false;

    PR::Rider *rid = &level->rider;
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

    PR::Camera *cam = &level->camera;
    cam->pos.x = p->body.pos.x;
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 3.f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

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
    return 0;
}

void level1_update() {
    // Level stuff
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

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
        CHANGE_CASE_TO(PR::MENU, menu_prepare);
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

        // DEBUG
        // float vel = 100.f;
        // if (input->up) p->body.pos.y -= vel * dt;
        // if (input->down) p->body.pos.y += vel * dt;
        // if (input->left) p->body.pos.x -= vel * dt;
        // if (input->right) p->body.pos.x += vel * dt;
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
                if (input->jump.clicked) {
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

    // NOTE: The portal can be activated only by the rider.
    //       If the rider is attached, then, by extensions, also
    //          the plane will activate the portal.
    //       Even if the rider is not attached, the effect is also
    //          applied to the plane.
    if ((!rid->crashed &&
            p->inverse != inverse_portal.enable_effect &&
            rid->inverse != inverse_portal.enable_effect) &&
        (rect_are_colliding(&rid->body, &inverse_portal.body, NULL, NULL) ||
            (rid->attached && rect_are_colliding(&p->body,
                                                 &inverse_portal.body,
                                                 NULL, NULL)))) {
        if (!p->crashed) {
            p->inverse = inverse_portal.enable_effect;
            p->body.pos.y += p->body.dim.y;
            p->body.dim.y = -p->body.dim.y;
        }
        rid->inverse = inverse_portal.enable_effect;
        rid->body.dim.y = -rid->body.dim.y;
    }
    if ((!rid->crashed &&
            p->inverse != reverse_portal.enable_effect &&
            rid->inverse != reverse_portal.enable_effect) &&
        (rect_are_colliding(&rid->body, &reverse_portal.body, NULL, NULL) ||
            (rid->attached && rect_are_colliding(&p->body,
                                                 &reverse_portal.body,
                                                 NULL, NULL)))) {
        if (!p->crashed) {
            p->inverse = reverse_portal.enable_effect;
            p->body.pos.y += p->body.dim.y;
            p->body.dim.y = -p->body.dim.y;
        }
        rid->inverse = reverse_portal.enable_effect;
        rid->body.dim.y = -rid->body.dim.y;
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
            p->crash_position = p->body.pos;
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
            rid->crash_position = rid->body.pos;
            rid->vel *= 0.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;

            // TODO: Debug flag
            std::cout << "Rider collided with " << obstacle_index << std::endl;
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

void level1_draw() {
    if (glob->state.current_case != PR::LEVEL1) return;

    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    float dt = glob->state.delta_time;

    // TESTING: Rendering the portal
    renderer_add_queue_uni(rect_in_camera_space(inverse_portal.body, cam),
                           glm::vec4(0.f, 0.f, 0.f, 1.f),
                           false);
    renderer_add_queue_uni(rect_in_camera_space(reverse_portal.body, cam),
                           glm::vec4(0.f, 0.f, 0.f, 1.f),
                           false);

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

    // High wind zone
    /* renderer_add_queue_uni(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false); */

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
                                    &glob->rend_res.global_sprite, p->inverse));

    renderer_draw_uni(glob->rend_res.shaders[0]);
    renderer_draw_tex(glob->rend_res.shaders[1],
                      &glob->rend_res.global_sprite);
}

int level2_prepare(PR::Level *level) {
    PR::WinInfo *win = &glob->window;

    int loading_result = load_map_from_file("level2.prmap",
                                            &level->obstacles,
                                            &level->obstacles_number,
                                            &level->boosts,
                                            &level->boosts_number,
                                            win->w, win->h);
    if (loading_result != 0) return loading_result;


    glfwSetInputMode(win->glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    PR::Plane *p = &level->plane;

    p->crashed = false;
    p->crash_position.x = 0.f;
    p->crash_position.y = 0.f;
    p->body.pos.x = 100.0f;
    p->body.pos.y = 0.0f;
    p->body.dim.y = 20.f;
    p->body.dim.x = p->body.dim.y * 3.f;
    p->body.angle = 0.f;
    p->body.triangle = false;
    p->render_zone.dim.y = 30.f;
    p->render_zone.dim.x = p->render_zone.dim.y * 3.f;
    p->render_zone.pos = p->body.pos + (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;
    p->vel.x = 0.f;
    p->vel.y = 0.f;
    p->acc.x = 0.f;
    p->acc.y = 0.f;
    p->mass = 0.005f; // kg
    // TODO: The alar surface should be somewhat proportional
    //       to the dimension of the actual rectangle
    p->alar_surface = 0.15f; // m squared
    p->current_animation = PR::Plane::IDLE_ACC;
    p->animation_countdown = 0.f;

    PR::Rider *rid = &level->rider;
    rid->body.dim.x = 30.f;
    rid->body.dim.y = 50.f;
    rid->body.angle = p->render_zone.angle;
    rid->body.triangle = false;
    rid->body.pos.x =
        p->render_zone.pos.x +
        (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
        (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
            sin(glm::radians(rid->body.angle)) -
        (p->render_zone.dim.x*0.2f) *
            cos(glm::radians(rid->body.angle));
    rid->body.pos.y =
        p->render_zone.pos.y +
        (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
        (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
            cos(glm::radians(rid->body.angle)) +
        (p->render_zone.dim.x*0.2f) *
            sin(glm::radians(rid->body.angle));
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

    PR::Camera *cam = &level->camera;
    cam->pos.x = p->body.pos.x;
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 3.f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

    std::cout << "level 2 prepared" << std::endl;

    return 0;
}

void level2_update() {
    // Level stuff
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    InputController *input = &glob->input;
    float dt = glob->state.delta_time;

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu.clicked) {
        CHANGE_CASE_TO(PR::MENU, menu_prepare);
    }

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;

    apply_air_resistances(p);

    // NOTE: Checking collision with boost_pads
    for (size_t boost_index = 0;
         boost_index < boosts_number;
         ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        if (rect_are_colliding(&p->body, &pad->body, NULL, NULL)) {

            p->acc.x += pad->boost_power * cos(glm::radians(pad->boost_angle));
            p->acc.y += pad->boost_power * -sin(glm::radians(pad->boost_angle));

            // std::cout << "BOOOST!!! against " << boost_index << std::endl;
        }
    }

    // Propulsion
    // TODO: Could this be a "powerup" or something?
    if (input->boost.pressed) {
        float propulsion = 8.f;
        p->acc.x += propulsion * cos(glm::radians(p->body.angle));
        p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
        /* std::cout << "PROP.x: " << propulsion * cos(glm::radians(p->body.angle)) */
        /*             << ",  PROP.y: " << propulsion * -sin(glm::radians(p->body.angle)) << "\n"; */
    }

    // NOTE: The mass is greater if the rider is attached
    if (rid->attached) p->acc *= 1.f/(p->mass + rid->mass);
    else p->acc *= 1.f/p->mass;

    // NOTE: Gravity is already an accelleration so it doesn't need to be divided
    p->acc.y += GRAVITY;

    // NOTE: Motion of the plane
    p->vel += p->acc * dt;
    p->body.pos += p->vel * dt + p->acc * POW2(dt) * 0.5f;

    if (rid->attached) {

        cam->pos.x = lerp(cam->pos.x, p->render_zone.pos.x+p->render_zone.dim.x*0.5f, dt * cam->speed_multiplier);

        // NOTE: Changing plane angle based on the input
        if (input->left_right) {
            p->body.angle += 150.f * -input->left_right * dt;
        }

        rid->body.angle = p->render_zone.angle;
        rid->body.pos.x =
            p->render_zone.pos.x +
            (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
            (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                sin(glm::radians(rid->body.angle)) -
            (p->render_zone.dim.x*0.2f) *
                cos(glm::radians(rid->body.angle));
        rid->body.pos.y =
            p->render_zone.pos.y +
            (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
            (p->render_zone.dim.y + rid->body.dim.y)*0.5f *
                cos(glm::radians(rid->body.angle)) +
            (p->render_zone.dim.x*0.2f) *
                sin(glm::radians(rid->body.angle));


        // NOTE: If the rider is attached, make it jump
        if (input->jump.clicked) {
            rid->attached = false;
            rid->base_velocity = p->vel.x;
            rid->vel.y = p->vel.y - 400.f;
            rid->body.angle = 0.f;
            rid->jump_time_elapsed = 0.f;
        }
    } else {
        cam->pos.x = lerp(cam->pos.x,
                          rid->render_zone.pos.x+rid->render_zone.dim.x*0.5f,
                          dt * cam->speed_multiplier);

        if (input->left_right) {
            rid->input_velocity += rid->input_max_accelleration *
                                   input->left_right * dt;
        } else {
            rid->input_velocity += rid->input_max_accelleration *
                                   -glm::sign(rid->input_velocity) * dt;
        }
        // NOTE: Base speed is the speed of the plane at the moment of the jump,
        //          which decreases slowly but constantly overtime
        //       Input speed is the speed that results from the player input.
        //
        //       This way, by touching nothing you get to keep the plane velocity,
        //       but you are still able to move left and right in a satisfying way
        if (glm::abs(rid->input_velocity) > RIDER_INPUT_VELOCITY_LIMIT) {
            rid->input_velocity =
                glm::sign(rid->input_velocity) * RIDER_INPUT_VELOCITY_LIMIT;
        }
        rid->base_velocity -= glm::sign(rid->base_velocity) *
                              rid->air_friction_acc * dt;

        rid->vel.x = rid->base_velocity + rid->input_velocity;
        rid->vel.y += GRAVITY * 1.5f * dt;

        if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
            rid->vel.y = glm::sign(rid->vel.y) *
                         RIDER_VELOCITY_Y_LIMIT ;
        }

        rid->body.pos += rid->vel * dt;

        rid->jump_time_elapsed += dt;

        // NOTE: If rider not attached, check if recollided
        if (rect_are_colliding(&p->body, &rid->body, NULL, NULL) &&
            rid->jump_time_elapsed > 0.5f) {
            rid->attached = true;
            p->vel += (rid->vel - p->vel) * 0.5f;
            rid->vel *= 0.f;
        }

    }

    // NOTE: Limiting the angle
    if (p->body.angle > 360.f) {
        p->body.angle -= 360.f;
    }
    if (p->body.angle < -360) {
        p->body.angle += 360.f;
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

            CHANGE_CASE_TO(PR::MENU, menu_prepare);

            // TODO: Debug flag
            std::cout << "Plane collided with " << obstacle_index << std::endl;
        }
        if (!rid->crashed && obs->collide_rider &&
            rect_are_colliding(&rid->body, &obs->body,
                               &rid->crash_position.x,
                               &rid->crash_position.y)) {
            // NOTE: Rider colliding with an obstacle

            CHANGE_CASE_TO(PR::MENU, menu_prepare);

            // TODO: Debug flag
            std::cout << "Rider collided with " << obstacle_index << std::endl;
        }
    }

    // NOTE: Limit velocities
    if (glm::length(p->vel) > PLANE_VELOCITY_LIMIT) {
        p->vel *= PLANE_VELOCITY_LIMIT / glm::length(p->vel);
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
    p->render_zone.pos = p->body.pos +
                         (p->body.dim - p->render_zone.dim) * 0.5f;
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

void level2_draw() {
    if (glob->state.current_case != PR::LEVEL2) return;

    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    PR::Obstacle *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    // float dt = glob->state.delta_time;

    // High wind zone
    /* renderer_add_queue_uni(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false); */

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
    }

    // NOTE: Rendering the obstacles
    for(size_t obstacle_index = 0;
        obstacle_index < obstacles_number;
        obstacle_index++) {

        PR::Obstacle *obs = obstacles + obstacle_index;

        Rect obs_in_cam_pos = rect_in_camera_space(obs->body, cam);

        if (obs_in_cam_pos.pos.x < -((float)win->w) ||
            obs_in_cam_pos.pos.x > win->w * 2.f) continue;

        renderer_add_queue_uni(obs_in_cam_pos,
                              get_obstacle_color(obs),
                              false);

    }

    // NOTE: Rendering the plane
    /*renderer_add_queue_uni(rect_in_camera_space(p->body, cam),
                          glm::vec4(1.0f, 1.0f, 1.0f, 1.f),
                          false);*/

    // TODO: Implement the boost as an actual thing
    glm::vec2 p_cam_pos = rect_in_camera_space(p->render_zone, cam).pos;
    if (glob->input.boost.pressed) {
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
                              false,
                              true);
    }

    renderer_add_queue_uni(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);

    renderer_draw_uni(glob->rend_res.shaders[0]);

    // NOTE: Rendering plane texture
    renderer_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                              texcoords_in_texture_space(
                                  p->current_animation * 32.f, 0.f,
                                  32.f, 8.f,
                                  &glob->rend_res.global_sprite, false));

    renderer_draw_tex(glob->rend_res.shaders[1], &glob->rend_res.global_sprite);
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
    float dest_x = center ?
                   rec->pos.x + rec->dim.x*0.5f :
                   rec->pos.x;
    cam->pos.x = lerp(cam->pos.x,
                      dest_x,
                      glob->state.delta_time * cam->speed_multiplier);
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
