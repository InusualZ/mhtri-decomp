/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit_quest` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_QUEST_SLOT_ARM_ALL_A_H
#define MHTRI_HUD_QUEST_SLOT_ARM_ALL_A_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802EA050 - arms every live player's quest slot with a value */
s32 quest_slot_arm_all_a(s8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_SLOT_ARM_ALL_A_H */
