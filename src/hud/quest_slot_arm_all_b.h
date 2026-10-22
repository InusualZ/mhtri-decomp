/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit_quest.cpp` symbol `Pl/pl_act_step.cpp` calls.  The owner's
 * full header defines `_mh_ivec2_`, which collides with the player band's includes. */
#ifndef MHTRI_HUD_QUEST_SLOT_ARM_ALL_B_H
#define MHTRI_HUD_QUEST_SLOT_ARM_ALL_B_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802EA138 - re-arms every quest slot of chunk `value`. */
s32 quest_slot_arm_all_b(s8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_SLOT_ARM_ALL_B_H */
