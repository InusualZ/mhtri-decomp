/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_range2_ck`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_PL_PL_ACT_RANGE2_CK_H
#define MHTRI_PL_PL_ACT_RANGE2_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027DDC4 (0xC4): whether the master player's act is in the selected act range */
u32 pl_act_range2_ck(struct _PLW* plw, u8 selector);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_RANGE2_CK_H */
