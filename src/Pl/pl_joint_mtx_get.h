/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_joint_mtx_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_JOINT_MTX_GET_H
#define MHTRI_PL_PL_JOINT_MTX_GET_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026A394 - returns joint `joint`'s matrix of the player's model (mhchar_joint_mtx_get) */
void pl_joint_mtx_get(struct _PLW* plw, s32 joint, nw4r::math::MTX34* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_JOINT_MTX_GET_H */
