/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/fn_80219260.cpp` row the game-mode flow calls.  C linkage. */
#ifndef MHTRI_LOBBY_LB_MENU_SCRATCH_INIT_H
#define MHTRI_LOBBY_LB_MENU_SCRATCH_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021DB30 - clears the lobby menu's scratch slots (and its +0x14C id when `full` is set) (GUESS name). */
void lb_menu_scratch_init(s16 full);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_SCRATCH_INIT_H */
