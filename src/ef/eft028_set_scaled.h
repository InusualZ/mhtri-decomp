/* ef/eft028_set_scaled.h - the declaration of `eft028_set_scaled`, which `ef/eft026_fx.cpp` owns (docs/plan.md 6.5
 * rule 2); the signature is the definition's. */
#ifndef MHTRI_EF_EFT028_SET_SCALED_H
#define MHTRI_EF_EFT028_SET_SCALED_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80119BB0 - spawns an eft028 record of `kind` and `variant` at `pos` with rotation `rot`, `scale` and `timer`
 * (the setter `eft028_set_koware` has no rotation or scale). */
void eft028_set_scaled(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 variant, long timer);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT028_SET_SCALED_H */
