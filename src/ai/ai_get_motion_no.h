/* ai/ai_get_motion_no.h - leaf header: `ai_get_motion_no` (0x802D29B0), owned by `ai/ai_npc.cpp`. C++ linkage: the map
 * carries `ai_get_motion_no__FP8_AINPC_W`. */
#ifndef MHTRI_AI_AI_GET_MOTION_NO_H
#define MHTRI_AI_AI_GET_MOTION_NO_H

#include "types.h"

struct _AINPC_W;

/* Returns the NPC's current motion number. */
u16 ai_get_motion_no(struct _AINPC_W* self);

#endif
