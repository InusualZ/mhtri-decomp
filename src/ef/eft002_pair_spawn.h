/* ef/eft002_pair_spawn.h - leaf header: `eft002_pair_spawn` (0x800FBE64), owned by `ef/eft002.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT002_PAIR_SPAWN_H
#define MHTRI_EF_EFT002_PAIR_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the pooled two-effect record of `type` at `pos` for the hit's owner, with colour set `color_set`. */
void eft002_pair_spawn(VEC3* pos, u32 type, struct _HIT_W* src, u8 color_set);
#ifdef __cplusplus
}
#endif

#endif
