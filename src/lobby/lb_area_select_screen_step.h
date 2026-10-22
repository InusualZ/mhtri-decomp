/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp`'s `lb_area_select_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_AREA_SELECT_SCREEN_STEP_H
#define MHTRI_LOBBY_LB_AREA_SELECT_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033BB78 - the area select screen step (lb_area_name_apply), screen 14 */
void lb_area_select_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AREA_SELECT_SCREEN_STEP_H */
