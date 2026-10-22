/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `ai/ai_npc.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AINPC_DEMO_START_H
#define MHTRI_AI_AINPC_DEMO_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7CE4 - starts the AI NPC's demo action for demo `demo`.  GUESS name. */
void ainpc_demo_start(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_DEMO_START_H */
