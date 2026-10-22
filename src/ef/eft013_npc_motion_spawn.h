/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft013_fx.cpp`'s `eft013_npc_motion_spawn`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_EF_EFT013_NPC_MOTION_SPAWN_H
#define MHTRI_EF_EFT013_NPC_MOTION_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80107860 (0x5C): spawns effect 9 for the owner and stamps its motion and motion step into the effect */
void eft013_npc_motion_spawn(struct _AINPC_W* owner, s32 mode, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT013_NPC_MOTION_SPAWN_H */
