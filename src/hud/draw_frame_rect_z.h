/* `draw_frame_rect_z`, defined by `hud/cockpit.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_DRAW_FRAME_RECT_Z_H
#define MHTRI_HUD_DRAW_FRAME_RECT_Z_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws a rectangle frame of size `w` x `h` at `x`, `y`, depth `z` in colour `color`. */
void draw_frame_rect_z(s16 x, s16 y, s16 w, s16 h, u8 alpha, u32 color, f32 z);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_FRAME_RECT_Z_H */
