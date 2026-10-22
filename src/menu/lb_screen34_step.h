/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s `lb_screen34_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_SCREEN34_STEP_H
#define MHTRI_MENU_LB_SCREEN34_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BF948 - the lobby screen step for screen 34 */
void lb_screen34_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_SCREEN34_STEP_H */
