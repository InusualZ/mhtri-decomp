/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ainpc_frame`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_AI_AINPC_FRAME_H
#define MHTRI_AI_AINPC_FRAME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C2900 (0x88): the AI NPC's per-frame step. */
void ainpc_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_FRAME_H */
