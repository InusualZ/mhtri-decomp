/* Leaf header (docs/plan.md 6.5 rule 2): symbols `camera/camera_main.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_CAMERA_CAMERA_MODE1_CK_PLAYER_H
#define MHTRI_CAMERA_CAMERA_MODE1_CK_PLAYER_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B9740: the aim and zoom state of the player's camera. */
void camera_aim_zoom_get(_PLW* plw, u8* aiming, u8* zooming, f32* zoom);
/* 0x802BED20: whether player `player`'s camera is in mode 1. */
u32 camera_mode1_ck_player(u8 player);
/* 0x802BED74: `camera_aim_zoom_get` for player `player`'s camera. */
void camera_aim_zoom_get_player(_PLW* plw, u8* aiming, u8* zooming, f32* zoom, u8 player);
/* 0x802BEDE8: whether player `player`'s camera work is busy. */
u32 camera_work_ck_player(u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_MODE1_CK_PLAYER_H */
