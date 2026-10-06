/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit_quest.cpp` symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_HUD_COCKPIT_QUEST_INIT_H
#define MHTRI_HUD_COCKPIT_QUEST_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4AD4 / 0x802EF340 - set the cockpit up for a hunt, and push a kill onto its hunt log (monster, its
 * count, kill/capture kind).  GUESS names. */
void cockpit_quest_init(void);
void cockpit_hunt_log_push(u8 monster, u16 count, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_QUEST_INIT_H */
