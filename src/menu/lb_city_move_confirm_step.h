/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_placeinfo.cpp`'s `lb_city_move_confirm_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_CITY_MOVE_CONFIRM_STEP_H
#define MHTRI_MENU_LB_CITY_MOVE_CONFIRM_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043A3E0 - the city move confirm step (lb_yes_no_step, lobby_player_place), screen 32 */
void lb_city_move_confirm_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_CITY_MOVE_CONFIRM_STEP_H */
