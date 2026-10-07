/* ai/ai_npc_motion_step_ck.h - leaf header: `ai_npc_motion_step_ck` (0x802D2B38), owned by `ai/ai_npc.cpp`. */
#ifndef MHTRI_AI_AI_NPC_MOTION_STEP_CK_H
#define MHTRI_AI_AI_NPC_MOTION_STEP_CK_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Returns 1 when the NPC's motion byte and step half-word equal the pair. */
s32 ai_npc_motion_step_ck(struct _AINPC_W* npc, u8 motion, u16 step);
#ifdef __cplusplus
}
#endif

#endif
