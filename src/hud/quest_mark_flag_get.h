/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit_quest` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_QUEST_MARK_FLAG_GET_H
#define MHTRI_HUD_QUEST_MARK_FLAG_GET_H

#include "types.h"

struct CockpitWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E8E5C - the quest mark's shown flag of a cockpit work record */
u8 quest_mark_flag_get(CockpitWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_MARK_FLAG_GET_H */
