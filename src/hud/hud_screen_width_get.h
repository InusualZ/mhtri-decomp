/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit`'s `hud_screen_width_get`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_HUD_HUD_SCREEN_WIDTH_GET_H
#define MHTRI_HUD_HUD_SCREEN_WIDTH_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D9F70 (hud/cockpit): the HUD screen width scale. */
f32 hud_screen_width_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_HUD_SCREEN_WIDTH_GET_H */
