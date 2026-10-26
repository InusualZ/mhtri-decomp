/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the particle manager's constructor and deleting destructor, owned by
 * `ef/ef_particlemanager.cpp` (C linkage there).  GUESS names `ef_particle_manager_ctor`/`ef_particle_manager_delete`:
 * the owner documents the first as the ParticleManager constructor, and the second frees a live object when its flag
 * is positive; the effect memory manager's particle-manager pool hands both to `__construct_new_array`.
 */
#ifndef MHTRI_EF_EF_PARTICLE_MANAGER_CTOR_H
#define MHTRI_EF_EF_PARTICLE_MANAGER_CTOR_H

#include "types.h"

struct EfPmManager;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB664 (0x58): Constructs a particle manager in place and returns it. */
struct EfPmManager* ef_particle_manager_ctor(struct EfPmManager* self);
/* 0x800AC178 (0x44): Frees the particle manager when it is live and the flag is positive; returns it. */
/* untyped: opaque handle passed through - the object being destroyed */
void* ef_particle_manager_delete(void* p, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLE_MANAGER_CTOR_H */
