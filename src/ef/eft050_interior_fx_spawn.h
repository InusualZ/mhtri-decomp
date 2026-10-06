/* Leaf header (docs/plan.md 6.5 rule 2): the `ef/eft050.cpp` symbol the lobby interior effects call.  C linkage (the map
 * row is a plain name). */
#ifndef MHTRI_EF_EFT050_INTERIOR_FX_SPAWN_H
#define MHTRI_EF_EFT050_INTERIOR_FX_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803428A8 - spawns interior effect `id` at `pos` with `scale` (the map kind picks the effect set; GUESS name). */
void eft050_interior_fx_spawn(u8 id, nw4r::math::VEC3* pos, f32 scale, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT050_INTERIOR_FX_SPAWN_H */
