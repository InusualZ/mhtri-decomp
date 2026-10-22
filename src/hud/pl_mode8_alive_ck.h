/* Leaf header (docs/plan.md 6.5 rule 2): `pl_mode8_alive_ck`, defined by `hud/pl_frame_sync.cpp`. */
#ifndef MHTRI_HUD_PL_MODE8_ALIVE_CK_H
#define MHTRI_HUD_PL_MODE8_ALIVE_CK_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033112C: whether the player is in act mode 8 and alive. */
u32 pl_mode8_alive_ck(_PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_MODE8_ALIVE_CK_H */
