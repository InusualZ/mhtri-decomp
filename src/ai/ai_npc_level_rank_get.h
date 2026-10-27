/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ai_npc_level_rank_get`, which `lobby/lb_companion_ui.cpp`
 * calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AI_NPC_LEVEL_RANK_GET_H
#define MHTRI_AI_AI_NPC_LEVEL_RANK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D77A0 (0x3C): which of the three ranks the level value `value` maps to.  GUESS name. */
s32 ai_npc_level_rank_get(u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_LEVEL_RANK_GET_H */
