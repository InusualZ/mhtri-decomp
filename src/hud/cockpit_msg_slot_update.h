/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_MSG_SLOT_UPDATE_H
#define MHTRI_HUD_COCKPIT_MSG_SLOT_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DEC18 - player idx's cockpit message record frame: counts its timer down while idle, else steps it; the menu frames skip while it reports busy */
s32 cockpit_msg_slot_update(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_MSG_SLOT_UPDATE_H */
