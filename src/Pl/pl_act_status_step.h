/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s `pl_act_status_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_ACT_STATUS_STEP_H
#define MHTRI_PL_PL_ACT_STATUS_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80260B38 - the player's per-frame act status step (charge, stagger, condition checks) */
void pl_act_status_step(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STATUS_STEP_H */
