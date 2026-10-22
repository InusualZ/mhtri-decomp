/* ef/eft004_fx.h - the declarations of `ef/eft004_fx.cpp`'s symbols its consumers call (docs/plan.md 6.5 rule 2).
 * C linkage: the map rows are plain names. */
#ifndef MHTRI_EF_EFT004_FX_H
#define MHTRI_EF_EFT004_FX_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801006A0 - spawns an effect from a vector, then stamps its key byte (its callers load the scale third). */
void eft004_key_spawn(u8 type, nw4r::math::VEC3* pos, u32 param, u8 flag, u8 key, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_FX_H */
