/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft016.cpp`'s `eft_npc_aura_create`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_EF_EFT_NPC_AURA_CREATE_H
#define MHTRI_EF_EFT_NPC_AURA_CREATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010C680 (0x90): binds a kind-5 effect slot to the owner */
void eft_npc_aura_create(struct _AINPC_W* owner);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_NPC_AURA_CREATE_H */
