/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbol `Pl/pl_act_step.cpp` calls.  `ai/ai_npc.h` defines
 * its own `_HIT_W`, which `Pl/hit_w.h` (the hit registry's shared record) also defines, so a unit that needs the
 * hit registry cannot include the owner's full header. */
#ifndef MHTRI_AI_AI_NPC_PLAYER_IN_REACH_CK_H
#define MHTRI_AI_AI_NPC_PLAYER_IN_REACH_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D78FC - whether the player is inside the AI NPC's current attack reach; C linkage (the map row is
 * unmangled). */
s32 ai_npc_player_in_reach_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_PLAYER_IN_REACH_CK_H */
