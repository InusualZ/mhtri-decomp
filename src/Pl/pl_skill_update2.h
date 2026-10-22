/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_skill_update2`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_SKILL_UPDATE2_H
#define MHTRI_PL_PL_SKILL_UPDATE2_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802789EC - the second skill update (Pl_cat_skill_ck, fn_80278D1C) */
void pl_skill_update2(struct _PLW* self, s32 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SKILL_UPDATE2_H */
