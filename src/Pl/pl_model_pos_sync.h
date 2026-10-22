/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_model_pos_sync`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_MODEL_POS_SYNC_H
#define MHTRI_PL_PL_MODEL_POS_SYNC_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026FD0C - copies the player's position into the model and updates it (fn_800E09D0) */
u32 pl_model_pos_sync(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MODEL_POS_SYNC_H */
