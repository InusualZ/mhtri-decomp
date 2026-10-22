/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_motion_speed_apply`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_MOTION_SPEED_APPLY_H
#define MHTRI_PL_PL_MOTION_SPEED_APPLY_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026A2BC - applies the motion speed to the player's model (mhchar_motion_speed_set) */
void pl_motion_speed_apply(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MOTION_SPEED_APPLY_H */
