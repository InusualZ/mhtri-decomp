/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_daily_menu_roll`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_DAILY_MENU_ROLL_H
#define MHTRI_LOBBY_LB_DAILY_MENU_ROLL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E9640 - rolls the lobby's daily rows from the unlock conditions and ran_suu, given the hunter name */
void lb_daily_menu_roll(u8* name);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_DAILY_MENU_ROLL_H */
