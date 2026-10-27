/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ai_npc_status_preview`, which `lobby/lb_companion_ui.cpp`
 * calls (the records are the owner's `ai/ai_npc.h`).  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AI_NPC_STATUS_PREVIEW_H
#define MHTRI_AI_AI_NPC_STATUS_PREVIEW_H

#include "types.h"

struct AINPCSetup;
struct AINPCStatus;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7F58 (0x4BC): fills the status preview `status` of the companion setup `setup`. */
void ai_npc_status_preview(struct AINPCSetup* setup, struct AINPCStatus* status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_STATUS_PREVIEW_H */
