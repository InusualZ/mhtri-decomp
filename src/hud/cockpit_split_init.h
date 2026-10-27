/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_split_init`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_SPLIT_INIT_H
#define MHTRI_HUD_COCKPIT_SPLIT_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4B8C (0x98): sets the cockpit up for the split screen. */
void cockpit_split_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_SPLIT_INIT_H */
