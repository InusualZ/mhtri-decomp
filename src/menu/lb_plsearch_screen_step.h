/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_plsearch.cpp`'s `lb_plsearch_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_PLSEARCH_SCREEN_STEP_H
#define MHTRI_MENU_LB_PLSEARCH_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044F724 - the player search screen step (menu_friend_list_step), screen 35 */
void lb_plsearch_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_PLSEARCH_SCREEN_STEP_H */
