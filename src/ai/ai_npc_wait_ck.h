/* Leaf header (docs/plan.md 6.5 rule 2): symbols `ai/ai_npc.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_AI_AI_NPC_WAIT_CK_H
#define MHTRI_AI_AI_NPC_WAIT_CK_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7DF8: whether the AI NPC is active and its +0x374 timer runs. */
u32 ai_npc_wait_ck(struct _AINPC_W* npc);
/* 0x802D7E68: whether the AI NPC is active in motion step 5/2. */
u32 ai_npc_motion5_ck(struct _AINPC_W* npc);
/* 0x802D948C: whether the AI NPC's +0x483 mode is 1. */
u32 ai_npc_mode1_ck(struct _AINPC_W* npc);
/* 0x802D9A50: whether the AI NPC is active and its +0x46C call flag is 1. */
u32 ai_npc_call_ck(struct _AINPC_W* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_WAIT_CK_H */
