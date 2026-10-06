/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/multi_result.cpp` NPC swap rows `lobby/lb_quest_screen.cpp`'s talk
 * step calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_NPC_TRADE_PICK_H
#define MHTRI_MENU_NPC_TRADE_PICK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A36B4 - picks the swap the quest NPC offers on this map: 0 ready, 1 no room, 2 nothing to swap, 3/4 a special
 * swap, 0xFF no swaps here (GUESS name). */
s32 npc_trade_pick(struct _PLW* me);

/* 0x803A3904 - rolls the NPC's gift for this map kind and reports whether the player has room for it (GUESS name). */
u32 npc_gift_roll(struct _PLW* me);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_NPC_TRADE_PICK_H */
