/* Leaf header (docs/plan.md 6.5 rule 2) for `draw_font` (0x802E22E0) and `put_button_icon` (0x802E0F5C), the
 * `hud/layout.cpp` text and button-icon draws.  A leaf because `hud/layout.h` declares `draw_font_anim_idx`
 * differently from `hud/sprite_frame_apply.h`, so the full header cannot meet that one in a consumer. */
#ifndef MHTRI_HUD_DRAW_FONT_H
#define MHTRI_HUD_DRAW_FONT_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
/* 0x802E22E0 - draws `text` with the layout record `spr` at `pos`. */
void draw_font(const _SPR_DATA_& spr, s8* text, u32 flag, const _mh_ivec2_* pos);
/* 0x802E0F5C - draws button icon `tex_idx` (wide variant `wide_idx`) at (`x`, `y`). */
void put_button_icon(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx);
#endif

#endif /* MHTRI_HUD_DRAW_FONT_H */
