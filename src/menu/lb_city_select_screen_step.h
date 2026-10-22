/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_placeinfo.cpp`'s `lb_city_select_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_CITY_SELECT_SCREEN_STEP_H
#define MHTRI_MENU_LB_CITY_SELECT_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043744C - the city select screen step (NetCtrlWk::requestEnterCity), screens 28/33/37 */
void lb_city_select_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_CITY_SELECT_SCREEN_STEP_H */
