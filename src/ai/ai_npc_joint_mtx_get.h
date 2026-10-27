/* `ai_npc_joint_mtx_get`, defined by `ai/ai_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_AI_AI_NPC_JOINT_MTX_GET_H
#define MHTRI_AI_AI_NPC_JOINT_MTX_GET_H

#include "types.h"
#include "nw4r/math.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif

/* Reads joint `joint`'s world matrix of the NPC's embedded model into `out`. */
void ai_npc_joint_mtx_get(struct _AINPC_W* self, u32 joint, nw4r::math::MTX34* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_JOINT_MTX_GET_H */
