/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_model_visible_set`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_MODEL_VISIBLE_SET_H
#define MHTRI_PL_PL_MODEL_VISIBLE_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026A2D0 - stores `value` in the player model's visibility byte (rig model +0xF1) */
void pl_model_visible_set(struct _PLW* self, u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MODEL_VISIBLE_SET_H */
