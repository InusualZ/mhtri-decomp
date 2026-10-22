/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_item_get_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_ITEM_GET_SCREEN_STEP_H
#define MHTRI_LOBBY_LB_ITEM_GET_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801EAC84 - the item-get screen step (eft052_item_get_step), screen 5 */
void lb_item_get_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_GET_SCREEN_STEP_H */
