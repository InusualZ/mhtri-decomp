/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s `pl_ground_check`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_GROUND_CHECK_H
#define MHTRI_PL_PL_GROUND_CHECK_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;
struct LbNpcGround;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802919FC - finds the ground under the player at `pos` (hit_ground_comon); 1 on a hit, the height in `height` */
s32 pl_ground_check(struct _PLW* self, VEC3* pos, struct LbNpcGround* ground, f32* height, s32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GROUND_CHECK_H */
