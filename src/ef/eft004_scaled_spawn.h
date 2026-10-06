/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft004_fx.cpp`'s one-effect spawn the note pane calls.  C linkage. */
#ifndef MHTRI_EF_EFT004_SCALED_SPAWN_H
#define MHTRI_EF_EFT004_SCALED_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80101670 - spawns a one-effect eft004 pool at `pos` in area `area` with scale `scale` (GUESS name). */
void eft004_scaled_spawn(nw4r::math::VEC3* pos, u8 area, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_SCALED_SPAWN_H */
