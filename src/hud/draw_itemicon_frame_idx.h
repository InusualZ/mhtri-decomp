/* Leaf header (docs/plan.md 6.5 rule 2): symbols `hud/layout.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_HUD_DRAW_ITEMICON_FRAME_IDX_H
#define MHTRI_HUD_DRAW_ITEMICON_FRAME_IDX_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E12A0: draws item icon cell `tex_idx` in `color` over frame `part` of sprite `id`. */
u32 draw_itemicon_frame_idx(u16 id, u16 part, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
/* 0x802E16E0: draws sprite `id` with texture cell `tex_idx` in `color`. */
void draw_sprite_cell_color_idx(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
/* 0x802E3AAC: draws a prompt frame `width` wide at (`x`, `y`). */
void draw_prompt_frame(s16 x, s16 y, u16 width, u8 mode, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_ITEMICON_FRAME_IDX_H */
