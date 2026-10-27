/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbol `Pl/pl_act_step.cpp` calls.  `ai/ai_npc.h` defines
 * its own view of `_AINPC_W`, which `ai/ainpc.h` also defines, so a unit that reads `ai/ainpc.h` cannot include it. */
#ifndef MHTRI_AI_AINPC_AREA_BUFF_APPLY_H
#define MHTRI_AI_AINPC_AREA_BUFF_APPLY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D9A78 - applies the party item effect `kind` to the AI companion; C linkage (the map row is unmangled). */
void ainpc_area_buff_apply(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_AREA_BUFF_APPLY_H */
