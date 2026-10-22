/* Leaf header (docs/plan.md 6.5 rule 2): `pl_gauge_level_get` and the other `hud/pl_frame_sync.cpp` player checks the
 * cockpit calls, defined by `hud/pl_frame_sync.cpp`. */
#ifndef MHTRI_HUD_PL_GAUGE_LEVEL_GET_H
#define MHTRI_HUD_PL_GAUGE_LEVEL_GET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803311A0: the player's charge gauge level byte. */
u8 pl_gauge_level_get(_PLW* plw);
/* 0x8033132C: whether the player's charge gauge flashes. */
u32 pl_charge_flash_ck(_PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_GAUGE_LEVEL_GET_H */
