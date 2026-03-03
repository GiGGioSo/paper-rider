#include "pr_particlesystem.h"

#include "pr_globals.h"
#include "pr_mathy.h"

// ### GENERIC FUNCTIONS ###
void particlesystem_init(
    PR_ParticleSystem *ps,
    uint32 particles_number,
    float time_between_particles,
    void (*create_particle)(struct PR_ParticleSystem *, PR_Particle *),
    void (*update_particle)(struct PR_ParticleSystem *, PR_Particle *),
    void (*draw_particle)(struct PR_ParticleSystem *, PR_Particle *)
) {
    PR_ASSERT(ps != NULL);
    PR_ASSERT(time_between_particles > 0.f);

    ps->particles_number = particles_number;
    if (ps->particles_number > 0) {
        ps->particles =
            (PR_Particle *) malloc(sizeof(PR_Particle) *
                                         ps->particles_number);
        PR_ASSERT(ps->particles != NULL);
    }
    ps->current_particle = 0;
    ps->time_between_particles = time_between_particles;
    ps->time_elapsed = 0.f;
    ps->frozen = false;
    ps->active = false;
    ps->all_inactive = true;
    ps->create_particle = create_particle;
    ps->update_particle = update_particle;
    ps->draw_particle = draw_particle;
}

void particlesystem_create_particles(PR_ParticleSystem *ps) {
    PR_ASSERT(ps != NULL);
    PR_ASSERT(ps->particles);

    for(size_t particle_index = 0;
        particle_index < ps->particles_number;
        ++particle_index) {

        PR_Particle *particle = ps->particles + particle_index;
        (ps->create_particle)(ps, particle);
    }
}

void particlesystem_set_active(PR_ParticleSystem *ps, bool active) {
    PR_ASSERT(ps != NULL);
    ps->active = active;
}

void particlesystem_set_time_between_particles(PR_ParticleSystem *ps, float time) {
    PR_ASSERT(ps != NULL);
    PR_ASSERT(time > 0.f);
    ps->time_between_particles = time;
}

void particlesystem_update_and_draw(PR_ParticleSystem *ps, float dt) {
    PR_ASSERT(ps != NULL);

    if (ps->particles_number == 0) return;

    PR_ASSERT(ps->particles != NULL);

    if (ps->active) ps->all_inactive = false;

    if (!ps->active && !ps->all_inactive) {
        ps->all_inactive = true;
        for(size_t particle_index = 0;
                particle_index < ps->particles_number;
                ++particle_index) {

            PR_Particle *particle = ps->particles +
                particle_index;

            if (particle->active) {
                ps->all_inactive = false;
                break;
            }
        }
    }

    if (ps->all_inactive) {
        return;
    }

    if (!ps->frozen) {
        ps->time_elapsed += dt;
        while(ps->time_elapsed > ps->time_between_particles) {
            ps->time_elapsed -= ps->time_between_particles;

            PR_Particle *particle = ps->particles +
                ps->current_particle;
            ps->current_particle = (ps->current_particle + 1) %
                ps->particles_number;

            (ps->create_particle)(ps, particle);
        }
    }
    for (size_t particle_index = 0;
            particle_index < ps->particles_number;
            ++particle_index) {

        PR_Particle *particle = ps->particles + particle_index;

        if (!ps->frozen) {
            (ps->update_particle)(ps, particle);
        }

        (ps->draw_particle)(ps, particle);
    }
}

// ### SPECIFIC FUNCTIONS ###
// # PLANE BOOST #
void particle_create_plane_boost(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    if (ps->active) {
        PR_Plane *p = &glob->current_level.plane;
        particle->body.pos = vec2f_sum(
                p->body.pos,
                vec2f_mult(p->body.dim, 0.5f));
        particle->body.dim.x = 10.f;
        particle->body.dim.y = 10.f;
        particle->body.triangle = false;
        particle->color.r = 1.0f;
        particle->color.g = 1.0f;
        particle->color.b = 1.0f;
        particle->color.a = 1.0f;
        float movement_angle = radiansf(p->body.angle);
        particle->vel.x =
            (vec2f_len(p->vel) - 400.f) * cosf(movement_angle) +
            (float)((rand() % 101) - 50);
        particle->vel.y =
            -(vec2f_len(p->vel) - 400.f) * sinf(movement_angle) +
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
void particle_update_plane_boost(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->vel = vec2f_mult(particle->vel, (1.f - dt)); 
    particle->color.a -= particle->color.a * dt * 3.0f;
    particle->body.pos = vec2f_sum(
            particle->body.pos,
            vec2f_mult(particle->vel, dt));
}
void particle_draw_plane_boost(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_uni_rect(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                            particle->color, true);
}

// # PLANE CRASH #
void particle_create_plane_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    if (ps->active) {
        PR_Plane *p = &glob->current_level.plane;
        //particle->body.pos = p->body.pos + p->body.dim*0.5f;
        particle->body.dim.x = 15.f;
        particle->body.dim.y = 15.f;
        particle->body.pos = vec2f_diff(
                p->crash_position,
                vec2f_mult(particle->body.dim, 0.5f));
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
void particle_update_plane_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos = vec2f_sum(
            particle->body.pos,
            vec2f_mult(particle->vel, dt));
    particle->body.angle -=
        SIGN(particle->vel.x) *
        lerpf(0.f, 720.f, ABS(particle->vel.x)/150.f) * dt;
}
void particle_draw_plane_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_tex_rect(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                           texcoords_in_texture_space(
                                730, 315, 90, 80,
                                glob->rend_res.global_sprite, false),
                           false);
    // renderer_add_queue_uni(rect_in_camera_space(particle->body,
    //                                             &glob->current_level.camera),
    //                         particle->color, true);
}

// # RIDER CRASH #
void particle_create_rider_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    if (ps->active) {
        PR_Rider *rid = &glob->current_level.rider;
        //particle->body.pos = rid->body.pos + rid->body.dim*0.5f;
        particle->body.dim.x = 15.f;
        particle->body.dim.y = 15.f;
        particle->body.pos = vec2f_diff(
                rid->crash_position,
                vec2f_mult(particle->body.dim, 0.5f));
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
void particle_update_rider_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    float dt = glob->state.delta_time;
    particle->color.a -= particle->color.a * dt * 2.0f;
    particle->vel.y += 400.f * dt;
    particle->vel.x *= (1.f - dt);
    particle->body.pos = vec2f_sum(
            particle->body.pos,
            vec2f_mult(particle->vel, dt));
    particle->body.angle -=
        SIGN(particle->vel.x) *
        lerpf(0.f, 720.f, ABS(particle->vel.x)/150.f) * dt;
}
void particle_draw_rider_crash(PR_ParticleSystem *ps,
                                 PR_Particle *particle) {
    UNUSED(ps);
    renderer_add_queue_uni_rect(rect_in_camera_space(particle->body,
                                                &glob->current_level.camera),
                            particle->color, true);
}
