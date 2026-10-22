/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s `npc_ground_check`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_NPC_GROUND_CHECK_H
#define MHTRI_PL_NPC_GROUND_CHECK_H

#include "types.h"
#include "nw4r/math.h"

struct _LB_NPC;
struct LbNpcGround;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80291B48 - finds the ground under NPC `self` at `pos` (hit_ground_comon); 1 on a hit */
s32 npc_ground_check(struct _LB_NPC* self, VEC3* pos, struct LbNpcGround* ground, f32* height, s32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_NPC_GROUND_CHECK_H */
