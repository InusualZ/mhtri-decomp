/* ef/ef_particlemanager.h - the symbols `ef/ef_particlemanager.cpp` owns that other units call (C linkage),
 * spelled as the consumers call them. */
#ifndef MHTRI_EF_EF_PARTICLEMANAGER_H
#define MHTRI_EF_EF_PARTICLEMANAGER_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB658 (0xC) - the sine of an angle in radians (the angle scaled onto the 256-step index table). */
f32 fn_800AB658(f32 x);

/* 0x800AB740 - puts `target` on the manager's closing list and marks it retired (`ef/ef_particle.cpp`'s
 * closing slot hands its particle over through it). */
struct EfPmManager;
struct EfPmParticle;
s32 fn_800AB740(struct EfPmManager* self, struct EfPmParticle* target);

/* The ramp helper `ef/ef_animcurve.cpp`'s ef_anim_tex_ramp calls at a ramp's last key (declared with the
 * owner's record tags). */
s32 fn_800AB880(struct EfPmManager* self, struct EfPmParticle* target);

/* 0x800AE360 - copies the manager's matrix (rebuilt from its emitter's when dirty) into `out` and returns it. */
/* untyped: opaque handle - the draw strategies pass the manager through their own record views */
MTX34* ef_pm_get_mtx(void* target, MTX34* out);

/* Retires every live particle of the manager and returns how many it walked (`ef/eft019.cpp`'s
 * effect-object teardown calls it). */
struct EfPmManager;
s32 fn_800AB9F4(struct EfPmManager* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLEMANAGER_H */
