/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s `cockpit_notice_hold`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_COCKPIT_NOTICE_HOLD_H
#define MHTRI_HUD_COCKPIT_NOTICE_HOLD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E5400 (0x14): holds the cockpit's notices. */
void cockpit_notice_hold(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_NOTICE_HOLD_H */
