#include <cassert>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>

#include "pp_game.h"
#include "pp_globals.h"
#include "pp_input.h"
#include "pp_quad_renderer.h"
#include "pp_rect.h"


// TODO:
//  - Proper wind
//  - Find textures
//  - Proper particle system
//  - Sound system

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

#define return_defer(ret) do { result = ret; goto defer; } while(0)

int load_map_from_file(const char *file_path,
                       Rect **obstacles,
                       size_t *number_of_obstacles) {
    
    int result = 0;
    FILE *map_file;

    {
        map_file = std::fopen(file_path, "r");
        if (map_file == NULL) return_defer(errno);

        std::fscanf(map_file, "%zu", number_of_obstacles);
        if (std::ferror(map_file)) return_defer(errno);

        std::cout << "Loading " << *number_of_obstacles
                  << " obstacles from the file " << file_path
                  << std::endl;

        *obstacles = (Rect *) malloc(sizeof(Rect) * *number_of_obstacles);

        for (size_t obstacle_index = 0;
             obstacle_index < *number_of_obstacles;
             ++obstacle_index) {

            int x, y, w, h, r;

            std::fscanf(map_file, " %i %i %i %i %i", &x, &y, &w, &h, &r);
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
    }
    defer:
    if (map_file) std::fclose(map_file);
    return result;
}

void menu_update(float dt) {
    InputController *input = &glob->input;

    if (input->level1) {
        int preparation_result = level1_prepare(&glob->current_level);
        if (preparation_result == 0) glob->current_state = PR::LEVEL1;
    } else
    if (input->level2) {
        int preparation_result = level2_prepare(&glob->current_level);
        if (preparation_result == 0) glob->current_state = PR::LEVEL2;
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


struct Particle {
    Rect body;
    glm::vec2 vel;
    glm::vec4 color;
};

struct ParticleSystem {
    Particle particles[200];
    int current_particle = 0;
    float time_between_particles;
    float time_elapsed;
};

ParticleSystem ps;

int level1_prepare(PR::Level *level) {
    PR::WinInfo *win = &glob->window;

    PR::Plane *p = &level->plane;
    p->body.pos.x = 200.0f;
    p->body.pos.y = 350.0f;
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

    level->obstacles_number = 50;
    level->obstacles = (Rect *) malloc(sizeof(Rect) * level->obstacles_number);
    if (level->obstacles == NULL) {
        std::cout << "Buy more RAM!" << std::endl;
        return 1;
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
    level->boosts = (PR::BoostPad *) malloc(sizeof(PR::BoostPad) *
                                       level->boosts_number);
    if (level->boosts == NULL) {
        std::cout << "Buy more RAM!" << std::endl;
        return 1;
    }
    for (size_t boost_index = 0;
         boost_index < level->boosts_number;
         ++boost_index) {
        PR::BoostPad *pad = level->boosts + boost_index;

        pad->body.pos.x = (boost_index + 1) * 1100.f;
        pad->body.pos.y = 300.f;
        pad->body.dim.x = 700.f;
        pad->body.dim.y = 200.f;
        pad->body.angle = 0.f;
        pad->boost_angle = pad->body.angle;
        pad->boost_power = 2.f;

        pad->col.r = 0.f;
        pad->col.g = 1.f;
        pad->col.b = 0.f;
        pad->col.a = 1.f;
    }

    return 0;
}

void level1_update(float dt) {
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

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu) {
        if (obstacles_number) free(obstacles);
        if (boosts_number) free(boosts);
        glob->current_state = PR::MENU;
    }

    // NOTE: Reset the accelleration for it to be recalculated
    p->acc *= 0;

    apply_air_resistances(p);

    // Propulsion
    // TODO: Could this be a "powerup" or something?
    if (glob->input.boost) {
        float propulsion = 8.f;
        p->acc.x += propulsion * cos(glm::radians(p->body.angle));
        p->acc.y += propulsion * -sin(glm::radians(p->body.angle));
        /* std::cout << "PROP.x: " << propulsion * cos(glm::radians(p->body.angle)) */
        /*             << ",  PROP.y: " << propulsion * -sin(glm::radians(p->body.angle)) << "\n"; */
    }

    // NOTE: Checking collision with boost_pads
    for (size_t boost_index = 0;
         boost_index < boosts_number;
         ++boost_index) {

        PR::BoostPad *pad = boosts + boost_index;

        if (rect_are_colliding(&p->body, &pad->body)) {

            p->acc.x += pad->boost_power * cos(glm::radians(pad->boost_angle));
            p->acc.y += pad->boost_power * sin(glm::radians(pad->boost_angle));

            std::cout << "BOOOST!!! against " << boost_index << std::endl;
        }
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

        rid->body.angle = p->body.angle;
        rid->body.pos.x = p->body.pos.x + (p->body.dim.x - rid->body.dim.x)*0.5f -
                    (p->body.dim.y + rid->body.dim.y)*0.5f * sin(glm::radians(rid->body.angle)) -
                    (p->body.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
        rid->body.pos.y = p->body.pos.y + (p->body.dim.y - rid->body.dim.y)*0.5f -
                    (p->body.dim.y + rid->body.dim.y)*0.5f * cos(glm::radians(rid->body.angle)) +
                    (p->body.dim.x*0.2f) * sin(glm::radians(rid->body.angle));


        // NOTE: If the rider is attached, make it jump
        if (input->jump) {
            rid->attached = false;
            rid->base_velocity = p->vel.x;
            rid->vel.y = p->vel.y - 400.f;
            rid->jump_time_elapsed = 0.f;
        }
    } else {
        cam->pos.x = lerp(cam->pos.x, rid->render_zone.pos.x+rid->render_zone.dim.x*0.5f, dt * cam->speed_multiplier);

        if (input->left_right) {
            rid->input_velocity += rid->input_max_accelleration * input->left_right * dt;
        } else {
            rid->input_velocity += rid->input_max_accelleration * -glm::sign(rid->input_velocity) * dt;
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

        if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT)
            rid->vel.y = RIDER_VELOCITY_Y_LIMIT * glm::sign(rid->vel.y);

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
            glob->current_state = PR::MENU;

            // TODO: Debug flag
            std::cout << "Plane collided with " << obstacle_index << std::endl;
        }
        if (rect_are_colliding(&rid->body, obs)) {
            // NOTE: Rider colliding with an obstacle

            if (obstacles_number) free(obstacles);
            if (boosts_number) free(boosts);
            glob->current_state = PR::MENU;

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
    p->render_zone.pos = p->body.pos + (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;

    rid->render_zone.pos = rid->body.pos + (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->render_zone.angle = rid->body.angle;

    // NOTE: Animation based on the accelleration
    if (p->animation_countdown < 0) {
        if (glm::abs(p->acc.y) < 20.f) {
            p->current_animation = PR::Plane::IDLE_ACC;
            p->animation_countdown = 0.25f;
        } else {
            if (p->current_animation == PR::Plane::UPWARDS_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::IDLE_ACC : PR::Plane::UPWARDS_ACC;
            else if (p->current_animation == PR::Plane::IDLE_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::DOWNWARDS_ACC : PR::Plane::UPWARDS_ACC;
            else if (p->current_animation == PR::Plane::DOWNWARDS_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::DOWNWARDS_ACC : PR::Plane::IDLE_ACC;

            p->animation_countdown = 0.25f;
        }
    } else {
        p->animation_countdown -= dt;
    }
}

void level1_draw(float dt) {
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    PR::WinInfo *win = &glob->window;

    ps.time_between_particles = lerp(0.02f, 0.005f, glm::length(p->vel)/PLANE_VELOCITY_LIMIT);

    ps.time_elapsed += dt;
    if (ps.time_elapsed > ps.time_between_particles) {
        ps.time_elapsed -= ps.time_between_particles;

        Particle *particle = &ps.particles[ps.current_particle++];
        ps.current_particle = ps.current_particle % ARRAY_LENGTH(ps.particles);

        particle->body.pos = p->body.pos + p->body.dim * 0.5f;
        particle->body.dim.x = 10.f;
        particle->body.dim.y = 10.f;
        particle->color.r = 1.0f;
        particle->color.g = 1.0f;
        particle->color.b = 1.0f;
        particle->color.a = 1.0f;
        float movement_angle = glm::radians(p->body.angle);
        particle->vel.x = (glm::length(p->vel) - 400.f) * cos(movement_angle) + (float)((rand() % 101) - 50);
        particle->vel.y = -(glm::length(p->vel) - 400.f) * sin(movement_angle) + (float)((rand() % 101) - 50);
    }

    for (size_t particle_index = 0;
         particle_index < ARRAY_LENGTH(ps.particles);
         ++particle_index) {

        Particle *particle = &ps.particles[particle_index];

        particle->vel *= (1.f - dt); 
        particle->color.a -= particle->color.a * dt * 3.0f;
        particle->body.pos += particle->vel * dt;

        quad_render_add_queue(rect_in_camera_space(particle->body, cam),
                             particle->color, true);
    }

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
        ++obstacle_index) {

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

int level2_prepare(PR::Level *level) {
    int loading_result = load_map_from_file("level2.prmap",
                                            &level->obstacles,
                                            &level->obstacles_number);
    if (loading_result != 0) return loading_result;

    PR::WinInfo *win = &glob->window;
    PR::Plane *p = &level->plane;

    p->body.pos.x = 100.0f;
    p->body.pos.y = 0.0f;
    p->body.dim.y = 30.f;
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

    level->boosts_number = 0;
    if (level->boosts_number) {
        level->boosts = (PR::BoostPad *) malloc(sizeof(PR::BoostPad) *
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

        pad->body.pos.x = 1000.f;
        pad->body.pos.y = 200.f;
        pad->body.dim.x = 1000.f;
        pad->body.dim.y = 300.f;
        pad->body.angle = 0.f;
        pad->boost_angle = pad->body.angle;
        pad->boost_power = 20.f;

        pad->col.r = 0.f;
        pad->col.g = 1.f;
        pad->col.b = 0.f;
        pad->col.a = 1.f;
    }

    return 0;
}

void level2_update(float dt) {
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

    assert(-360.f <= p->body.angle && p->body.angle <= 360.f);

    if (input->menu) {
        // Free the stuff only if it was allocated
        if (obstacles_number) free(obstacles);
        if (boosts_number) free(boosts);
        glob->current_state = PR::MENU;
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
            p->acc.y += pad->boost_power * sin(glm::radians(pad->boost_angle));

            std::cout << "BOOOST!!! against " << boost_index << std::endl;
        }
    }

    // Propulsion
    // TODO: Could this be a "powerup" or something?
    if (glob->input.boost) {
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
    // Apply gravity based on the mass
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
        rid->body.pos.x = p->render_zone.pos.x + (p->render_zone.dim.x - rid->body.dim.x)*0.5f -
                    (p->render_zone.dim.y + rid->body.dim.y)*0.5f * sin(glm::radians(rid->body.angle)) -
                    (p->render_zone.dim.x*0.2f) * cos(glm::radians(rid->body.angle));
        rid->body.pos.y = p->render_zone.pos.y + (p->render_zone.dim.y - rid->body.dim.y)*0.5f -
                    (p->render_zone.dim.y + rid->body.dim.y)*0.5f * cos(glm::radians(rid->body.angle)) +
                    (p->render_zone.dim.x*0.2f) * sin(glm::radians(rid->body.angle));


        // NOTE: If the rider is attached, make it jump
        if (input->jump) {
            rid->attached = false;
            rid->base_velocity = p->vel.x;
            rid->vel.y = p->vel.y - 400.f;
            rid->jump_time_elapsed = 0.f;
        }
    } else {
        cam->pos.x = lerp(cam->pos.x, rid->render_zone.pos.x+rid->render_zone.dim.x*0.5f, dt * cam->speed_multiplier);

        if (input->left_right) {
            rid->input_velocity += rid->input_max_accelleration * input->left_right * dt;
        } else {
            rid->input_velocity += rid->input_max_accelleration * -glm::sign(rid->input_velocity) * dt;
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

        if (glm::abs(rid->vel.y) > RIDER_VELOCITY_Y_LIMIT)
            rid->vel.y = RIDER_VELOCITY_Y_LIMIT * glm::sign(rid->vel.y);

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
            glob->current_state = PR::MENU;

            // TODO: Debug flag
            std::cout << "Plane collided with " << obstacle_index << std::endl;
        }
        if (rect_are_colliding(&rid->body, obs)) {
            // NOTE: Rider colliding with an obstacle

            if (obstacles_number) free(obstacles);
            if (boosts_number) free(boosts);
            glob->current_state = PR::MENU;

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
    p->render_zone.pos = p->body.pos + (p->body.dim - p->render_zone.dim) * 0.5f;
    p->render_zone.angle = p->body.angle;

    rid->render_zone.pos = rid->body.pos + (rid->body.dim - rid->render_zone.dim) * 0.5f;
    rid->render_zone.angle = rid->body.angle;

    // NOTE: Animation based on the accelleration
    if (p->animation_countdown < 0) {
        if (glm::abs(p->acc.y) < 20.f) {
            p->current_animation = PR::Plane::IDLE_ACC;
            p->animation_countdown = 0.25f;
        } else {
            if (p->current_animation == PR::Plane::UPWARDS_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::IDLE_ACC : PR::Plane::UPWARDS_ACC;
            else if (p->current_animation == PR::Plane::IDLE_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::DOWNWARDS_ACC : PR::Plane::UPWARDS_ACC;
            else if (p->current_animation == PR::Plane::DOWNWARDS_ACC)
                p->current_animation = (p->acc.y > 0) ? PR::Plane::DOWNWARDS_ACC : PR::Plane::IDLE_ACC;

            p->animation_countdown = 0.25f;
        }
    } else {
        p->animation_countdown -= dt;
    }
}

void level2_draw(float dt) {
    PR::Plane *p = &glob->current_level.plane;
    PR::Camera *cam = &glob->current_level.camera;
    PR::Rider *rid = &glob->current_level.rider;
    size_t obstacles_number = glob->current_level.obstacles_number;
    Rect *obstacles = glob->current_level.obstacles;
    size_t boosts_number = glob->current_level.boosts_number;
    PR::BoostPad *boosts = glob->current_level.boosts;

    PR::WinInfo *win = &glob->window;

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

