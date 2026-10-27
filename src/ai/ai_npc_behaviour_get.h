/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ai_npc_behaviour_get`, which `lobby/lb_companion_ui.cpp`
 * calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AI_NPC_BEHAVIOUR_GET_H
#define MHTRI_AI_AI_NPC_BEHAVIOUR_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D773C (0x18): the behaviour record of entry `index` (+0x04: the skill slots per level).  GUESS name. */
const u8* ai_npc_behaviour_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_BEHAVIOUR_GET_H */
