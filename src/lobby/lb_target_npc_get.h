/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_scratch.cpp`'s `lb_target_npc_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_TARGET_NPC_GET_H
#define MHTRI_LOBBY_LB_TARGET_NPC_GET_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021EBF0 - returns the menu scratch's target NPC */
struct _LB_NPC* lb_target_npc_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TARGET_NPC_GET_H */
