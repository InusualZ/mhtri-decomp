/* ai/ai_npc_time_scale_get.h - leaf header: `ai_npc_time_scale_get` (0x802D29DC), owned by `ai/ai_npc.cpp`; C linkage. */
#ifndef MHTRI_AI_AI_NPC_TIME_SCALE_GET_H
#define MHTRI_AI_AI_NPC_TIME_SCALE_GET_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the NPC's +0x64 time scale. */
f32 ai_npc_time_scale_get(struct _AINPC_W* self);

#ifdef __cplusplus
}
#endif

#endif
