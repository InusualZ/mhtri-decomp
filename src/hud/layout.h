/* The types and declarations `src/hud/layout.cpp` owns (docs/plan.md 6.5, rule 1 and rule 2).
 *
 * This is the HUD's 2D element library: `draw_sprite*`, `draw_font*`, the icon putters and the
 * `_SPR_DATA_` record they all take.  Its consumers are the lobby screens
 * (`lobby/fn_801E7530.cpp`, `lobby/fn_801E0ADC.cpp`), the move indicator (`src/hud/fn_80324F7C.c`)
 * and the cockpit HUD above it, so the *public* half of this header is the set that
 * `unsplit/lobby.h` used to declare while no unit owned these addresses.
 *
 * `_mh_ivec2_` / `_mh_tex_uv_` / `_SPR_DATA_` are spelled with the map's own type names - the
 * manglings (`draw_sprite__FRC10_SPR_DATA_PC10_mh_ivec2_`, `...PC11_mh_tex_uv_...`) only come out
 * of a type with exactly that name (docs/plan.md 6.5 rule 1: a name more than one file needs lives
 * in one header; rules 3/4/5: every type states its size, every field its offset and a name).
 *
 * Rule-2 debt, recorded rather than guessed: the callees below whose map names carry an argument
 * list are declared at global C++ scope with the signature the target's call site shows (rule 9),
 * and the ones with a `fn_`/`lbl_` stem keep C linkage; the owner headers that exist today are
 * `fn_8004CAD8.h` (some of the `fn_8005xxxx` helpers), `g3d/g3d_anmchr.h` (the
 * font helpers) and `menu/menu_item.h` (the item table), and each owner's header should
 * carry its own declaration - these are this unit's own view until that pass happens.
 */
#ifndef MHTRI_HUD_LAYOUT_H
#define MHTRI_HUD_LAYOUT_H

#include "types.h"
#include "pl.h"

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

/* One animation key of a step's frame table: the frame the key starts at, whether its payload is
 * interpolated towards the next key, and the payload itself.  The key's size is what selects the
 * sub-record a step drives, so each step casts the table to its own key type.
 * size: 0x8 */
typedef struct _SPR_KEY_POS_ {
    /* +0x00 */ u16 frame;
    /* +0x02 */ u16 interp;
    /* +0x04 */ _mh_ivec2_ pos;
} _SPR_KEY_POS_;      /* kind 0, the record's anchor */

/* size: 0x6 */
typedef struct _SPR_KEY_TEX_ {
    /* +0x00 */ u16 frame;
    /* +0x02 */ u16 interp;
    /* +0x04 */ u16 tex_id;
} _SPR_KEY_TEX_;      /* kind 1, the record's texture id */

/* size: 0x8 */
typedef struct _SPR_KEY_SCALE_ {
    /* +0x00 */ u16 frame;
    /* +0x02 */ u16 interp;
    /* +0x04 */ u16 u_scale;
    /* +0x06 */ u16 v_scale;
} _SPR_KEY_SCALE_;    /* kind 2, the record's uv scales */

/* size: 0xC */
typedef struct _SPR_KEY_UV_ {
    /* +0x00 */ u16 frame;
    /* +0x02 */ u16 interp;
    /* +0x04 */ _mh_tex_uv_ uv;   /* the first coordinate pair */
    /* +0x08 */ _mh_tex_uv_ size; /* the second pair's offset from the first */
} _SPR_KEY_UV_;       /* kind 3, the record's texture coordinates */

/* size: 0x8 */
typedef struct _SPR_KEY_COLOR_ {
    /* +0x00 */ u16 frame;
    /* +0x02 */ u16 interp;
    /* +0x04 */ u32 color;
} _SPR_KEY_COLOR_;    /* kind 4, the record's colour word */

#ifdef __cplusplus
extern "C" {
#endif

/* The `fn_8005xxxx` helpers of the draw-shape band (`src/fn_8004CAD8.cpp` and `src/draw_shape.cpp`
 * own these addresses; rule 2 debt above).  `fn_800526E4`/`uv_pair_copy` are the two-halfword copy,
 * the rest are the pipeline steps the `draw_*` family drives. */
void fn_800526E4(void* dst, const void* src);                  /* 0x800526E4 */
void uv_pair_copy(void* dst, const void* src);                  /* 0x800526F8 */
void fn_80053E78(const f32* scale, const f32* offset, u16 id); /* 0x80053E78 */
void fn_80055C5C(void* uv0, void* uv1, u8 tex_idx);            /* 0x80055C5C */
void fn_80055D20(void* uv0, void* uv1, u8 tex_idx);            /* 0x80055D20 */
void fn_80055D8C(void* uv0, void* uv1, u8 tex_idx);            /* 0x80055D8C */
void fn_80055CC4(void* uv0, void* uv1, u8 tex_idx);            /* 0x80055CC4 */
void fn_80055DC8(void* uv0, void* uv1, u8 tex_idx);            /* 0x80055DC8 */
void spr_data_copy(_SPR_DATA_* dst, const _SPR_DATA_* src);      /* 0x801E6850, the record copy */

const _SPR_ANIM_* fn_802E0714(u16 id);                          /* 0x802E0714 */
u32 get_rare_color(u8 index);                                  /* 0x802DB254 */

/* This unit's own bodies, in address order. */
void fn_802E08E8(u16 id, u32 color, const _mh_ivec2_* pos);
u32 fn_802E099C(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame);
u32 fn_802E0A24(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame, _mh_ivec2_* out);
u32 sprite_frame_apply(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out);
u16 fn_802E0B54(u16 id);
u16 sprite_ary_last_frame(const u16* ids);
u32 fn_802E0CE4(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 part, const _mh_ivec2_* pos);
u32 fn_802E0DA8(_SPR_DATA_* rec, u16 part, const _mh_ivec2_* pos);
void fn_802E0F78(s16 x, s16 y, s16 w, s16 h, u32 color, u8 wide_idx);
void fn_802E1024(_SPR_DATA_* rec, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
void fn_802E1134(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
void fn_802E1190(u16 id, u16 item_id, const _mh_ivec2_* pos);
void fn_802E1288(s16 x, s16 y, s16 size, u32 color, u8 tex_idx);
u32 fn_802E12A0(u16 id, u16 part, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
void fn_802E1320(u16 id, u16 part, u16 item_id, const _mh_ivec2_* pos);
void fn_802E1400(_SPR_DATA_* spr, _EQUIP* equip, const _mh_ivec2_* pos, u8 rare);
void fn_802E14CC(u16 id, _EQUIP* equip, const _mh_ivec2_* pos);
void fn_802E1518(_SPR_DATA_* spr, u8 kind, u32 color, const _mh_ivec2_* pos);
void fn_802E1588(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
void fn_802E163C(u16 id, u8 tex_idx, const _mh_ivec2_* pos);
void fn_802E16E0(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos);
void fn_802E1794(s16 x, s16 y, s16 size, u32 color, u8 tex_idx);
void fn_802E1834(s16 x, s16 y, s16 size, u32 color, u8 tex_idx);
void fn_802E18D4(u16 id, u8 tex_idx, const _mh_ivec2_* pos);
s8 fn_802E1978(u8 index);
void fn_802E198C(_SPR_DATA_* rec, u8 tex_idx, const _mh_ivec2_* pos);
u32 fn_802E1A7C(u16 id, u16 part, u8 tex_idx, const _mh_ivec2_* pos);
void fn_802E1AEC(_SPR_DATA_* rec, u8 frame, u32 color, const _mh_ivec2_* pos);
u32 draw_number_anim(u16 id, u16 part, u8 frame, u32 color, const _mh_ivec2_* pos);
void fn_802E1C74(_SPR_DATA_* rec, u8 frame, u32 color, const _mh_ivec2_* pos);
void fn_802E1D30(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos);
void draw_number_digits_ary(const u16* ids, s32 number, u32 color, const _mh_ivec2_* pos);
void draw_number_anim_ary(const u16* ids, u16 part, s32 number, u32 color, const _mh_ivec2_* pos);
void draw_number_2digit_ary(const u16* ids, u8 value, u32 color, const _mh_ivec2_* pos);
u32 draw_number_2digit_anim_ary(const u16* ids, u16 part, u8 value, u32 color, const _mh_ivec2_* pos);
u16 anim_frame_wrap(u16 period, u16 frame);
u32 color_scale(u32 color, f32 scale, f32 low_scale);
u32 fn_802E29F4(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame);
u32 anim_step_tex_id(u16* out, const _SPR_ANIM_* anim, u16 frame);
u32 fn_802E2D84(u16* out, const _SPR_ANIM_* anim, u16 frame);
u32 anim_step_uv(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, const _SPR_ANIM_* anim, u16 frame);
u32 anim_step_color(u32* out, const _SPR_ANIM_* anim, u16 frame);

/* `color_lerp` (0x802E270C) is **not declared here** - `src/hud/layout.cpp` declares it
 * `extern "C"` instead.  Three consumer headers already declare the same name with their own
 * signatures (`hud/cockpit_quest.h` as `s32 color_lerp(s32, s32, f32)`,
 * `menu/fn_802E4978.h` with five parameters and `menu/menu_item_page.h` with two)
 * and their call sites pass that many arguments, so a declaration here is a C++ overload clash the
 * moment a consumer includes both headers (measured: `(10197) illegal function overloading` in
 * `hud/cockpit_quest.cpp` and `ef/eft050.cpp`).  Those three are pre-existing rule-2 debt; folding
 * them into this header means fixing their call sites (2 and 5 arguments -> 3), filed, not done. */
s16 note_text_max_len(u32 size, char* str);
void note_box_anchor_upper(_mh_ivec2_* pos);
void note_box_anchor_lower(_mh_ivec2_* pos);
void note_box_size(_mh_ivec2_* extent, s16 lines);
void note_box_pos_upper(_mh_ivec2_* pos, s16 lines);
void note_box_pos_lower(_mh_ivec2_* pos, s16 lines);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The mangled map names are the compiler's spelling of these declarations (rule 9): the front-end
 * reproduces each map name exactly, and the call site writes the plain function. */

/* The cockpit-side getters this library reads.  Their owners are the `cockpit.cpp` range above
 * (0x802D9EB4..0x802E0740), which `ai/fn_802D44F4.cpp` currently ends at 0x802DDC04 - rule 2 debt
 * until that range is re-cut (see this unit's file header).  They are declared here, at global C++
 * scope, because their map names carry an argument list - a declaration inside the `extern "C"`
 * block above relocates to the bare stem, which no link input defines. */
_SPR_DATA_* get_lsp_data(u16 id, _mh_ivec2_* out);             /* 0x802E0550 */
s32 get_wide_offset(u8 index);                                 /* 0x802E0490 */

void draw_sprite(const _SPR_DATA_& spr, const _mh_ivec2_* pos);
void draw_font(const _SPR_DATA_& spr, s8* str, u32 flags, const _mh_ivec2_* pos);
void draw_font_idx(u16 id, s8* str, u32 flags, const _mh_ivec2_* pos);
void draw_font_order(const _mh_ivec2_* anchor, s16 width, s16 height, u32 color, s8* str, u32 flags,
                     const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void draw_sprite_ary(const u16* ids, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 anim, const _mh_ivec2_* pos);
u32 draw_sprite_anim_ary(const u16* ids, u16 anim, const _mh_ivec2_* pos);
void put_button_icon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx, u16 tex_flag);
void put_button_icon(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx);
void put_itemicon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u16 tex_flag);
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos);
void draw_equipicon(const _SPR_DATA_& spr, _EQUIP* equip, const _mh_ivec2_* pos);
void draw_weaponicon_idx(u16 id, u8 kind, u32 color, const _mh_ivec2_* pos);
void draw_monstericon_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos);
void draw_number_idx(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos);
u32 color_mult(u32 a, u32 b);
u8 Get_equip_rare(_EQUIP* equip);
u32 Gunner_opt_ok_ck(_EQUIP* equip);
u8 Get_pl_type(_EQUIP* equipA, _EQUIP* equipB);
void drawshape_init(u8 kind, u16 arg);
void drawshape_set_vertex_rect(s16 x, s16 y, s16 w, s16 h);
void drawshape_set_vertex_rect(f32 x, f32 y, f32 w, f32 h); /* 0x800538EC, the float overload */
void drawshape_set_flat_color(u32 color);
void drawshape_set_texture_rect(u16 id, const _mh_tex_uv_* uv0, const _mh_tex_uv_* uv1);
void drawshape_exec(void);
#endif

#endif /* MHTRI_HUD_LAYOUT_H */
