/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_npc_entries_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_NPC_ENTRIES_UPDATE_H
#define MHTRI_HUD_COCKPIT_NPC_ENTRIES_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E53C4 (0x3C): updates the cockpit's NPC entries. */
void cockpit_npc_entries_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_NPC_ENTRIES_UPDATE_H */
