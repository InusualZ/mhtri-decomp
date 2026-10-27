/* ef/eft013_guard_spawn.h - leaf header: `eft013_guard_spawn` (0x80106DC8), owned by `ef/fn_80105314.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT013_GUARD_SPAWN_H
#define MHTRI_EF_EFT013_GUARD_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the type-16 spark effect at `pos` turned by `rot` (two angle words), scaled by `scale`, following the hit's
 * target while it has one. */
void eft013_guard_spawn(VEC3* pos, u32* rot, f32 scale, struct _HIT_W* hit);
#ifdef __cplusplus
}
#endif

#endif
