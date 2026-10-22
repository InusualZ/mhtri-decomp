/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit_quest` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_QUEST_MARK_TOGGLE_H
#define MHTRI_HUD_QUEST_MARK_TOGGLE_H

#include "types.h"

struct CockpitWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E8E34 - toggles the quest mark gate of a cockpit work record */
u8 quest_mark_toggle(CockpitWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_MARK_TOGGLE_H */
