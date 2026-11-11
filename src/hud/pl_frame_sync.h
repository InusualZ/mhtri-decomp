/* hud/pl_frame_sync.h - the step tables `hud/pl_frame_sync.cpp` defines for weapon kind 8 (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_HUD_PL_FRAME_SYNC_H
#define MHTRI_HUD_PL_FRAME_SYNC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805E2074 / 0x805E2458: the attack-range step tables for weapon kind 8 (default, and kind_0x09 == 3) */
extern u16 pl_steps_kind8[0xE7];
extern u16 pl_steps_kind8_slot3[0xBB];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_FRAME_SYNC_H */
