/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_split_frame_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_SPLIT_FRAME_UPDATE_H
#define MHTRI_HUD_COCKPIT_SPLIT_FRAME_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802EF87C: the cockpit's split-screen frame update. */
void cockpit_split_frame_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_SPLIT_FRAME_UPDATE_H */
