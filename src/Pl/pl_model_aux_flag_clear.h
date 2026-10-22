/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_model_aux_flag_clear`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_MODEL_AUX_FLAG_CLEAR_H
#define MHTRI_PL_PL_MODEL_AUX_FLAG_CLEAR_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026A2F8 - clears the player model's +0xF2 byte (rig +0xF6) */
void pl_model_aux_flag_clear(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MODEL_AUX_FLAG_CLEAR_H */
