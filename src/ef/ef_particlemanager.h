#ifndef MHTRI_EF_EF_PARTICLEMANAGER_H
#define MHTRI_EF_EF_PARTICLEMANAGER_H

#include "types.h"
#include "nw4r/math.h"

/* Declarations for the symbols `src/ef/ef_particlemanager.cpp` owns (docs/plan.md 6.5, rule 2).  The
 * signature is the one the consumer (ef/ef_drawfreestrategy.cpp) calls with; the owner's stub definition
 * must match it.  C-visible, kept minimal. */
#ifdef __cplusplus
extern "C" {
#endif

/* The particle-manager ramp helper `ef/ef_animcurve.cpp` calls at a ramp's last key
 * (fn_800A1504).  The owner's own definition is `fn_800AB880(EfPmManager*, EfPmParticle*)`; the tag
 * is enough here, so the header declares it with a forward declaration. */
struct EfPmParticle;
s32 fn_800AB880(struct EfPmManager* self, struct EfPmParticle* target);

void fn_800AE360(void* target, MTX34* out); /* the per-particle transform fn_800BE3C0 reads */

/* Retires every live particle of the manager and returns how many it walked (`ef/eft019.cpp`'s
 * effect-object teardown calls it).  The owner's `EfPmManager` is complete in its own file, so the
 * declaration only needs the tag. */
struct EfPmManager;
s32 fn_800AB9F4(struct EfPmManager* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLEMANAGER_H */
