#include <cassert>
#include <cstring>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>

#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_quad_renderer.h"
#include "pp_rect.h"

// FIXME: I probably do not need different update and draw method for each level

// TODO: Add support to render triangles
// TODO: Make the obstacles be possibly triangles, just add a bool to the rect

// TODO:
//  - Proper wind
//  - Find textures
//  - Proper particle system
//  - Sound system
//  - Text rendering
//  - UI

//  - Make changing angle more or less difficult
//  - Make the plane change angle alone when the rider jumps off (maybe)

// TODO: RESEARCH
// - flags you probably want to google: -MF -MMD (clang & gcc)
// - https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

// TODO: Will this always be some arbitrary numbers that kind of just works? YES!!
#define GRAVITY (9.81f * 50.f)

#define PLANE_VELOCITY_LIMIT (1000.f)
#define RIDER_VELOCITY_Y_LIMIT (600.f)
#define RIDER_INPUT_VELOCITY_LIMIT (400.f)

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

#define return_defer(ret) do { result = ret; goto defer; } while(0)

int load_map_from_file(const char *file_path,
                       Rect **obstacles,
                       size_t *number_of_obstacles,
                       PR::BoostPad **boosts,
                       size_t *number_of_boosts) {
    
    int result = 0;
    FILE *map_file;

    {
        map_file = std::fopen(file_path, "r");
        if (map_file == NULL) return_defer(errno);

        std::fscanf(map_file, " %zu", number_of_obstacles);
        if (std::ferror(map_file)) return_defer(errno);

        std::cout << "[LOADING] " << *number_of_obstacles
                  << " obstacles from the file " << file_path
                  << std::endl;

        *obstacles = (Rect *) malloc(sizeof(Rect) * *number_of_obstacles);

        for (size_t obstacle_index = 0;
             obstacle_index < *number_of_obstacles;
             ++obstacle_index) {

            float x, y, w, h, r;

            std::fscanf(map_file,
                        " %f %f %f %f %f",
                        &x, &y, &w, &h, &r);
            if (std::ferror(map_file)) return_defer(errno);

            Rect *rec = *obstacles + obstacle_index;
            rec->pos.x = x;
            rec->pos.y = y;
            rec->dim.x = w;
            rec->dim.y = h;
            rec->angle = r;

            std::cout << "x: " << x
                      << " y: " << y
                      << " w: " << w
                      << " h: " << h
                      << " r: " << r
                      << std::endl;

            if (std::feof(map_file)) {
                *number_of_obstacles = obstacle_index;
                return_defer(errno);
            }
        }

        // NOTE: Loading the boosts from memory
        std::fscanf(map_file, " %zu", number_of_boosts);
        if (std::ferror(map_file)) return_defer(errno);

        std::cout << "[LOADING] " << *number_of_boosts
                  << " boost pads from file " << file_path
                  << std::endl;

        *boosts = (PR::BoostPad *) malloc(sizeof(PR::BoostPad) *
                                   *number_of_boosts);

        for(size_t boost_index = 0;
            boost_index < *number_of_boosts;
            ++boost_index) {
            
            float x, y, w, h, r, ba, bp;

            std::fscanf(map_file,
                        " %f %f %f %f %f %f %f",
                        &x, &y, &w, &h, &r, &ba, &bp);
            if (std::ferror(map_file)) return_defer(errno);

            PR::BoostPad *pad = *boosts + boost_index;
            pad->body.pos.x = x;
            pad->body.pos.y = y;
            pad->body.dim.x = w;
            pad->body.dim.y = h;
            pad->body.angle = r;
            pad->boost_angle = ba;
            pad->boost_power = bp;

            pad->col.r = 0.f;
            pad->col.g = 1.f;
            pad->col.b = 0.f;
            pad->col.a = 1.f;

            std::cout << "x: " << x
                      << " y: " << y
                      << " w: " << w
                      << " h: " << h
                      << " r: " << r
                      << " ba: " << ba
                      << " bp: " << bp
                      << std::endl;

            if (std::feof(map_file)) {
                *number_of_boosts = boost_index;
                return_defer(errno);
            }

        }

    }

    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

int menu_prepare(PR::Level *level) {
    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    uint8_t cursor_pixels[16 * 16 * 4];
    std::memset(cursor_pixels, 0xff, sizeof(cursor_pixels));

    GLFWimage image;
    image.width = 16;
    image.height = 16;
    image.pixels = cursor_pixels;

    GLFWcursor *cursor = glfwCreateCursor(&image, 0, 0);

    // Don't really need to check if the cursor is NULL,
    // because if it is, then the cursor will be set to default
    glfwSetCursor(glob->window.glfw_win, cursor);
    return 0;
}

void menu_update() {
    InputController *input = &glob->input;

    if (input->level1) {
        int preparation_result = level1_prepare(&glob->current_level);
        if (preparation_result == 0) {
            glob->state.current_case = PR::LEVEL1;
            glfwSetInputMode(glob->window.glfw_win,
                             GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    } else
    if (input->level2) {
        int preparation_result = level2_prepare(&glob->current_level);
        if (preparation_result == 0) {
            glob->state.current_case = PR::LEVEL2;
            glfwSetInputMode(glob->window.glfw_win,
                             GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }
    return; 
}

void menu_draw(void) {
    PR::WinInfo *win = &glob->window;

    quad_render_add_queue(win->w * 0.5f, win->h * 0.5f,
                          win->w * 0.5f, win->h * 0.3f,
                          0.f,
                          glm::vec4(1.f, 0.f, 0.f, 1.f),
                          true);

    quad_render_draw(glob->rend.shaders[0]);
    return;
}

int level1_prepare(PR::Level *level) {
    PR::WinInfo *win = &glob->window;

    PR::Plane *p = &level->plane;
    p->crashed = false;
    p->crash_position.x = 0.f;
    p->crash_position.y = 0.f;
    p->body.pos.x = 200.0f;
    p->body.pos.y = 350.0f;
    p->body.dim.y = 23.f;
    p->body.dim.x = p->body.dim.y * 3.f;
    p->body.angle = 0.f;
    p->render_zone.dim.y = 25.f;
    p->render_zone.dim.x = p->render_zone.dim.y * 3.f;
    p->render_zone.pos = p->body.pos +
                         (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;
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



    PR::Rider *rid = &level->rider;
    rid->crashed = false;
    rid->crash_position.x = 0.f;
    rid->crash_position.y = 0.f;
    rid->body.dim.x = 30.f;
    rid->body.dim.y = 50.f;
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
    rid->input_max_accelleration = 4000.f;

    PR::Camera *cam = &level->camera;
    cam->pos.x = p->body.pos.x;
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 3.f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

    level->obstacles_number = 50;
    if (level->obstacles_number) {
        level->obstacles =
            (Rect *) std::malloc(sizeof(Rect) *
                            level->obstacles_number);
        if (level->obstacles == NULL) {
            std::cout << "Buy more RAM!" << std::endl;
            return 1;
        }
    }
    for(size_t obstacle_index = 0;
        obstacle_index < level->obstacles_number;
        ++obstacle_index) {

        Rect *obs = level->obstacles + obstacle_index;

        obs->pos.x = win->w * 0.8f * obstacle_index;
        obs->pos.y = win->h * 0.4f;

        obs->dim.x = win->w * 0.1f;
        obs->dim.y = win->h * 0.3f;

        obs->angle = 0.0f;
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
        pad->boost_angle = pad->body.angle;
        pad->boost_power = 20.f;

        pad->col.r = 0.f;
        pad->col.g = 1.f;
        pad->col.b = 0.f;
        pad->col.a = 1.f;
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
    plane_crash_ps->particles_number = 50;
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
    plane_crash_ps->time_between_particles = 0.03f;
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
    rider_crash_ps->particles_number = 0;
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
    rider_crash_ps->time_between_particles = 0.f;
    rider_crash_ps->time_elapsed = 0.f;
    rider_crash_ps->active = false;
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
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    PR::ParticleSystem *boost_ps =
        &glob->current_level.particle_systems[0];
    PR::ParticleSystem *plane_crash_ps =
        &glob->current_level.particle_systems[1];

    // Global stuff
    PR::WinInfo *win = &glob->window;
    InputController *input = &glob->input;
    float dt = glob->state.delta_time;

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu) {
        if (obstacles_number) free(obstacles);
        if (boosts_number) free(boosts);
        int preparation_result = menu_prepare(&glob->current_level);
        if (preparation_result == 0) {
            glob->state.current_case = PR::MENU;
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

            if (rect_are_colliding(&p->body, &pad->body)) {

                p->acc.x += pad->boost_power *
                            cos(glm::radians(pad->boost_angle));
                p->acc.y += pad->boost_power *
                            -sin(glm::radians(pad->boost_angle));

                // std::cout << "BOOOST!!! against " << boost_index << std::endl;
            }
        }
        // Propulsion
        // TODO: Could this be a "powerup" or something?
        if (glob->input.boost &&
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
        p->acc.y += GRAVITY;

        // NOTE: Motion of the plane
        p->vel += p->acc * dt;
        // NOTE: Limit velocities
        if (glm::length(p->vel) > PLANE_VELOCITY_LIMIT) {
            p->vel *= PLANE_VELOCITY_LIMIT / glm::length(p->vel);
        }
        p->body.pos += p->vel * dt + p->acc * POW2(dt) * 0.5f;
        // DEBUG
        // float vel = 300.f;
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
                    p->body.angle -= 150.f * input->left_right * dt;
                }
                // NOTE: Limiting the angle of the plane
                if (p->body.angle > 360.f) {
                    p->body.angle -= 360.f;
                }
                if (p->body.angle < -360) {
                    p->body.angle += 360.f;
                }
                // NOTE: Make the rider jump based on input
                if (input->jump) {
                    rid->attached = false;
                    rid->base_velocity = p->vel.x;
                    rid->vel.y = p->vel.y - 400.f;
                    rid->body.angle = 0.f;
                    rid->jump_time_elapsed = 0.f;
                }
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached

                // NOTE: Modify accelleration based on input
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
                    rid->input_velocity = glm::sign(rid->input_velocity) *
                                          RIDER_INPUT_VELOCITY_LIMIT;
                }
                rid->base_velocity -= glm::sign(rid->base_velocity) *
                                      rid->air_friction_acc * dt;

                rid->vel.x = rid->base_velocity + rid->input_velocity;
                rid->vel.y += GRAVITY * dt;

                // NOTE: Limit velocity before applying it to the rider
                if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
                    rid->vel.y = glm::sign(rid->vel.y) *
                                 RIDER_VELOCITY_Y_LIMIT ;
                }
                rid->body.pos += rid->vel * dt;
                rid->jump_time_elapsed += dt;

                // NOTE: Check if rider remounts the plane
                if (rect_are_colliding(&p->body, &rid->body) &&
                    rid->jump_time_elapsed > 0.5f) {
                    rid->attached = true;
                    p->vel += (rid->vel - p->vel) * 0.5f;
                    rid->vel *= 0.f;
                }

                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        } else { // rid->crashed
            // rid->crash_particles();
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
        std::cout << "Crash activated" << std::endl;
        if (!rid->crashed) {
            if (rid->attached) {
                move_rider_to_plane(rid, p);
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached

                // NOTE: Modify accelleration based on input
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
                    rid->input_velocity = glm::sign(rid->input_velocity) *
                                          RIDER_INPUT_VELOCITY_LIMIT;
                }
                rid->base_velocity -= glm::sign(rid->base_velocity) *
                                      rid->air_friction_acc * dt;

                rid->vel.x = rid->base_velocity + rid->input_velocity;
                rid->vel.y += GRAVITY * dt;

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
            // rid->crash_particles();
            if (rid->attached) {
                // NOTE: Making the camera move to the plane
                lerp_camera_x_to_rect(cam, &p->body, true);
            } else { // !rid->attached
                // NOTE: Make the camera follow the rider
                lerp_camera_x_to_rect(cam, &rid->body, true);
            }
        }
    }

    // NOTE: Checking collision with obstacles
    for (int obstacle_index = 0;
         obstacle_index < obstacles_number;
         obstacle_index++) {

        Rect *obs = obstacles + obstacle_index;

        if (rect_are_colliding(&p->body, obs)) {
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
        if (rect_are_colliding(&rid->body, obs)) {
            // NOTE: Rider colliding with an obstacle

            rid->crashed = true;
            rid->attached = false;
            rid->crash_position = rid->body.pos;
            rid->vel *= 0.f;
            rid->base_velocity = 0.f;
            rid->input_velocity = 0.f;
            glob->current_level.game_over = true;

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

void level1_draw() {
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    float dt = glob->state.delta_time;

    // NOTE: Rendering the boosts
    for(size_t boost_index = 0;
        boost_index < boosts_number;
        ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

        if (pad_in_cam_pos.pos.x < -((float)win->w) ||
            pad_in_cam_pos.pos.x > win->w * 2.f) continue;

        quad_render_add_queue(pad_in_cam_pos,
                              pad->col,
                              false);
    }

    // NOTE: Rendering the obstacles
    for(size_t obstacle_index = 0;
        obstacle_index < obstacles_number;
        ++obstacle_index) {

        Rect *obs = obstacles + obstacle_index;

        Rect obs_in_cam_pos = rect_in_camera_space(*obs, cam);

        if (obs_in_cam_pos.pos.x < -((float)win->w) ||
            obs_in_cam_pos.pos.x > win->w * 2.f) continue;

        quad_render_add_queue(obs_in_cam_pos,
                              glm::vec4(1.f, 0.2f, 0.2f, 1.f),
                              false);
    }


    // Set the time_between_particles for the boost based on the velocity
    glob->current_level.particle_systems[0].time_between_particles =
        lerp(0.02f, 0.005f, glm::length(p->vel)/PLANE_VELOCITY_LIMIT);

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

            quad_render_add_queue(rect_in_camera_space(particle->body, cam),
                                 particle->color, true);
        }
    }

    // High wind zone
    /* quad_render_add_queue(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false); */

    // NOTE: Rendering the plane
    quad_render_add_queue(rect_in_camera_space(p->body, cam),
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
        quad_render_add_queue(bx, by,
                              p->render_zone.dim.y, p->render_zone.dim.y,
                              p->render_zone.angle,
                              glm::vec4(1.0f, 0.0f, 0.0f, 1.f),
                              true);
    }
    */

    quad_render_add_queue(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);

    quad_render_draw(glob->rend.shaders[0]);

    // NOTE: Rendering plane texture
    quad_render_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                              texcoords_in_texture_space(
				      p->current_animation * 32.f, 0.f,
                                      32.f, 8.f,
                                      &glob->rend.global_sprite));

    quad_render_draw_tex(glob->rend.shaders[1], &glob->rend.global_sprite);
}

int level2_prepare(PR::Level *level) {
    int loading_result = load_map_from_file("level2.prmap",
                                            &level->obstacles,
                                            &level->obstacles_number,
                                            &level->boosts,
                                            &level->boosts_number);
    if (loading_result != 0) return loading_result;

    PR::WinInfo *win = &glob->window;
    PR::Plane *p = &level->plane;

    p->body.pos.x = 100.0f;
    p->body.pos.y = 0.0f;
    p->body.dim.y = 20.f;
    p->body.dim.x = p->body.dim.y * 3.f;
    p->body.angle = 0.f;
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
    rid->body.pos.x = p->render_zone.pos.x + (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f * sin(glm::radians(rid->body.angle)) -
                (p->render_zone.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
    rid->body.pos.y = p->render_zone.pos.y + (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
                (p->render_zone.dim.y + rid->body.dim.y)*0.5f * cos(glm::radians(rid->body.angle)) +
                (p->render_zone.dim.x*0.2f) * sin(glm::radians(rid->body.angle));
    rid->render_zone.dim = rid->body.dim;
    rid->render_zone.pos = rid->body.pos + (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->vel.x = 0.0f;
    rid->vel.y = 0.0f;
    rid->body.angle = p->body.angle;
    rid->attached = true;
    rid->mass = 0.010f;
    rid->jump_time_elapsed = 0.f;
    rid->air_friction_acc = 100.f;
    rid->base_velocity = 0.f;
    rid->input_velocity = 0.f;
    rid->input_max_accelleration = 4000.f;

    PR::Camera *cam = &level->camera;
    cam->pos.x = p->body.pos.x;
    cam->pos.y = win->h * 0.5f;
    cam->speed_multiplier = 3.f;

    PR::Atmosphere *air = &level->air;
    air->density = 0.015f;

    return 0;
}

void level2_update() {
    // Level stuff
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    InputController *input = &glob->input;
    float dt = glob->state.delta_time;

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu) {
        // Free the stuff only if it was allocated
        if (obstacles_number) free(obstacles);
        if (boosts_number) free(boosts);
        int preparation_result = menu_prepare(&glob->current_level);
        if (preparation_result == 0) {
            glob->state.current_case = PR::MENU;
        }
    }

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;

    apply_air_resistances(p);

    // NOTE: Checking collision with boost_pads
    for (size_t boost_index = 0;
         boost_index < boosts_number;
         ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        if (rect_are_colliding(&p->body, &pad->body)) {

            p->acc.x += pad->boost_power * cos(glm::radians(pad->boost_angle));
            p->acc.y += pad->boost_power * -sin(glm::radians(pad->boost_angle));

            // std::cout << "BOOOST!!! against " << boost_index << std::endl;
        }
    }

    // Propulsion
    // TODO: Could this be a "powerup" or something?
    if (input->boost) {
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
        if (input->jump) {
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
        if (glm::abs(rid->input_velocity) > RIDER_INPUT_VELOCITY_LIMIT) rid->input_velocity = glm::sign(rid->input_velocity) * RIDER_INPUT_VELOCITY_LIMIT;
        rid->base_velocity -= glm::sign(rid->base_velocity) * rid->air_friction_acc * dt;

        rid->vel.x = rid->base_velocity + rid->input_velocity;
        rid->vel.y += GRAVITY * dt;

        if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT) {
            rid->vel.y = glm::sign(rid->vel.y) *
                         RIDER_VELOCITY_Y_LIMIT ;
        }

        rid->body.pos += rid->vel * dt;

        rid->jump_time_elapsed += dt;

        // NOTE: If rider not attached, check if recollided
        if (rect_are_colliding(&p->body, &rid->body) &&
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

        Rect *obs = obstacles + obstacle_index;

        if (rect_are_colliding(&p->body, obs)) {
            // NOTE: Plane colliding with an obstacle

            if (obstacles_number) free(obstacles);
            if (boosts_number) free(boosts);
            int preparation_result = menu_prepare(&glob->current_level);
            if (preparation_result == 0) {
                glob->state.current_case = PR::MENU;
            }

            // TODO: Debug flag
            std::cout << "Plane collided with " << obstacle_index << std::endl;
        }
        if (rect_are_colliding(&rid->body, obs)) {
            // NOTE: Rider colliding with an obstacle

            if (obstacles_number) free(obstacles);
            if (boosts_number) free(boosts);
            int preparation_result = menu_prepare(&glob->current_level);
            if (preparation_result == 0) {
                glob->state.current_case = PR::MENU;
            }

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
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    // Global stuff
    PR::WinInfo *win = &glob->window;
    // float dt = glob->state.delta_time;

    // High wind zone
    /* quad_render_add_queue(0.f, win->h * 0.6f, win->w, win->h * 0.4f, 0.f, glm::vec3(0.2f, 0.3f, 0.6f), false); */

    // NOTE: Rendering the boosts
    for(size_t boost_index = 0;
        boost_index < boosts_number;
        ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        Rect pad_in_cam_pos = rect_in_camera_space(pad->body, cam);

        if (pad_in_cam_pos.pos.x < -((float)win->w) ||
            pad_in_cam_pos.pos.x > win->w * 2.f) continue;

        quad_render_add_queue(pad_in_cam_pos,
                              pad->col,
                              false);
    }

    // NOTE: Rendering the obstacles
    for(size_t obstacle_index = 0;
        obstacle_index < obstacles_number;
        obstacle_index++) {

        Rect *obs = obstacles + obstacle_index;

        Rect obs_in_cam_pos = rect_in_camera_space(*obs, cam);

        if (obs_in_cam_pos.pos.x < -((float)win->w) ||
            obs_in_cam_pos.pos.x > win->w * 2.f) continue;

        quad_render_add_queue(obs_in_cam_pos,
                              glm::vec4(1.f, 0.2f, 0.2f, 1.f),
                              false);

    }

    // NOTE: Rendering the plane
    quad_render_add_queue(rect_in_camera_space(p->body, cam),
                          glm::vec4(1.0f, 1.0f, 1.0f, 1.f),
                          false);

    // TODO: Implement the boost as an actual thing
    glm::vec2 p_cam_pos = rect_in_camera_space(p->render_zone, cam).pos;
    if (glob->input.boost) {
        float bx = p_cam_pos.x + p->render_zone.dim.x*0.5f -
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f *
                    cos(glm::radians(p->render_zone.angle));
        float by = p_cam_pos.y + p->render_zone.dim.y*0.5f +
                    (p->render_zone.dim.x+p->render_zone.dim.y)*0.5f *
                    sin(glm::radians(p->render_zone.angle));

        // NOTE: Rendering the boost of the plane
        quad_render_add_queue(bx, by,
                              p->render_zone.dim.y, p->render_zone.dim.y,
                              p->render_zone.angle,
                              glm::vec4(1.0f, 0.0f, 0.0f, 1.f),
                              true);
    }

    quad_render_add_queue(rect_in_camera_space(rid->render_zone, cam),
                          glm::vec4(0.0f, 0.0f, 1.0f, 1.f),
                          false);

    quad_render_draw(glob->rend.shaders[0]);

    // NOTE: Rendering plane texture
    quad_render_add_queue_tex(rect_in_camera_space(p->render_zone, cam),
                              texcoords_in_texture_space(p->current_animation * 32.f, 0.f,
                                                         32.f, 8.f,
                                                         &glob->rend.global_sprite));

    quad_render_draw_tex(glob->rend.shaders[1], &glob->rend.global_sprite);
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
        particle->body.pos = p->body.pos + p->body.dim*0.5f;
        particle->body.dim.x = 15.f;
        particle->body.dim.y = 15.f;
        particle->body.angle = 0.f;
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
    } else {
        particle->body.pos.x = 0.f;
        particle->body.pos.y = 0.f;
        particle->body.dim.x = 0.f;
        particle->body.dim.y = 0.f;
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
    return;
}
