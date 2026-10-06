/* The sprite-layout record types `hud/layout.cpp` owns (rule 1): `hud/layout.h` includes them, and a consumer that
 * cannot include that header (its `pl.h` meets another `MHchar` view) includes this one. */
#ifndef MHTRI_HUD_LAYOUT_TYPES_H
#define MHTRI_HUD_LAYOUT_TYPES_H

#include "types.h"

/* The 2D integer vector the HUD helpers exchange (`_mh_ivec2_` in the map's mangling).
 * size: 0x4 */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_;

/* One texture coordinate pair (`_mh_tex_uv_` in the map's mangling: the two halfwords
 * `fn_800526E4` copies and `drawshape_set_texture_rect` takes by pointer).
 * size: 0x4 */
typedef struct _mh_tex_uv_ {
    /* +0x00 */ s16 u;
    /* +0x02 */ s16 v;
} _mh_tex_uv_;

/* One 8-byte animation step.  The halfword at +0x00 packs the step's kind into its low 12 bits and
 * one flag into bit 15: when that bit is set the caller's frame index is first wrapped by the
 * table's own last key, which is what the cyclic steps need.  `count` is the number of keys
 * `frames` holds.  size: 0x8 */
typedef struct _SPR_ANIM_ {
    /* +0x00 */ u16 flags;
    /* +0x02 */ s16 count;
    /* +0x04 */ const u16* frames;
} _SPR_ANIM_;

/* `_SPR_ANIM_.flags`: the step kind, and the "wrap the frame at the last key" bit. */
#define SPR_ANIM_KIND_MASK 0x0FFF
#define SPR_ANIM_WRAP_LAST 0x8000

/* The sprite-data record every `draw_*` entry takes by reference - `get_lsp_data` hands one back
 * and `draw_sprite` blits it.  Every offset below is one the disassembly reads, and the 0x24-byte
 * size is the copy `spr_data_copy` makes field by field (its `lwz`/`sth`/`stb` sequence covers
 * 0x00..0x23).  size: 0x24 */
typedef struct _SPR_DATA_ {
    /* +0x00 */ _mh_ivec2_ pos;      /* the record's own anchor, added to the caller's position */
    /* +0x04 */ s16 ofs_x;           /* added to x after the float conversion */
    /* +0x06 */ s16 ofs_y;
    /* +0x08 */ s16 width;           /* the vertex rectangle */
    /* +0x0A */ s16 height;
    /* +0x0C */ u16 u_scale;         /* over 100.0f for the texture rectangle */
    /* +0x0E */ u16 v_scale;
    /* +0x10 */ u16 tex_id;          /* the texture/sprite id `fn_80053E78` is handed */
    /* +0x12 */ u8 tex_flag;         /* the texture-rect flag byte (zero-extended to u16) */
    /* +0x13 */ u8 wide_idx;         /* the index `get_wide_offset` widens x by */
    /* +0x14 */ _mh_tex_uv_ uv0;
    /* +0x18 */ _mh_tex_uv_ uv1;
    /* +0x1C */ u32 color;           /* its low byte gates the blit in `fn_802E0CE4` */
    /* +0x20 */ const _SPR_ANIM_* anim; /* the animation table `draw_sprite_anim_*` walks */
} _SPR_DATA_;

#endif /* MHTRI_HUD_LAYOUT_TYPES_H */
