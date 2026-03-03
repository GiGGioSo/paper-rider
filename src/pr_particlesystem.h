#ifndef _PR_PARTICLESYSTEM_H_
#define _PR_PARTICLESYSTEM_H_

#include "pr_types.h"

// ### GENERIC FUNCTIONS ###
void
particlesystem_init(PR_ParticleSystem *ps, uint32 particles_number, float time_between_particles, void (*create_particle)(struct PR_ParticleSystem *, PR_Particle *), void (*update_particle)(struct PR_ParticleSystem *, PR_Particle *), void (*draw_particle)(struct PR_ParticleSystem *, PR_Particle *));

void
particlesystem_create_particles(PR_ParticleSystem *ps);

void
particlesystem_set_active(PR_ParticleSystem *ps, bool active);

void
particlesystem_set_time_between_particles(PR_ParticleSystem *ps, float time);

void
particlesystem_update_and_draw(PR_ParticleSystem *ps, float dt);

// ### SPECIFIC FUNCTIONS ###
// # PLANE BOOST #
void
particle_create_plane_boost(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_update_plane_boost(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_draw_plane_boost(PR_ParticleSystem *ps, PR_Particle *particle);

// # PLANE CRASH #
void
particle_create_plane_crash(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_update_plane_crash(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_draw_plane_crash(PR_ParticleSystem *ps, PR_Particle *particle);

// # RIDER CRASH #
void
particle_create_rider_crash(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_update_rider_crash(PR_ParticleSystem *ps, PR_Particle *particle);
void
particle_draw_rider_crash(PR_ParticleSystem *ps, PR_Particle *particle);

#endif//_PR_PARTICLESYSTEM_H_
