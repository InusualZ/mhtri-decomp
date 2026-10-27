/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft018.cpp`'s `eft_npc_model_set_create`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_EF_EFT_NPC_MODEL_SET_CREATE_H
#define MHTRI_EF_EFT_NPC_MODEL_SET_CREATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010D678 (0x10): builds the AI NPC's three-model effect set (eft_light_spawn with 3, 2, 1) */
void eft_npc_model_set_create(struct _AINPC_W* owner);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_NPC_MODEL_SET_CREATE_H */
