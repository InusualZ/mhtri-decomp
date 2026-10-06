/* Leaf header (docs/plan.md 6.5 rule 2): the plain-C `hud/layout.cpp` draw entries a consumer that cannot include
 * `hud/layout.h` calls, on the types of `hud/layout_types.h`. */
#ifndef MHTRI_HUD_SPRITE_FRAME_APPLY_H
#define MHTRI_HUD_SPRITE_FRAME_APPLY_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E0AD4 - fills `rec` with frame `part` of sprite `id` and its position (plain C, like the map row). */
u32 sprite_frame_apply(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out);
/* 0x802E0DA8 - draws frame `part` of `rec` from the record's own animation table (GUESS name). */
u32 spr_anim_draw(_SPR_DATA_* rec, u16 part, const _mh_ivec2_* pos);
/* 0x802E23D0 - draws `text` with the font of frame `part` of sprite `id` (GUESS name). */
void draw_font_anim_idx(u32 id, u32 part, s8* text, u8 flag, const _mh_ivec2_* pos);
/* 0x802E4798 - draws window frame `kind` over the rectangle at (`x`, `y`) of `w` by `h` (GUESS name). */
void draw_window_frame_style(s16 x, s16 y, u16 w, u16 h, u8 kind);
/* 0x802E1E4C - draws `number` with the digit sprites `ids` at frame `part`. */
void draw_number_anim_ary(const u16* ids, u16 part, s32 number, u32 color, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_SPRITE_FRAME_APPLY_H */
