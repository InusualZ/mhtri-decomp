/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_MSG_SLOT_DRAW_H
#define MHTRI_HUD_COCKPIT_MSG_SLOT_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DF094 - the cockpit message record's draw task, registered with subTransSetPrio in place of the menu draw while a message shows */
void cockpit_msg_slot_draw(u32 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_MSG_SLOT_DRAW_H */
