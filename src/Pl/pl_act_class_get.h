/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_class_get`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_PL_PL_ACT_CLASS_GET_H
#define MHTRI_PL_PL_ACT_CLASS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80274624 (0x124): the 1-based class of the player's current act in the melee/ranged act tables (0 when none) */
s32 pl_act_class_get(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_CLASS_GET_H */
