/* ef/ef_particlemanager.h - the symbols `ef/ef_particlemanager.cpp` owns that other units call (C linkage),
 * spelled as the consumers call them. */
#ifndef MHTRI_EF_EF_PARTICLEMANAGER_H
#define MHTRI_EF_EF_PARTICLEMANAGER_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The ramp helper `ef/ef_animcurve.cpp`'s fn_800A1504 calls at a ramp's last key (declared with the
 * owner's record tags). */
struct EfPmParticle;
s32 fn_800AB880(struct EfPmManager* self, struct EfPmParticle* target);

void ef_pm_get_mtx(void* target, MTX34* out); /* the per-particle transform fn_800BE3C0 reads */

/* Retires every live particle of the manager and returns how many it walked (`ef/eft019.cpp`'s
 * effect-object teardown calls it). */
struct EfPmManager;
s32 fn_800AB9F4(struct EfPmManager* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLEMANAGER_H */
