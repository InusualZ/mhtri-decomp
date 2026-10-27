/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_event_notice_spawn`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_EVENT_NOTICE_SPAWN_H
#define MHTRI_HUD_COCKPIT_EVENT_NOTICE_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E5414 (0x94): spawns the cockpit's end-of-quest notice for `kind`. */
void cockpit_event_notice_spawn(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_EVENT_NOTICE_SPAWN_H */
