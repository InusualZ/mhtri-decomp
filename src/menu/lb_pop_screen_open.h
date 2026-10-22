/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s `lb_pop_screen_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_POP_SCREEN_OPEN_H
#define MHTRI_MENU_LB_POP_SCREEN_OPEN_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BEF0C - clears the lobby screen work and opens the pop-up screen for player `self` */
void lb_pop_screen_open(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_POP_SCREEN_OPEN_H */
