/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_MSG_SLOT_BUSY_CK_H
#define MHTRI_HUD_COCKPIT_MSG_SLOT_BUSY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DE670 - whether player idx's 0x130-byte cockpit message record has its +0x04 state byte set (neg/or/srwi) */
u32 cockpit_msg_slot_busy_ck(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_MSG_SLOT_BUSY_CK_H */
