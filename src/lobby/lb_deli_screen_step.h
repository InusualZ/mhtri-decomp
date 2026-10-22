/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_deli_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_DELI_SCREEN_STEP_H
#define MHTRI_LOBBY_LB_DELI_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E79E4 - the delivery-list screen step (lb_deli_list_build, lb_deli_menu_step), screen 4 */
void lb_deli_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_DELI_SCREEN_STEP_H */
