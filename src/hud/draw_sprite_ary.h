/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/layout.cpp` draw entries a consumer that cannot include
 * `hud/layout.h` (its `pl.h` meets another `MHchar` view) calls, on the types of `hud/layout_types.h`: the mangled
 * rows here, the plain-C ones in `hud/sprite_frame_apply.h`. */
#ifndef MHTRI_HUD_DRAW_SPRITE_ARY_H
#define MHTRI_HUD_DRAW_SPRITE_ARY_H

#include "types.h"
#include "hud/layout_types.h"
#include "hud/sprite_frame_apply.h"   /* the plain-C draw entries */

#ifdef __cplusplus
/* The mangled map rows are these declarations' C++ spellings (rule 9). */
void draw_sprite(const _SPR_DATA_& spr, const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void draw_sprite_ary(const u16* ids, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 anim, const _mh_ivec2_* pos);
u32 draw_sprite_anim_ary(const u16* ids, u16 anim, const _mh_ivec2_* pos);
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos);
void draw_number_idx(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos);
#endif

#endif /* MHTRI_HUD_DRAW_SPRITE_ARY_H */
