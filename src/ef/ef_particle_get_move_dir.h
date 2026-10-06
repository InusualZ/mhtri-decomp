/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the particle move-direction getter, owned by `ef/ef_emitter.cpp`; spelled
 * as the draw strategies call it.  GUESS name `ef_particle_get_move_dir`: the body writes the particle's current
 * position minus its previous one into `out` and returns `out`.
 */
#ifndef MHTRI_EF_EF_PARTICLE_GET_MOVE_DIR_H
#define MHTRI_EF_EF_PARTICLE_GET_MOVE_DIR_H

#include "types.h"
#include "ef/ef_drawstrategy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A7F00 (0x140): writes the particle's move direction into `out` and returns `out`. */
VEC3* ef_particle_get_move_dir(EfDrawParticle* p, VEC3* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLE_GET_MOVE_DIR_H */
