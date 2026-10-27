/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbol `Pl/pl_act_step.cpp`'s call-the-companion act
 * tests.  `ai/ai_npc.h` defines its own view of `_AINPC_W`, which `ai/ainpc.h` also defines. */
#ifndef MHTRI_AI_AI_NPC_BESIDE_PLAYER_CK_H
#define MHTRI_AI_AI_NPC_BESIDE_PLAYER_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D79A4 - whether the AI companion stands beside the player. */
u32 ai_npc_beside_player_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_BESIDE_PLAYER_CK_H */
