/* ef/eft026_pl_spawn.h - the declaration of `eft026_pl_spawn`, which `ef/eft026_fx.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT026_PL_SPAWN_H
#define MHTRI_EF_EFT026_PL_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void eft026_pl_spawn(struct _EFT26_EM* em, u8 kind, nw4r::math::VEC3* vec, u32 joint, f32 scale);
#ifdef __cplusplus
}
#endif

#endif
