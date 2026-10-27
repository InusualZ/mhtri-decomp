/*
 * Declarations for the symbols `src/pad_connect.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 * `word_read`, `pad_hold_set`/`pad_hold_clear`, `pad_hold_ck`, `screen_split_view_get`, `player_pad_channel_get` and
 * `scene_camera_get` are declared by their leaf headers (`src/<name>.h`), and `word_copy_return_dst`
 * and `setSoftresetFlag` by `mh3_pad.h`.
 */
#ifndef MHTRI_PAD_CONNECT_H
#define MHTRI_PAD_CONNECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x806694D0 - the game's OSMutex (0x18 B), initialised by `game_system_init`; the network heap's
 * allocators lock it around every MEM call. */
extern u8 game_mutex[0x18];

/* 0x80046C80 - initialises the game systems at boot (`main.cpp` calls it once). */
void game_system_init(void);
/* 0x80046D34 - runs one frame of the main loop; returns 1. */
s32 game_frame_run(void);
/* 0x80046E34 - loads the 640x480 orthographic projection and an identity position matrix. */
void screen_ortho_set(void);
/* 0x80046E98 - resets the G3D state while `system_w` requests it. */
void g3d_reset_on_request(void);
/* 0x80046F28 - clears the stage and hook pointers and calls the face and menu entry points with neutral arguments. */
void game_init_hooks_clear(void);

/* 0x80047058 - non-zero while the screen is split between two players. */
s32 screen_split_mode_ck(void);
/* 0x80047020 - resets the split-screen state (single screen, no view, camera banks 8/16, 4:3 aspect). */
void pad_connect_reset(void);
/* 0x80047074 - turns the split screen on (1) or off. */
void pad_connect_disp_set(u32 on);
/* 0x800470DC - copies split viewport `view` of player `bank` into that bank's six scene cameras. */
void split_camera_set(u8 bank, u8 view);
/* 0x8004726C - copies the word at `src` into `dst`. */
void word_copy(s32* dst, s32* src);
/* 0x80047278 - selects split view `view` and lays both players' cameras out for it. */
void screen_split_view_set(u8 view);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PAD_CONNECT_H */
