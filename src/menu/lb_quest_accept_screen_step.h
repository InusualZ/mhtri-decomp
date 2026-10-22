/* Leaf header (docs/plan.md 6.5 rule 2): `menu/fn_8031EA8C.cpp`'s `lb_quest_accept_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_QUEST_ACCEPT_SCREEN_STEP_H
#define MHTRI_MENU_LB_QUEST_ACCEPT_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80321D98 - the quest accept screen step (quest_board_accept), screen 12 */
void lb_quest_accept_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_QUEST_ACCEPT_SCREEN_STEP_H */
