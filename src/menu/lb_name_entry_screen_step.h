/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s `lb_name_entry_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_NAME_ENTRY_SCREEN_STEP_H
#define MHTRI_MENU_LB_NAME_ENTRY_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BF294 - the keyboard name-entry screen step (kbd_open), screen 31 */
void lb_name_entry_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_NAME_ENTRY_SCREEN_STEP_H */
