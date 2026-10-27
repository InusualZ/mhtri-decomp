/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ai_npc_pair_values_get`, which `lobby/lb_companion_ui.cpp`
 * calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AI_NPC_PAIR_VALUES_GET_H
#define MHTRI_AI_AI_NPC_PAIR_VALUES_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7754 (0x4C): the four tuning values of the pair (`first`, `second`) into `out`.  GUESS name. */
void ai_npc_pair_values_get(u8 first, u8 second, u16* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_PAIR_VALUES_GET_H */
