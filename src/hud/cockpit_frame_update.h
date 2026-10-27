/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_frame_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_FRAME_UPDATE_H
#define MHTRI_HUD_COCKPIT_FRAME_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E5284 (0x140): the cockpit's single-screen frame update. */
void cockpit_frame_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_FRAME_UPDATE_H */
