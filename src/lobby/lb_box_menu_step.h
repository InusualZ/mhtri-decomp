/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_pane_ui.cpp`'s `lb_box_menu_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  The map's name. */
#ifndef MHTRI_LOBBY_LB_BOX_MENU_STEP_H
#define MHTRI_LOBBY_LB_BOX_MENU_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801F4C14 - the lobby screen step for screen 6 */
void lb_box_menu_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_BOX_MENU_STEP_H */
