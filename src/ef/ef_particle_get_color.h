/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the particle colour getters, owned by `ef/ef_particle.cpp`; spelled as the draw strategies call
 * them.  GUESS names (from the callers' use): `ef_particle_get_color`, `ef_particle_get_alpha`, `ef_particle_flick_alpha`.
 */
#ifndef MHTRI_EF_EF_PARTICLE_GET_COLOR_H
#define MHTRI_EF_EF_PARTICLE_GET_COLOR_H

#include "types.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB058 - copies colour [layer][index] of the particle into `color` (rgb). */
void ef_particle_get_color(EfParticle* self, u32 layer, u32 index, u8* color);
/* 0x800AB220 - the alpha of colour [layer][index]. */
u8 ef_particle_get_alpha(EfParticle* self, u32 layer, u32 index);
/* 0x800AB3FC - the particle's current flicker alpha scale. */
u8 ef_particle_flick_alpha(EfParticle* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLE_GET_COLOR_H */
