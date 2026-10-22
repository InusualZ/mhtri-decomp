/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_MSG_SLOT_CLEAR_H
#define MHTRI_HUD_COCKPIT_MSG_SLOT_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DE550 - clears the 0x130-byte cockpit message record of player idx (two u16 and a byte); menu_item_work_init calls it for both players */
void cockpit_msg_slot_clear(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_MSG_SLOT_CLEAR_H */
