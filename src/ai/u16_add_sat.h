/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `u16_add_sat`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_AI_U16_ADD_SAT_H
#define MHTRI_AI_U16_ADD_SAT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D9DE8 - adds `value` to `*dst`, saturating at 0xFFFF */
void u16_add_sat(u16* dst, u16 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_U16_ADD_SAT_H */
