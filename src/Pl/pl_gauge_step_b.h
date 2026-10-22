/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_gauge_step_b`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_GAUGE_STEP_B_H
#define MHTRI_PL_PL_GAUGE_STEP_B_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80270728 - the second gauge step of the player's frame (fn_80273ED8, fn_802703F4) */
void pl_gauge_step_b(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GAUGE_STEP_B_H */
