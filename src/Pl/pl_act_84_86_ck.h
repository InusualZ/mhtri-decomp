/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_84_86_ck`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_PL_PL_ACT_84_86_CK_H
#define MHTRI_PL_PL_ACT_84_86_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D874 (0x2C): whether the player's act is 84..86 in act group 0 */
u32 pl_act_84_86_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_84_86_CK_H */
