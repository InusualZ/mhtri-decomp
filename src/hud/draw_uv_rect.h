/* Leaf header (docs/plan.md 6.5 rule 2): `hud/layout.cpp`'s `draw_uv_rect`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HUD_DRAW_UV_RECT_H
#define MHTRI_HUD_DRAW_UV_RECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E0F78 (0xAC): draws a rectangle filled with the fixed uv pair */
void draw_uv_rect(s16 x, s16 y, s16 w, s16 h, u32 color, u8 wide_idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_UV_RECT_H */
