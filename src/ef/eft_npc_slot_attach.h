/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft_model_slot.cpp`'s `eft_npc_slot_attach`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_EF_EFT_NPC_SLOT_ATTACH_H
#define MHTRI_EF_EFT_NPC_SLOT_ATTACH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800FAD40 (0x50): takes a kind-2 effect slot and binds the owner and its update callback */
void eft_npc_slot_attach(struct _AINPC_W* owner);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_NPC_SLOT_ATTACH_H */
