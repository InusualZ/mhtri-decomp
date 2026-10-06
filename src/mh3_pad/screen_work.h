/*
 * `Screen_w` - the screen geometry/calibration block (.bss 0x8065903C, 0x54 B), defined by `src/mh3_pad.cpp`
 * (its `.bss` 0x806585B8-0x806694E8 is that unit's own).  `main.cpp` fills it in `render_mode_copy`'s video setup
 * and reads it through its screen accessors; the band units read single fields (`frame_scale`, `aspect`).
 * This is the type's one home (rule 1); the object is declared in `Screen_w.h`.
 *
 * Field names: `width`/`height` (640/448), `width_f`/`height_f` (the float pair `get_ScreenSize` returns),
 * `aspect` (`arena_camera_init` hands it to `Camera::SetPerspective`), `frame_scale` (60.0f / `frame_divisor`;
 * the clear-time formatters and the wait-time scalers multiply/divide by it).  GUESS: `margin_*`/`inner_*`
 * (the two rectangles `fn_80040360` copies out) and `visible_*` (the u16 pair `fn_8004028C` points at).
 */
#ifndef MHTRI_MH3_PAD_SCREEN_WORK_H
#define MHTRI_MH3_PAD_SCREEN_WORK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ScreenWork {
    /* +0x00 */ u16 width;
    /* +0x02 */ u16 height;
    /* +0x04 */ f32 width_f;
    /* +0x08 */ f32 height_f;
    /* +0x0C */ f32 aspect;
    /* +0x10 */ u32 frame_divisor;
    /* +0x14 */ f32 frame_scale;
    /* +0x18 */ u8 flag_0x18;       /* `fn_800403DC` reports it */
    /* +0x19 */ u8 wide_mode;       /* `ck_WideMode` */
    /* +0x1A */ u8 split_mode;      /* `screen_split_mode_ck`; selects `player_aspect` */
    /* +0x1B */ u8 split_view_no;   /* `mh3_pad.cpp` returns it as an s8 while `split_mode` is set */
    /* +0x1C */ u8 pad_0x1c[0x10];
    /* +0x2C */ f32 margin_x;
    /* +0x30 */ f32 margin_y;
    /* +0x34 */ f32 inner_width;    /* width_f - margin_x */
    /* +0x38 */ f32 inner_height;   /* height_f - margin_y */
    /* +0x3C */ u16 visible_width;
    /* +0x3E */ u16 visible_height;
    /* +0x40 */ u8 pad_0x40[8];
    /* +0x48 */ f32 player_aspect[3];
} ScreenWork; /* size: 0x54 */


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_SCREEN_WORK_H */
