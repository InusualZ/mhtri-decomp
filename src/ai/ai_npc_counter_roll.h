/* ai/ai_npc_counter_roll.h - leaf header: `ai_npc_counter_roll` (0x802D3E4C), owned by `ai/ai_npc.cpp`. C linkage. */
#ifndef MHTRI_AI_AI_NPC_COUNTER_ROLL_H
#define MHTRI_AI_AI_NPC_COUNTER_ROLL_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Rolls the AI NPC's counter-attack chance against the attack it took (10 %, 40 % with skill 0x10); 1, with the latch
 * set, when it hit. */
s32 ai_npc_counter_roll(struct _AINPC_W* self);
#ifdef __cplusplus
}
#endif

#endif
