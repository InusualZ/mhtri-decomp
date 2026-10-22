/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s `lb_box_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_BOX_SCREEN_STEP_H
#define MHTRI_LOBBY_LB_BOX_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802213AC - the item-box screen step (eft052_box_open), screen 8 */
void lb_box_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_BOX_SCREEN_STEP_H */
