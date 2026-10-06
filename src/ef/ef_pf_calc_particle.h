/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `ef_pf_calc_particle`, owned by `ef/ef_postfield.cpp`; spelled as that
 * unit defines it, for the particle manager's calc pass, except `offset`: the caller passes a copy (by value), which
 * `ef/ef_postfield.cpp` receives as `VEC3*` (the same argument).
 */
#ifndef MHTRI_EF_EF_PF_CALC_PARTICLE_H
#define MHTRI_EF_EF_PF_CALC_PARTICLE_H

#include "types.h"
#include "nw4r/math.h"

struct EfDrawParticle;
struct EfPostFieldTransform;
struct EfPostFieldInfo;
struct EffectHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800B1C80 (0x724): applies a post field to one particle; 0 when the particle was moved or killed here. */
s32 ef_pf_calc_particle(struct EfDrawParticle* p, const struct EfPostFieldTransform* xform,
                        struct EfPostFieldInfo* info, struct EffectHandle* eh, const MTX34* emitter_mtx,
                        const MTX34* pm_mtx, const VEC3* pos, VEC3 offset, VEC3* velocity, u8* killed);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PF_CALC_PARTICLE_H */
