/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_skill_update`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_SKILL_UPDATE_H
#define MHTRI_PL_PL_SKILL_UPDATE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027885C - updates the player's skill effects (Pl_cat_skill_ck) */
void pl_skill_update(struct _PLW* self, s32 a, s32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SKILL_UPDATE_H */
