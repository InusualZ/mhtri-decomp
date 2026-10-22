/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_MSG_SLOT_SPLIT_DRAW_H
#define MHTRI_HUD_COCKPIT_MSG_SLOT_SPLIT_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DF40C - the split-screen form of the cockpit message draw task */
void cockpit_msg_slot_split_draw(u32 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_MSG_SLOT_SPLIT_DRAW_H */
