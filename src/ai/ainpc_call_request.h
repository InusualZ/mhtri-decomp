/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbols `Pl/pl_act_step.cpp`'s companion acts call.
 * `ai/ai_npc.h` defines its own view of `_AINPC_W`, which `ai/ainpc.h` also defines, so a unit that reads `ai/ainpc.h`
 * cannot include it. */
#ifndef MHTRI_AI_AINPC_CALL_REQUEST_H
#define MHTRI_AI_AINPC_CALL_REQUEST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7A50 - raises the active AI companion's call latch (registry name). */
void ainpc_call_request(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_CALL_REQUEST_H */
