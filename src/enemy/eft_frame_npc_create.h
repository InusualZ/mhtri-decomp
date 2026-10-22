/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_act_step_tail.cpp`'s `eft_frame_npc_create`, the one declaration
 * `ai/ai_npc.cpp` needs. */
#ifndef MHTRI_ENEMY_EFT_FRAME_NPC_CREATE_H
#define MHTRI_ENEMY_EFT_FRAME_NPC_CREATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80330284 (0xF0): pools a one-model frame effect for an NPC source, mode 2. */
void eft_frame_npc_create(struct _AINPC_W* source, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EFT_FRAME_NPC_CREATE_H */
