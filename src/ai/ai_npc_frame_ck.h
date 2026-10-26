/* ai/ai_npc_frame_ck.h - leaf header: `ai_npc_frame_ck` (0x802D2990), owned by `ai/ai_npc.cpp`. C linkage. */
#ifndef MHTRI_AI_AI_NPC_FRAME_CK_H
#define MHTRI_AI_AI_NPC_FRAME_CK_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Returns non-zero when the NPC's motion frame crosses `low` (`flag` selects the test). */
u32 ai_npc_frame_ck(struct _AINPC_W* self, u32 flag, f32 low, f32 high);
#ifdef __cplusplus
}
#endif

#endif
