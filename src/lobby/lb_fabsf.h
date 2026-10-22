/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_scratch.cpp`'s `lb_fabsf`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_LOBBY_LB_FABSF_H
#define MHTRI_LOBBY_LB_FABSF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021E300 (0x4): the absolute value of a float (one fabs) */
f32 lb_fabsf(f32 v);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FABSF_H */
