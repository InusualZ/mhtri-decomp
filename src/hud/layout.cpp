/* hud/layout.cpp - the HUD's 2D element library: sprite/font/icon draws, sprite animation steps and window frames.
 * RANGE. .text 0x802E0740-0x802E4978 (88 functions); extab 0x80014DAC-0x80014FFC, extabindex 0x800332E8-0x80033660,
 *   .data 0x805D5798-0x805D5B48 (every table defined here, byte-identical), .sdata 0x807927B0-0x807927C0,
 *   .sdata2 0x8079A8C0-0x8079A8E0.  Name from the one `__FILE__` string "layout.cpp" (0x805D5800), cited by
 *   `knj_msg_str_build` and `str_tbl_utf16_build` only.
 * RANGE. The TU starts at 0x802E0480, not 0x802E0740: `get_lsp_data_ptr`, `get_sub_lsp_data_ptr`, `get_wide_offset`,
 *   `get_lsp_data`, `put_lsp_anchor_offset` and `get_lsp_anim` (0x802E0480-0x802E0740, registered under hud/cockpit)
 *   belong here - `get_wide_offset` loads 0.5f from 0x8079A8C0, the first word of this unit's `.sdata2` claim,
 *   while hud/cockpit pools its own 0.5f at 0x8079A878 (seam request hud-layout#1).
 * FLAGS. cflags_hud (Wii/1.3 -O3, -inline noauto, -Cpp_exceptions on, -opt nopeephole); `#pragma fp_contract off`
 *   (retail keeps fmuls+fadds) and `#pragma pool_data off` (every table its own lis/addi) for this file.
 * NAMES. GUESS (from each body and its callers): spr_anim_draw, sprite_anim_last_frame, sprite_frame_apply,
 *   spr_anim_frame_set, anim_step_apply, spr_anim_frame_draw, draw_sprite_idx_color, draw_tex_icon_idx,
 *   draw_itemicon_item_id_idx, draw_itemicon_anim_idx, draw_equipicon_idx, draw_weaponicon_rec,
 *   draw_sprite_uv_color_idx, draw_sprite_uv_idx, draw_tex_icon_square, draw_uv_rect, monster_icon_cell_get,
 *   draw_monstericon_rec, draw_monstericon_anim_idx, draw_number_digit_rec, draw_number_rec, draw_number_rec_idx,
 *   draw_number_*_ary, draw_font_align_at, draw_font_align, draw_font_align_idx, draw_font_anim_idx,
 *   knj_msg_str_build, str_tbl_utf16_build, anim_frame_wrap, color_lerp, color_scale, anim_step_pos,
 *   anim_step_scale, anim_step_tex_id, anim_step_uv, anim_step_color, window_frame_tex_draw,
 *   draw_layout_window_style, draw_title_window, draw_prompt_frame, draw_window_box, window_body_draw,
 *   draw_layout_window, draw_window_frame, note_line_draw, note_text_max_len, note_text_draw, note_box_*,
 *   note_box_window_draw, draw_help_window, draw_window_frame_style, draw_window_frame_preset, draw_spin_icon,
 *   get_lsp_anim; every table name (button_icon_uvs, uv_rect_uvs, equip_kind_icon_tex, weapon_type_icon_tex,
 *   monster_icon_cells, window_*, title_*, prompt_frame_*, window_box_uvs, window_frame_presets).
 * RESIDUALS. note_box_size: one extra `extsh` after the `neg` (four source shapes reproduce it).
 * RESIDUALS. note_line_draw / note_text_draw: retail narrows the centre into a temporary, ours updates `x` in place.
 * RESIDUALS. draw_spin_icon: `li r4,0x4000` is scheduled before the `fmr f1` (retail after).
 * RESIDUALS. window_frame_tex_draw: frame 0x100 vs 0xA0 (96 bytes of retail locals no body reads) and a `cmpwi`
 *   where ours has `cmplwi` on `style == 1`.
 * RESIDUALS. draw_title_window: retail sign-extends `x` again before `x + 32`.
 * RESIDUALS. draw_prompt_frame: retail re-indexes the strip table (`lhax`), ours reuses the element address.
 * RESIDUALS. window_body_draw: three locals swap registers.
 * RESIDUALS. draw_sprite: retail materialises the 0x4330 word twice.
 * RESIDUALS. .sdata2: our pool lacks the 0.5f `get_wide_offset` (the seam above) pools at 0x8079A8C0.
 * SHAPES. A parameter changed in place (`y -= 8`, `width &= 0xFFF0`) is narrowed lazily, which is what keeps
 *   retail's raw stores and unextended adds; a fresh `s16` local is narrowed at once.
 * SHAPES. color_lerp / color_scale pack their channels as `(c0 | c1 << 8) | (c3 << 24 | c2 << 16)` - the grouping
 *   is the OR order retail emits.
 * SHAPES. Vertex arrays are filled with chained assignments in retail's store order (the rightmost target is
 *   stored first); `cur`/`prev` in the animation steps are declared `prev` first.
 */

#include "types.h"
#include "hud/layout.h"
#include "menu/menu_item.h"
#include "font/flfnt.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "hud/draw_font_align.h"
#include "font/font_print_plain.h"
#include "menu/menu_message.h"
#include "menu/menu_text_center_x_by_gaps.h"
#include "font/flKnjMsgNum.h"
#include "ef/system_core.h"
#include "nw4r/db_assert.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad/system_w.h"
#include "drawshape_set_rot_scale_ivec2.h"
#include "hud/get_lsp_anim.h"
#include "drawshape_set_tex_translate.h"
#include "drawshape_set_tex_scale.h"
#include "sound/fn_800E3CBC.h"
#include "draw_rect.h"
#include "draw_rect_set.h"
#include "draw_rect_fill.h"
#include "draw_shape.h"

/* `get_wide_offset` as this TU calls it: retail adds its result to an `s16` with no `extsh`, i.e. an `s16` return,
 * while `hud/layout.h` keeps the `s32` its other includers measure best with. */
#define get_wide_offset_s16 ((s16 (*)(u8))get_wide_offset)

/* Retail keeps every `a*b + c` as its own `fmuls` + `fadds` (the animation-step interpolations). */
#pragma fp_contract off

/* Retail addresses each `.data` table with its own `lis`/`addi` pair (`window_frame_tex_draw` loads every uv and
 * offset table so), not through one pooled base register. */
#pragma pool_data off

/* `color_lerp` (0x802E270C) is declared here rather than in `hud/layout.h`: three consumer headers
 * declare the same name with their own signatures, so the owner's header would be a C++ overload
 * clash for every consumer that includes two of them (see the note in the header).  The declaration
 * is this TU's own - it owns the address - and `extern "C"` is what gives the definition the
 * unmangled name the map and objdiff pair by. */
extern "C" u32 color_lerp(u32 a, u32 b, f32 t);


/* 0x802E0740 (0x16C) - the 2D blit behind every `draw_*` entry.  The record's own anchor and its
 * two texture-coordinate pairs are copied out, the anchor is shaken by the caller's `pos`, both
 * halves go through the int->float conversion, the uv scales are normalised over 100.0f, and the
 * draw-shape pipeline is armed, given the rectangle/colour/texture and executed. */
void draw_sprite(const _SPR_DATA_& spr, const _mh_ivec2_* pos)
{
    _mh_ivec2_ anchor;
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;
    f32 scale[2];
    f32 posf[2];

    uv_pair_copy(&anchor, &spr.pos);
    tex_uv_copy(&uv0, &spr.uv0);
    tex_uv_copy(&uv1, &spr.uv1);
    if (pos != 0) {
        anchor.x += pos->x;
        anchor.y += pos->y;
    }
    posf[0] = (f32)anchor.x;
    posf[1] = (f32)anchor.y;
    anchor.x += spr.ofs_x;
    anchor.y += spr.ofs_y;
    scale[0] = (f32)spr.u_scale / 100.0f;
    scale[1] = (f32)spr.v_scale / 100.0f;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(anchor.x, anchor.y, spr.width, spr.height);
    drawshape_set_flat_color(spr.color);
    drawshape_set_texture_rect(spr.tex_flag, &uv0, &uv1);
    drawshape_set_rot_scale(scale, posf, spr.tex_id);
    drawshape_exec();
}

/* 0x802E08AC (0x3C) - blit the sprite record the id names, at `pos`. */
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos)
{
    draw_sprite(*get_lsp_data(id, 0), pos);
}

/* 0x802E08E8 (0x5C) - the same, with the record's colour word replaced by `color`. */
void draw_sprite_idx_color(u16 id, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ rec;

    spr_data_copy(&rec, get_lsp_data(id, 0));
    rec.color = color;
    draw_sprite(rec, pos);
}

/* 0x802E0944 (0x58) - blit every id in the 0xFFFF-terminated run at `pos`. */
void draw_sprite_ary(const u16* ids, const _mh_ivec2_* pos)
{
    for (; *ids != 0xFFFF; ids++) {
        draw_sprite(*get_lsp_data(*ids, 0), pos);
    }
}

/* 0x802E099C (0x88) - the animation-step dispatcher: the step record's low 12 bits select which
 * sub-record of `rec` the frame is drawn from, and the call is a tail call into the matching
 * `anim_step_pos`/`anim_step_tex_id`/`anim_step_scale`/`anim_step_uv`/`anim_step_color` family member.  Returns 0
 * for a kind it does not know. */
u32 anim_step_apply(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame)
{
    switch (anim->flags & SPR_ANIM_KIND_MASK) {
    case 0:
        return anim_step_pos(&rec->pos, anim, frame);
    case 2:
        return anim_step_scale(&rec->u_scale, anim, frame);
    case 1:
        return anim_step_tex_id(&rec->tex_id, anim, frame);
    case 3:
        return anim_step_uv(&rec->uv0, &rec->uv1, anim, frame);
    case 4:
        return anim_step_color(&rec->color, anim, frame);
    }
    return 0;
}

/* 0x802E0A24 (0xB0) - walk up to five animation steps of `anim`, stopping at the first whose frame
 * count has run out, and report whether any step drew.  `out` (optional) receives the record's
 * anchor, widened by `get_wide_offset_s16(rec->wide_idx)`. */
u32 spr_anim_frame_set(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame, _mh_ivec2_* out)
{
    u32 drawn = 0;

    if (anim != 0) {
        const _SPR_ANIM_* step = anim;
        s32 i;

        for (i = 5; i > 0; i--) {
            if (step->count <= 0) {
                break;
            }
            if (anim_step_apply(rec, step, frame) == 1) {
                drawn = 1;
            }
            step++;
        }
    }
    if (out != 0) {
        out->x = rec->pos.x + get_wide_offset_s16(rec->wide_idx);
        out->y = rec->pos.y;
    }
    return drawn;
}

/* 0x802E0AD4 (0x80) - copy the id's record into the caller's block, then run the id's animation
 * table against it with `part` as the frame. */
u32 sprite_frame_apply(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out)
{
    spr_data_copy(rec, get_lsp_data(id, 0));
    return spr_anim_frame_set(rec, get_lsp_anim(id), part, out);
}

/* 0x802E0B54 (0x12C) - the frame of the last key of the id's first animation step, at least 1 (1 when the
 * table is absent).  The five-pass loop re-reads the same step: retail never advances `anim`. */
u16 sprite_anim_last_frame(u16 id)
{
    const _SPR_ANIM_* anim = get_lsp_anim(id);
    u16 highest;
    s32 i;

    if (anim == 0) {
        return 1;
    }
    highest = 1;
    for (i = 5; i != 0; i--) {
        if (anim->count <= 0) {
            break;
        }
        s16 last = anim->count - 1;

        switch (anim->flags & SPR_ANIM_KIND_MASK) {
        case 0:
            if (highest < ((const _SPR_KEY_POS_*)anim->frames)[last].frame) {
                highest = ((const _SPR_KEY_POS_*)anim->frames)[last].frame;
            }
            break;
        case 2:
            if (highest < ((const _SPR_KEY_SCALE_*)anim->frames)[last].frame) {
                highest = ((const _SPR_KEY_SCALE_*)anim->frames)[last].frame;
            }
            break;
        case 1:
            if (highest < ((const _SPR_KEY_TEX_*)anim->frames)[last].frame) {
                highest = ((const _SPR_KEY_TEX_*)anim->frames)[last].frame;
            }
            break;
        case 3:
            if (highest < ((const _SPR_KEY_UV_*)anim->frames)[last].frame) {
                highest = ((const _SPR_KEY_UV_*)anim->frames)[last].frame;
            }
            break;
        case 4:
            if (highest < ((const _SPR_KEY_COLOR_*)anim->frames)[last].frame) {
                highest = ((const _SPR_KEY_COLOR_*)anim->frames)[last].frame;
            }
            break;
        }
    }
    return highest;
}

/* 0x802E0C80 (0x64) - the highest frame value over the 0xFFFF-terminated id run. */
u16 sprite_ary_last_frame(const u16* ids)
{
    u16 highest = 1;

    for (; *ids != 0xFFFF; ids++) {
        u16 value = sprite_anim_last_frame(*ids);

        if (highest < value) {
            highest = value;
        }
    }
    return highest;
}

/* 0x802E0CE4 (0xC4) - run `part` of the animation table against a *copy* of the record and blit
 * the copy when the step drew something and the copy still carries a colour and a uv scale. */
u32 spr_anim_frame_draw(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 part, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;
    u32 drawn;

    drawn = spr_anim_frame_set(&work, anim, part, 0);
    if ((work.color & 0xFF) != 0 && work.u_scale != 0 && work.v_scale != 0) {
        draw_sprite(work, pos);
    }
    return drawn;
}

/* 0x802E0DA8 (0x14) - the same from the record's own animation table. */
u32 spr_anim_draw(_SPR_DATA_* rec, u16 part, const _mh_ivec2_* pos)
{
    return spr_anim_frame_draw(rec, rec->anim, part, pos);
}

/* 0x802E0DBC (0x50) - blit frame `anim` of the id's animation table. */
void draw_sprite_anim_idx(u16 id, u16 anim, const _mh_ivec2_* pos)
{
    _SPR_DATA_* rec = get_lsp_data(id, 0);

    spr_anim_frame_draw(rec, rec->anim, anim, pos);
}

/* 0x802E0E0C (0x88) - the same over the 0xFFFF-terminated id run, reporting whether any of them
 * drew. */
u32 draw_sprite_anim_ary(const u16* ids, u16 anim, const _mh_ivec2_* pos)
{
    u32 drawn = 0;

    for (; *ids != 0xFFFF; ids++) {
        _SPR_DATA_* rec = get_lsp_data(*ids, 0);

        if (spr_anim_frame_draw(rec, rec->anim, anim, pos) != 0) {
            drawn = 1;
        }
    }
    return drawn;
}

/* .data 0x805D5798: the uv pair of each button-icon texture. */
_mh_tex_uv_ button_icon_uvs[8] = {
    { 163, 52 }, { 189, 78 }, { 31, 46 }, { 52, 67 }, { 189, 52 }, { 215, 78 }, { 228, 208 }, { 249, 229 },
};

/* 0x802E0E94 (0xC8) - draw texture `tex_idx` of the button-icon uv table as a square of side
 * `size`, widened along x by `get_wide_offset_s16(wide_idx)`. */
void put_button_icon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx, u16 tex_flag)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;
    u32 index;

    x += get_wide_offset_s16(wide_idx);
    index = (u32)tex_idx * 2;
    tex_uv_copy(&uv0, &button_icon_uvs[index]);
    tex_uv_copy(&uv1, &button_icon_uvs[index + 1]);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(tex_flag, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E0F5C (0x1C) - the button icon with the standard 8-pixel texture flag. */
void put_button_icon(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx)
{
    put_button_icon_tex(x, y, size, color, tex_idx, wide_idx, 8);
}

/* .sdata 0x807927B0: the uv pair `draw_uv_rect` blits. */
_mh_tex_uv_ uv_rect_uvs[2] = {
    { 216, 80 }, { 255, 129 },
};

/* 0x802E0F78 (0xAC) - a rectangle filled with the fixed `.sdata` uv pair at scale 8. */
void draw_uv_rect(s16 x, s16 y, s16 w, s16 h, u32 color, u8 wide_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    x += get_wide_offset_s16(wide_idx);
    tex_uv_copy(&uv0, &uv_rect_uvs[0]);
    tex_uv_copy(&uv1, &uv_rect_uvs[1]);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, w, h);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(8, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1024 (0xAC) - blit a *copy* of the record with texture `tex_idx` and the colour
 * multiplied by `color`. */
void draw_tex_icon(_SPR_DATA_* rec, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;

    atlas_digit_uv_get(&work.uv0, &work.uv1, tex_idx);
    work.color = color_mult(work.color, color);
    draw_sprite(work, pos);
}

/* 0x802E10D0 (0x64) - the item icon: the item record's texture index and its kind's colour-table
 * entry feed the copy helper. */
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(id);

    draw_tex_icon((_SPR_DATA_*)&spr, item->tex_idx_0x004, item_kind_color_tbl[item->kind], pos);
}

/* 0x802E1134 (0x5C) - the same from the sprite id. */
void draw_tex_icon_idx(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    draw_tex_icon(get_lsp_data(id, 0), tex_idx, color, pos);
}

/* 0x802E1190 (0x64) - the item icon drawn with an arbitrary sprite id. */
void draw_itemicon_item_id_idx(u16 id, u16 item_id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(item_id);

    draw_tex_icon_idx(id, item->tex_idx_0x004, item_kind_color_tbl[item->kind], pos);
}

/* 0x802E11F4 (0x94) - a square filled with texture `tex_idx` of the item uv table. */
void put_itemicon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u16 tex_flag)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    atlas_digit_uv_get(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(tex_flag, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1288 (0x18) - the same with the standard texture flag. */
void put_itemicon(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    put_itemicon_tex(x, y, size, color, tex_idx, 0);
}

/* 0x802E12A0 (0x80) - the animation-step blit with a texture override: the record is built by
 * `sprite_frame_apply`, then handed to the copy-and-recolour helper. */
u32 draw_itemicon_frame_idx(u16 id, u16 part, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

    draw_tex_icon(&work, tex_idx, color, pos);
    return drawn;
}

/* 0x802E1320 (0x74) - the same keyed by the item record. */
void draw_itemicon_anim_idx(u16 id, u16 part, u16 item_id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(item_id);

    draw_itemicon_frame_idx(id, part, item->tex_idx_0x004, item_kind_color_tbl[item->kind], pos);
}

/* 0x802E1394 (0x6C) - the equipment icon: nothing is drawn for an empty slot. */
void draw_equipicon(const _SPR_DATA_& spr, _EQUIP* equip, const _mh_ivec2_* pos)
{
    if (equip->kind != 0) {
        draw_equipicon_rare((_SPR_DATA_*)&spr, equip, pos, Get_equip_rare(equip));
    }
}

/* .data 0x805D57B8: the icon texture of each equipment kind. */
u8 equip_kind_icon_tex[16] = {
    0x00, 0x5B, 0x5C, 0x5D, 0x5E, 0x5A, 0x28, 0x52, 0x51, 0x55, 0x54, 0x60, 0x61, 0x62, 0x53, 0x59,
};

/* 0x802E1400 (0xCC) - the equipment icon with the rarity colour: a gunner-only slot the player
 * cannot use falls back to the weapon-icon path, everything else uses the kind's texture. */
void draw_equipicon_rare(_SPR_DATA_* spr, _EQUIP* equip, const _mh_ivec2_* pos, u8 rare)
{
    if (equip->kind != 0) {
        if (equip->kind == 0xB && !Gunner_opt_ok_ck(equip)) {
            draw_weaponicon_rec(spr, Get_pl_type(equip, 0), get_rare_color(rare), pos);
        } else {
            draw_tex_icon(spr, equip_kind_icon_tex[equip->kind], get_rare_color(rare), pos);
        }
    }
}

/* 0x802E14CC (0x4C) - the equipment icon for a sprite id. */
void draw_equipicon_idx(u16 id, _EQUIP* equip, const _mh_ivec2_* pos)
{
    draw_equipicon(*get_lsp_data(id, 0), equip, pos);
}

/* .data 0x805D57C8: the icon texture of each weapon (player) type. */
u8 weapon_type_icon_tex[10] = {
    0x52, 0x51, 0x55, 0x54, 0x58, 0x57, 0x56, 0x53, 0x59, 0x00,
};

/* 0x802E1518 (0x14) - the weapon icon: the player type selects the texture out of a byte table. */
void draw_weaponicon_rec(_SPR_DATA_* spr, u8 kind, u32 color, const _mh_ivec2_* pos)
{
    draw_tex_icon(spr, weapon_type_icon_tex[kind], color, pos);
}

/* 0x802E152C (0x5C) - the same for a sprite id. */
void draw_weaponicon_idx(u16 id, u8 kind, u32 color, const _mh_ivec2_* pos)
{
    draw_weaponicon_rec(get_lsp_data(id, 0), kind, color, pos);
}

/* 0x802E1588 (0xB4) - the copy with texture `tex_idx` of the *colour* uv table and a colour word. */
void draw_sprite_uv_color_idx(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    atlas_icon_uv_get(&work.uv0, &work.uv1, tex_idx);
    work.color = color;
    draw_sprite(work, pos);
}

/* 0x802E163C (0xA4) - the same without the colour override. */
void draw_sprite_uv_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    atlas_cell20_uv_get(&work.uv0, &work.uv1, tex_idx);
    draw_sprite(work, pos);
}

/* 0x802E16E0 (0xB4) - the same with it. */
void draw_sprite_cell_color_idx(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    atlas_cell20_uv_get(&work.uv0, &work.uv1, tex_idx);
    work.color = color;
    draw_sprite(work, pos);
}

/* 0x802E1794 (0xA0) - a square of `_SPR_DATA_` uv pairs from `atlas_cell20_uv_get` at flag 1. */
void draw_tex_icon_square(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    atlas_cell20_uv_get(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(1, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1834 (0xA0) - the same at texture flag 0x1E. */
void draw_tex_icon_square_arena(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    atlas_cell20_uv_get(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(0x1E, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E18D4 (0xA4) - the sprite record with an `atlas_cell28_uv_get` uv pair. */
void draw_award_icon_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    atlas_cell28_uv_get(&work.uv0, &work.uv1, tex_idx);
    draw_sprite(work, pos);
}

/* .data 0x805D57D4: the monster icon cell of each monster kind (0xFF = no icon). */
u8 monster_icon_cells[41] = {
    0xFF, 0x16, 0x17, 0x15, 0x19, 0x1A, 0x18, 0x20, 0x13, 0x14, 0x0A, 0x0B, 0x11, 0x0C, 0x12, 0x1D, 0x1B, 0x0D, 0x1C,
    0x1E, 0x1F, 0x0E, 0x0F, 0x02, 0x22, 0x21, 0x09, 0x00, 0x03, 0x10, 0x06, 0x07, 0x08, 0x05, 0x01, 0xFF, 0x04, 0x04,
    0x04, 0x04, 0xFF,
};

/* 0x802E1978 (0x14) - the monster texture index a kind byte maps to; negative means "no icon". */
u8 monster_icon_cell_get(u8 index)
{
    return monster_icon_cells[index];
}

/* 0x802E198C (0xA4) - the sprite record with an `atlas_cell64_uv_get` uv pair, skipped when the kind has no
 * texture in the table (the `-1` the byte-table helper answers). */
void draw_monstericon_rec(_SPR_DATA_* rec, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;

    u8 cell = monster_icon_cell_get(tex_idx);

    if ((s8)cell >= 0) {
        atlas_cell64_uv_get(&work.uv0, &work.uv1, cell);
        draw_sprite(work, pos);
    }
}

/* 0x802E1A30 (0x4C) - the monster icon for a sprite id. */
void draw_monstericon_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    draw_monstericon_rec(get_lsp_data(id, 0), tex_idx, pos);
}

/* 0x802E1A7C (0x70) - the monster icon on an animation-built record. */
u32 draw_monstericon_anim_idx(u16 id, u16 part, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

    draw_monstericon_rec(&work, tex_idx, pos);
    return drawn;
}

/* 0x802E1AEC (0xAC) - the record with its u coordinates scrolled one frame further along the span
 * `uv0.u`..`uv1.u`, and an optional colour override. */
void draw_number_digit_rec(_SPR_DATA_* rec, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;
    s16 offset = (s16)(frame * (rec->uv1.u - rec->uv0.u));

    work.uv0.u += offset;
    work.uv1.u += offset;
    if (color != 0) {
        work.color = color;
    }
    draw_sprite(work, pos);
}

/* 0x802E1B98 (0x5C) - the animated number glyph for a sprite id. */
void draw_number_idx(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    draw_number_digit_rec(get_lsp_data(id, 0), frame, color, pos);
}

/* 0x802E1BF4 (0x80) - the same on an animation-built record. */
u32 draw_number_anim(u16 id, u16 part, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

    draw_number_digit_rec(&work, frame, color, pos);
    return drawn;
}

/* 0x802E1C74 (0xBC): Draws `number` in decimal over the record, least significant digit first, stepping left by
 * the record's width per digit. */
void draw_number_rec(_SPR_DATA_* rec, u32 number, u32 color, const _mh_ivec2_* pos)
{
    _mh_ivec2_ at;

    if (pos != 0) {
        uv_pair_copy(&at, pos);
    } else {
        at.y = 0;
        at.x = 0;
    }
    for (;;) {
        draw_number_digit_rec(rec, number % 10, color, &at);
        number /= 10;
        if (number == 0) {
            break;
        }
        at.x -= rec->width;
    }
}

/* 0x802E1D30 (0x5C) - the same for a sprite id. */
void draw_number_rec_idx(u16 id, u32 number, u32 color, const _mh_ivec2_* pos)
{
    draw_number_rec(get_lsp_data(id, 0), number, color, pos);
}

/* 0x802E1D8C (0xC0) - the same over the 0xFFFF-terminated id run, one digit per id, least
 * significant first, stopping when the value has no digits left. */
void draw_number_digits_ary(const u16* ids, s32 number, u32 color, const _mh_ivec2_* pos)
{
    draw_number_idx(*ids, number % 10, color, pos);
    for (;;) {
        ids++;
        if (*ids == 0xFFFF) {
            break;
        }
        number /= 10;
        if (number <= 0) {
            break;
        }
        draw_number_idx(*ids, number % 10, color, pos);
    }
}

/* 0x802E1E4C (0xCC) - the same on animation-built records. */
void draw_number_anim_ary(const u16* ids, u16 part, s32 number, u32 color, const _mh_ivec2_* pos)
{
    draw_number_anim(*ids, part, number % 10, color, pos);
    for (;;) {
        ids++;
        if (*ids == 0xFFFF) {
            break;
        }
        number /= 10;
        if (number <= 0) {
            break;
        }
        draw_number_anim(*ids, part, number % 10, color, pos);
    }
}

/* 0x802E1F18 (0xAC) - the fixed two-slot form: a value below ten uses the run's first id alone,
 * anything wider puts the tens on the second id and the units on the third. */
void draw_number_2digit_ary(const u16* ids, u8 value, u32 color, const _mh_ivec2_* pos)
{
    if (value < 10) {
        draw_number_idx(ids[0], value, color, pos);
    } else {
        draw_number_idx(ids[1], value / 10, color, pos);
        draw_number_idx(ids[2], value % 10, color, pos);
    }
}

/* 0x802E1FC4 (0xDC) - the two-slot form on animation-built records, reporting whether either
 * slot drew. */
u32 draw_number_2digit_anim_ary(const u16* ids, u16 part, u8 value, u32 color, const _mh_ivec2_* pos)
{
    if (value < 10) {
        return draw_number_anim(ids[0], part, value, color, pos);
    }
    u32 drawn0 = draw_number_anim(ids[1], part, value / 10, color, pos);
    u32 drawn1 = draw_number_anim(ids[2], part, value % 10, color, pos);

    return drawn0 || drawn1;
}

/* 0x802E20A0 (0x120): Prints `str` at `anchor` + `pos` in `width` x `height` glyphs, centred by the `flags` bits
 * (1 by width, 2 by gaps, 8 as a block), with a shadow under flag 4. */
void draw_font_order(const _mh_ivec2_* anchor, s16 width, s16 height, u32 color, s8* str, u32 flags,
                     const _mh_ivec2_* pos)
{
    s16 x = pos->x + anchor->x;
    s16 y = pos->y + anchor->y;

    if (flags & 1) {
        x = menu_text_center_x((char*)str, x, width);
    } else if (flags & 2) {
        x = menu_text_center_x_by_gaps((char*)str, x, width);
    } else if (flags & 8) {
        x = menu_text_block_center_x((char*)str, x, width, 0);
    }
    font_set_size(width, height);
    if (flags & 4) {
        font_print_ex(x + 2, y + 1, 1, str);
    }
    flfntSetColor(color);
    font_locate(x, y);
    flfntFontPuts((char*)str);
}

/* 0x802E21C0 (0x120): Prints `str` like `draw_font_order`, its shadow through the plain printer. */
void draw_font_align_at(const _mh_ivec2_* anchor, s16 width, s16 height, u32 color, s8* str, u32 flags,
                        const _mh_ivec2_* pos)
{
    s16 x = pos->x + anchor->x;
    s16 y = pos->y + anchor->y;

    if (flags & 1) {
        x = menu_text_center_x((char*)str, x, width);
    } else if (flags & 2) {
        x = menu_text_center_x_by_gaps((char*)str, x, width);
    } else if (flags & 8) {
        x = menu_text_block_center_x((char*)str, x, width, 0);
    }
    font_set_size(width, height);
    if (flags & 4) {
        font_print_plain(x + 2, y + 1, 1, (const char*)str);
    }
    flfntSetColor(color);
    font_locate(x, y);
    flfntFontPuts((char*)str);
}

/* 0x802E22E0 (0x1C) - draw `str` over the record's own anchor, width, height and colour. */
void draw_font(const _SPR_DATA_& spr, s8* str, u32 flags, const _mh_ivec2_* pos)
{
    draw_font_order(&spr.pos, spr.width, spr.height, spr.color, str, flags, pos);
}

/* 0x802E22FC (0x5C) - the same for a sprite id. */
void draw_font_idx(u16 id, s8* str, u32 flags, const _mh_ivec2_* pos)
{
    draw_font(*get_lsp_data(id, 0), str, flags, pos);
}

/* 0x802E2358 (0x1C): Prints `str` over the record's anchor, size and colour through `draw_font_align_at`. */
void draw_font_align(const _SPR_DATA_* spr, s8* str, u32 flags, const _mh_ivec2_* pos)
{
    draw_font_align_at(&spr->pos, spr->width, spr->height, spr->color, str, flags, pos);
}

/* 0x802E2374 (0x5C): Prints `str` aligned over layout `id`'s record. */
void draw_font_align_idx(u16 id, s8* str, u32 flags, const _mh_ivec2_* pos)
{
    draw_font_align(get_lsp_data(id, 0), str, flags, pos);
}

/* 0x802E23D0 (0x70): Prints `str` over layout `id`'s record posed at animation frame `part`. */
void draw_font_anim_idx(u16 id, u16 part, s8* str, u32 flags, const _mh_ivec2_* pos)
{
    _SPR_DATA_ rec;

    sprite_frame_apply(&rec, id, part, 0);
    draw_font(rec, str, flags, pos);
}

/* 0x802E2440 (0xE4): Copies the kanji message `src` into `dst`, breaking it into lines of `columns` characters. */
void knj_msg_str_build(s8* src, s8* dst, s32 columns, s32 lines)
{
    char line[0x208];

    if (columns > 64) {
        nw4r::db::Panic("layout.cpp", 1254, "NW4R:Failed assertion 0");
        return;
    }
    char* rest = (char*)src;
    s32 count = flKnjMsgNum(rest);
    dst[0] = 0;
    for (;;) {
        if (count <= columns) {
            strcat((char*)dst, rest);
            return;
        }
        line[0] = 0;
        strncpy(line, rest, 256);
        char* cut = flKnjMsgNumPtr(line, columns);
        if (cut != 0) {
            *cut = 0;
        }
        strcat((char*)dst, line);
        strcat((char*)dst, "\n");
        rest = flKnjMsgNumPtr(rest, columns);
        count -= columns;
    }
}

/* 0x802E2524 (0x1C8): Copies the first `count` characters of `src` into `dst`, ending a truncated text with the
 * string table's continuation mark. */
void str_tbl_utf16_build(s8* src, s8* dst, s32 count)
{
    s8 text[0x81];
    u8** table = get_str_tbl(32);
    u32 limit = count * 2;

    dst[0] = 0;
    if (count > 64) {
        nw4r::db::Panic("layout.cpp", 1296, "NW4R:Failed assertion 0");
        return;
    }
    if (limit < flfntStrLen((char*)src)) {
        u32 chars = 0;
        memset(text, 0, sizeof(text));
        s8* out = text;
        do {
            s32 size = utf82unicode2((u8*)src);
            chars++;
            for (s32 i = 0; i < size; i++) {
                *out++ = (s8)*src++;
            }
        } while (chars < limit);
        strcat((char*)text, (char*)table[12]);
        strcpy((char*)dst, (char*)text);
    } else {
        strcpy((char*)dst, (char*)src);
    }
}

/* 0x802E26EC (0x20) - the frame a step's table value wraps `frame` into: the table's last entry is
 * the number of frames the step holds, so the wrap period is that value plus one. */
u16 anim_frame_wrap(u16 period, u16 frame)
{
    return frame % (period + 1);
}

/* 0x802E270C (0x130) - the per-channel blend `a`*(1-t) + `b`*t of two colour words, repacked in
 * the record's own channel order (low byte, then 0x08, then 0x18, then 0x10). */
u32 color_lerp(u32 a, u32 b, f32 t)
{
    f32 u = 1.0f - t;
    f32 c3 = (f32)(a >> 24) * u + (f32)(b >> 24) * t;
    f32 c2 = (f32)((a >> 16) & 0xFF) * u + (f32)((b >> 16) & 0xFF) * t;
    f32 c1 = (f32)((a >> 8) & 0xFF) * u + (f32)((b >> 8) & 0xFF) * t;
    f32 c0 = (f32)(a & 0xFF) * u + (f32)(b & 0xFF) * t;

    return ((u8)c0 | ((u8)c1 << 8)) | (((u8)c3 << 24) | ((u8)c2 << 16));
}

/* 0x802E283C (0x74) - the per-channel product of two ARGB colour words, each channel scaled by
 * 1/255. */
u32 color_mult(u32 a, u32 b)
{
    u32 lo = ((a & 0xFF) * (b & 0xFF)) / 255
           | ((((a >> 8) & 0xFF) * ((b >> 8) & 0xFF)) / 255) << 8;
    u32 hi = ((((a >> 24) & 0xFF) * ((b >> 24) & 0xFF)) / 255) << 24
           | ((((a >> 16) & 0xFF) * ((b >> 16) & 0xFF)) / 255) << 16;

    return lo | hi;
}

/* 0x802E28B0 (0x144) - the colour word with three of its channels scaled by `scale` and the low
 * one by `low_scale`, every channel clamped at 255. */
u32 color_scale(u32 color, f32 scale, f32 low_scale)
{
    u32 c3 = (u32)((f32)(color >> 24) * scale);
    u32 c2 = (u32)((f32)((color >> 16) & 0xFF) * scale);
    u32 c1 = (u32)((f32)((color >> 8) & 0xFF) * scale);
    u32 c0 = (u32)((f32)(color & 0xFF) * low_scale);

    if (c3 > 255) {
        c3 = 255;
    }
    if (c2 > 255) {
        c2 = 255;
    }
    if (c1 > 255) {
        c1 = 255;
    }
    if (c0 > 255) {
        c0 = 255;
    }
    return (c0 | (c1 << 8)) | ((c3 << 24) | (c2 << 16));
}

/* The animation-step walk every `anim_step_*` entry shares: the step's `count` frames are a table
 * of `frame`/`interp`-tagged keys, and the walk finds the two keys `frame` falls between, then
 * interpolates their payloads.  Returns 1 when a payload was written and 0 when the frame ran past
 * the step's last key. */

/* 0x802E29F4 (0x214): The anchor step (kind 0, an 8-byte key holding a position). */
u32 anim_step_pos(_mh_ivec2_* out, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_POS_* prev;
    const _SPR_KEY_POS_* cur = (const _SPR_KEY_POS_*)anim->frames;
    prev = cur;
    s16 count = anim->count;

    if ((anim->flags & SPR_ANIM_WRAP_LAST) != 0) {
        frame = anim_frame_wrap(cur[count - 1].frame, frame);
    }
    if (frame == 0) {
        uv_pair_copy(out, &cur->pos);
        return 1;
    }
    for (;;) {
        if (cur->frame == frame) {
            uv_pair_copy(out, &cur->pos);
            return 1;
        }
        if (cur->frame > frame) {
            break;
        }
        count--;
        if (count <= 0) {
            uv_pair_copy(out, &cur->pos);
            return 0;
        }
        prev = cur;
        cur++;
    }
    if (prev->interp == 0) {
        uv_pair_copy(out, &prev->pos);
    } else {
        f32 rate = (f32)(frame - prev->frame) / (f32)(cur->frame - prev->frame);

        out->x = (s16)((f32)prev->pos.x + rate * (s16)(f32)(cur->pos.x - prev->pos.x));
        out->y = (s16)((f32)prev->pos.y + rate * (s16)(f32)(cur->pos.y - prev->pos.y));
    }
    return 1;
}

/* 0x802E2C08 (0x17C) - the texture-id step (kind 1, a 6-byte key). */
u32 anim_step_tex_id(u16* out, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_TEX_* prev;
    const _SPR_KEY_TEX_* cur = (const _SPR_KEY_TEX_*)anim->frames;
    prev = cur;
    s16 count = anim->count;

    if ((anim->flags & SPR_ANIM_WRAP_LAST) != 0) {
        frame = anim_frame_wrap(cur[count - 1].frame, frame);
    }
    if (frame == 0) {
        *out = cur->tex_id;
        return 1;
    }
    for (;;) {
        if (cur->frame == frame) {
            *out = cur->tex_id;
            return 1;
        }
        if (cur->frame > frame) {
            break;
        }
        count--;
        if (count <= 0) {
            *out = cur->tex_id;
            return 0;
        }
        prev = cur;
        cur++;
    }
    if (prev->interp == 0) {
        *out = prev->tex_id;
    } else {
        f32 rate = (f32)(frame - prev->frame) / (f32)(cur->frame - prev->frame);
        s16 from = prev->tex_id;
        s16 delta = cur->tex_id - prev->tex_id;

        *out = from + (s16)(rate * (f32)delta);
    }
    return 1;
}

/* 0x802E2D84 (0x1D0): The uv-scale step (kind 2, an 8-byte key holding the two scales). */
u32 anim_step_scale(u16* out, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_SCALE_* prev;
    const _SPR_KEY_SCALE_* cur = (const _SPR_KEY_SCALE_*)anim->frames;
    prev = cur;
    s16 count = anim->count;

    if ((anim->flags & SPR_ANIM_WRAP_LAST) != 0) {
        frame = anim_frame_wrap(cur[count - 1].frame, frame);
    }
    if (frame == 0) {
        menu_word_pair_copy(out, &cur->u_scale);
        return 1;
    }
    for (;;) {
        if (cur->frame == frame) {
            menu_word_pair_copy(out, &cur->u_scale);
            return 1;
        }
        if (cur->frame > frame) {
            break;
        }
        count--;
        if (count <= 0) {
            menu_word_pair_copy(out, &cur->u_scale);
            return 0;
        }
        prev = cur;
        cur++;
    }
    if (prev->interp == 0) {
        menu_word_pair_copy(out, &prev->u_scale);
    } else {
        f32 rate = (f32)(frame - prev->frame) / (f32)(cur->frame - prev->frame);

        out[0] = (u16)((f32)prev->u_scale + rate * (f32)(cur->u_scale - prev->u_scale));
        out[1] = (u16)((f32)prev->v_scale + rate * (f32)(cur->v_scale - prev->v_scale));
    }
    return 1;
}

/* 0x802E2F54 (0x318) - the texture-coordinate step (kind 3, a 12-byte key holding the first pair
 * and the second pair's offset from it). */
u32 anim_step_uv(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_UV_* prev;
    const _SPR_KEY_UV_* cur = (const _SPR_KEY_UV_*)anim->frames;
    prev = cur;
    s16 count = anim->count;

    if ((anim->flags & SPR_ANIM_WRAP_LAST) != 0) {
        frame = anim_frame_wrap(cur[count - 1].frame, frame);
    }
    if (frame == 0) {
        uv0->u = cur->uv.u;
        uv0->v = cur->uv.v;
        uv1->u = cur->uv.u + cur->size.u;
        uv1->v = cur->uv.v + cur->size.v;
        return 1;
    }
    for (;;) {
        if (cur->frame == frame) {
            uv0->u = cur->uv.u;
            uv0->v = cur->uv.v;
            uv1->u = cur->uv.u + cur->size.u;
            uv1->v = cur->uv.v + cur->size.v;
            return 1;
        }
        if (cur->frame > frame) {
            break;
        }
        count--;
        if (count <= 0) {
            uv0->u = cur->uv.u;
            uv0->v = cur->uv.v;
            uv1->u = cur->uv.u + cur->size.u;
            uv1->v = cur->uv.v + cur->size.v;
            return 0;
        }
        prev = cur;
        cur++;
    }
    if (prev->interp == 0) {
        uv0->u = prev->uv.u;
        uv0->v = prev->uv.v;
        uv1->u = prev->uv.u + prev->size.u;
        uv1->v = prev->uv.v + prev->size.v;
    } else {
        f32 rate = (f32)(frame - prev->frame) / (f32)(cur->frame - prev->frame);

        uv0->u = (s16)((f32)prev->uv.u + rate * (f32)(cur->uv.u - prev->uv.u));
        uv0->v = (s16)((f32)prev->uv.v + rate * (f32)(cur->uv.v - prev->uv.v));
        uv1->u = uv0->u + (s16)((f32)prev->size.u + rate * (f32)(cur->size.u - prev->size.u));
        uv1->v = uv0->v + (s16)((f32)prev->size.v + rate * (f32)(cur->size.v - prev->size.v));
    }
    return 1;
}

/* 0x802E326C (0x148) - the colour step (kind 4, an 8-byte key holding a colour word). */
u32 anim_step_color(u32* out, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_COLOR_* prev;
    const _SPR_KEY_COLOR_* cur = (const _SPR_KEY_COLOR_*)anim->frames;
    prev = cur;
    s16 count = anim->count;

    if ((anim->flags & SPR_ANIM_WRAP_LAST) != 0) {
        frame = anim_frame_wrap(cur[count - 1].frame, frame);
    }
    if (frame == 0) {
        *out = cur->color;
        return 1;
    }
    for (;;) {
        if (cur->frame == frame) {
            *out = cur->color;
            return 1;
        }
        if (cur->frame > frame) {
            break;
        }
        count--;
        if (count <= 0) {
            *out = cur->color;
            return 0;
        }
        prev = cur;
        cur++;
    }
    if (prev->interp == 0) {
        *out = prev->color;
    } else {
        f32 rate = (f32)(frame - prev->frame) / (f32)(cur->frame - prev->frame);

        *out = color_lerp(prev->color, cur->color, rate);
    }
    return 1;
}

/* One horizontal strip of a window body texture: where it starts in v and how tall it is.  size: 0x4 */
typedef struct WindowBodyStrip {
    /* +0x00 */ s16 v;
    /* +0x02 */ s16 height;
} WindowBodyStrip;

/* .data 0x805D5828: the body texture strips `window_body_draw` and `draw_prompt_frame` tile, per body kind. */
WindowBodyStrip window_body_strips[8] = {
    { 0, 0 }, { 0, 32 }, { 36, 16 }, { 56, 24 }, { 84, 24 }, { 112, 16 }, { 128, 16 }, { 0, 0 },
};

/* .data 0x805D5848 / 0x805D5888: the uvs of the frame's four 32x32 corner quads - style 1 has its own corner art,
 * every other style mirrors one corner tile. */
_mh_tex_uv_ window_corner_uvs_style1[16] = {
    { 0, 0 }, { 32, 0 }, { 32, 32 }, { 0, 32 }, { 32, 0 }, { 64, 0 }, { 64, 32 }, { 32, 32 },
    { 0, 32 }, { 32, 32 }, { 32, 64 }, { 0, 64 }, { 32, 32 }, { 64, 32 }, { 64, 64 }, { 32, 64 },
};
_mh_tex_uv_ window_corner_uvs[16] = {
    { 32, 0 }, { 0, 0 }, { 0, 32 }, { 32, 32 }, { 0, 0 }, { 32, 0 }, { 32, 32 }, { 0, 32 },
    { 32, 32 }, { 0, 32 }, { 0, 0 }, { 32, 0 }, { 0, 32 }, { 32, 32 }, { 32, 0 }, { 0, 0 },
};

/* .data 0x805D58C8 / 0x805D58E8: the uvs of the top/bottom and left/right edge quads (one texel stretched). */
_mh_tex_uv_ window_edge_x_uvs[8] = {
    { 0, 0 }, { 1, 0 }, { 1, 16 }, { 0, 16 }, { 0, 16 }, { 1, 16 }, { 1, 32 }, { 0, 32 },
};
_mh_tex_uv_ window_edge_y_uvs[8] = {
    { 0, 0 }, { 16, 0 }, { 16, 1 }, { 0, 1 }, { 16, 0 }, { 32, 0 }, { 32, 1 }, { 16, 1 },
};

/* .data 0x805D5908 / 0x805D5938 / 0x805D5968: per frame style, the corner texture translation and the top/bottom
 * and left/right edge texture offsets. */
f32 window_corner_translate[6][2] = {
    { 0.5546875f, 0.0703125f }, { 0.0f, 0.6328125f }, { 0.0885416642f, 0.5f },
    { 0.0f, 0.5f }, { 0.177083328f, 0.5f }, { 0.0f, 0.5f },
};
f32 window_edge_x_offset[6][2] = {
    { 0.0f, 0.75f }, { 0.0f, 0.125f }, { 0.0f, 0.25f }, { 0.0f, 0.625f }, { 0.0f, 0.375f }, { 0.0f, 0.0f },
};
f32 window_edge_y_offset[6][2] = {
    { 0.75f, 0.0f }, { 0.125f, 0.0f }, { 0.25f, 0.0f }, { 0.375f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f },
};

/* 0x802E33B4 (0x314): Draws a textured window frame: the body, then the four corners, the top/bottom edges and
 * the left/right edges stretched over a size rounded down to 16 (at least 64x64). */
void window_frame_tex_draw(s16 x, s16 y, u16 width, u16 height, u8 style, u8 shade, u32 color, u32 fill,
                           u16 edge_x_tex, u16 edge_y_tex, u16 corner_tex)
{
    f32 scale[2];
    _mh_ivec2_ verts[16];
    _mh_tex_uv_* corners;

    width &= 0xFFF0;
    height &= 0xFFF0;
    if (width < 64) {
        width = 64;
    }
    if (height < 64) {
        height = 64;
    }
    window_body_draw(x, y, width, height, shade, fill, 0);

    s16 x32 = x + 32;
    s16 y32 = y + 32;
    s16 right32;
    s16 bottom32;
    s16 right = x + width;
    s16 bottom = y + height;

    right32 = right - 32;
    bottom32 = bottom - 32;

    verts[0].x = verts[3].x = verts[8].x = verts[11].x = x;
    verts[0].y = verts[1].y = verts[4].y = verts[5].y = y;
    verts[1].x = verts[2].x = verts[9].x = verts[10].x = x32;
    verts[2].y = verts[3].y = verts[6].y = verts[7].y = y32;
    verts[4].x = verts[7].x = verts[12].x = verts[15].x = right32;
    verts[8].y = verts[9].y = verts[12].y = verts[13].y = bottom32;
    verts[5].x = verts[6].x = verts[13].x = verts[14].x = right;
    verts[10].y = verts[11].y = verts[14].y = verts[15].y = bottom;
    if (style == 1) {
        corners = window_corner_uvs_style1;
    } else {
        corners = window_corner_uvs;
    }
    drawshape_init(3, 16);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(corner_tex, corners);
    drawshape_set_tex_translate(window_corner_translate[style]);
    drawshape_exec();

    verts[0].x = verts[3].x = verts[4].x = verts[7].x = x32;
    verts[1].x = verts[2].x = verts[5].x = verts[6].x = right32;
    verts[0].y = verts[1].y = y;
    verts[2].y = verts[3].y = y + 16;
    verts[4].y = verts[5].y = bottom - 16;
    verts[6].y = verts[7].y = bottom;
    scale[0] = width - 64;
    scale[1] = 1.0f;
    drawshape_init(3, 8);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(edge_x_tex, window_edge_x_uvs);
    drawshape_set_tex_scale(scale, window_edge_x_offset[style]);
    drawshape_exec();

    verts[0].x = verts[3].x = x;
    verts[1].x = verts[2].x = x + 16;
    verts[4].x = verts[7].x = right - 16;
    verts[5].x = verts[6].x = right;
    verts[0].y = verts[1].y = verts[4].y = verts[5].y = y32;
    verts[2].y = verts[3].y = verts[6].y = verts[7].y = bottom32;
    scale[0] = 1.0f;
    scale[1] = height - 64;
    drawshape_init(3, 8);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(edge_y_tex, window_edge_y_uvs);
    drawshape_set_tex_scale(scale, window_edge_y_offset[style]);
    drawshape_exec();
}

/* 0x802E36C8 (0x50): Draws a textured window frame with the default edge and corner textures (3, 4 and 5). */
void draw_layout_window_style(s16 x, s16 y, u16 width, u16 height, u8 style, u8 shade, u32 color, u32 fill)
{
    window_frame_tex_draw(x, y, width, height, style, shade, color, fill, 3, 4, 5);
}

/* .data 0x805D5998 / 0x805D59B8 / 0x805D59E8: the uvs of the title window's tab strip, its three right-hand and
 * bottom corner quads, and its left/right edge quads. */
_mh_tex_uv_ title_tab_uvs[8] = {
    { 0, 0 }, { 0, 48 }, { 32, 0 }, { 32, 48 }, { 32, 0 }, { 32, 48 }, { 64, 0 }, { 64, 48 },
};
_mh_tex_uv_ title_corner_uvs[12] = {
    { 0, 0 }, { 32, 0 }, { 32, 32 }, { 0, 32 }, { 32, 32 }, { 0, 32 },
    { 0, 0 }, { 32, 0 }, { 0, 32 }, { 32, 32 }, { 32, 0 }, { 0, 0 },
};
_mh_tex_uv_ title_edge_y_uvs[8] = {
    { 0, 0 }, { 16, 0 }, { 16, 1 }, { 0, 1 }, { 16, 0 }, { 32, 0 }, { 32, 1 }, { 16, 1 },
};

/* .data 0x805D5A08 / 0x805D5A20 / 0x805D5A38 / 0x805D5A50: per title window kind, the texture translation of the
 * tab, the corners and the top/bottom edges, and the left/right edge texture offset. */
f32 title_tab_translate[3][2] = {
    { 0.166666672f, 0.6328125f }, { 0.338541657f, 0.6328125f }, { 0.338541657f, 0.6328125f },
};
f32 title_corner_translate[3][2] = {
    { 0.0885416642f, 0.5f }, { 0.0f, 0.5f }, { 0.0f, 0.5f },
};
f32 title_edge_x_translate[3][2] = {
    { 0.0f, 0.25f }, { 0.0f, 0.0f }, { 0.0f, 0.625f },
};
f32 title_edge_y_offset[3][2] = {
    { 0.25f, 0.0f }, { 0.0f, 0.0f }, { 0.375f, 0.0f },
};

/* 0x802E3718 (0x394): Draws a titled window (at least 176x64, rounded down to 16): the body with its title bar,
 * the title tab at the top-left, then the corners and edges in title kind `kind`. */
void draw_title_window(s16 x, s16 y, u16 width, u16 height, u8 kind, u8 shade, u32 color, u32 fill)
{
    f32 scale[2];
    _mh_tex_uv_ uvs[8];
    _mh_ivec2_ verts[12];

    width &= 0xFFF0;
    height &= 0xFFF0;
    if (width < 176) {
        width = 176;
    }
    if (height < 64) {
        height = 64;
    }
    window_body_draw(x, y, width, height, shade, fill, 1);

    s16 x32 = x + 32;
    s16 y32 = y + 32;
    s16 right32;
    s16 bottom32;
    s16 right = x + width;
    s16 bottom = y + height;
    s16 tab;

    right32 = right - 32;
    bottom32 = bottom - 32;
    tab = x + 192;

    verts[0].x = verts[1].x = x;
    verts[2].x = verts[3].x = x32;
    verts[4].x = verts[5].x = tab - 32;
    verts[6].x = verts[7].x = tab;
    verts[0].y = verts[2].y = verts[4].y = verts[6].y = y - 16;
    verts[1].y = verts[3].y = verts[5].y = verts[7].y = y32;
    drawshape_init(5, 8);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(5, title_tab_uvs);
    drawshape_set_tex_translate(title_tab_translate[kind]);
    drawshape_exec();

    verts[4].x = verts[7].x = x;
    verts[0].y = verts[1].y = y;
    verts[5].x = verts[6].x = x32;
    verts[2].y = verts[3].y = y32;
    verts[0].x = verts[3].x = verts[8].x = verts[11].x = right32;
    verts[4].y = verts[5].y = verts[8].y = verts[9].y = bottom32;
    verts[1].x = verts[2].x = verts[9].x = verts[10].x = right;
    verts[6].y = verts[7].y = verts[10].y = verts[11].y = bottom;
    drawshape_init(3, 12);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(5, title_corner_uvs);
    drawshape_set_tex_translate(title_corner_translate[kind]);
    drawshape_exec();

    verts[0].x = verts[3].x = tab;
    verts[4].x = verts[7].x = x32;
    verts[1].x = verts[2].x = verts[5].x = verts[6].x = right32;
    verts[0].y = verts[1].y = y;
    verts[2].y = verts[3].y = y + 16;
    verts[4].y = verts[5].y = bottom - 16;
    verts[6].y = verts[7].y = bottom;
    uvs[0].u = uvs[3].u = 160;
    uvs[4].u = uvs[7].u = 0;
    uvs[1].u = uvs[2].u = uvs[5].u = uvs[6].u = width - 64;
    uvs[0].v = uvs[1].v = 0;
    uvs[2].v = uvs[3].v = uvs[4].v = uvs[5].v = 16;
    uvs[6].v = uvs[7].v = 32;
    drawshape_init(3, 8);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(3, uvs);
    drawshape_set_tex_translate(title_edge_x_translate[kind]);
    drawshape_exec();

    verts[0].x = verts[3].x = x;
    verts[1].x = verts[2].x = x + 16;
    verts[4].x = verts[7].x = right - 16;
    verts[5].x = verts[6].x = right;
    verts[0].y = verts[1].y = verts[4].y = verts[5].y = y32;
    verts[2].y = verts[3].y = verts[6].y = verts[7].y = bottom32;
    scale[0] = 1.0f;
    scale[1] = height - 64;
    drawshape_init(3, 8);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(4, title_edge_y_uvs);
    drawshape_set_tex_scale(scale, title_edge_y_offset[kind]);
    drawshape_exec();
}

/* .data 0x805D5A68 / 0x805D5AA0: the uvs of the prompt frame's 14-vertex border strip, and its texture offset per
 * frame mode. */
_mh_tex_uv_ prompt_frame_uvs[14] = {
    { 0, 0 }, { 0, 64 }, { 32, 0 }, { 32, 64 }, { 32, 0 }, { 32, 64 }, { 48, 0 },
    { 48, 64 }, { 32, 0 }, { 32, 64 }, { 32, 0 }, { 32, 64 }, { 0, 0 }, { 0, 64 },
};
f32 prompt_frame_translate[2][2] = {
    { 0.0f, 0.0f }, { 0.0f, 0.25f },
};

/* 0x802E3AAC (0x26C): Draws a one-line prompt frame `width` wide (at least 96, rounded down to 16): a body strip of
 * texture `kind` (a flat colour for 0), then the border strip, notched in the middle, in frame `mode`. */
void draw_prompt_frame(s16 x, s16 y, u16 width, u8 mode, u8 kind)
{
    _mh_tex_uv_ uvs[4];
    _mh_ivec2_ verts[14];
    s32 edge;
    u32 color;

    y -= 8;
    width &= 0xFFF0;
    if (width < 96) {
        width = 96;
    }
    if (mode == 0) {
        edge = 22;
    } else {
        edge = 24;
    }
    s16 inner = width - edge;

    verts[0].x = verts[3].x = x + edge;
    verts[1].x = verts[2].x = x + inner;
    verts[0].y = verts[1].y = y + 20;
    verts[2].y = verts[3].y = y + 44;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_array(verts);
    if (kind == 0) {
        color = 0x1E1C0DB2;
    } else {
        if (kind == 1) {
            color = 0xFFFFFFFF;
        } else {
            color = 0x0A1407FF;
        }
        s16 height = window_body_strips[kind].height;

        uvs[0].u = uvs[3].u = 0;
        uvs[1].u = uvs[2].u = (s16)(height * (inner - edge) / 24);
        uvs[0].v = uvs[1].v = window_body_strips[kind].v;
        uvs[2].v = uvs[3].v = window_body_strips[kind].v + height;
        drawshape_set_texture_array(6, uvs);
    }
    drawshape_set_flat_color(color);
    drawshape_exec();

    s16 mid = x + width / 2 - 16;
    s16 right = x + width;

    verts[0].x = verts[1].x = x;
    verts[2].x = verts[3].x = x + 32;
    verts[4].x = verts[5].x = mid;
    verts[6].x = verts[7].x = mid + 16;
    verts[8].x = verts[9].x = mid + 32;
    verts[10].x = verts[11].x = right - 32;
    verts[12].x = verts[13].x = right;
    verts[0].y = verts[2].y = verts[4].y = verts[6].y = verts[8].y = verts[10].y = verts[12].y = y;
    verts[1].y = verts[3].y = verts[5].y = verts[7].y = verts[9].y = verts[11].y = verts[13].y = y + 64;
    drawshape_init(5, 14);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(0xFFFFFFFF);
    drawshape_set_texture_array(5, prompt_frame_uvs);
    drawshape_set_tex_translate(prompt_frame_translate[mode]);
    drawshape_exec();
}

/* .data 0x805D5AB0: the uvs of `draw_window_box`'s eight border quads (texture 5). */
_mh_tex_uv_ window_box_uvs[32] = {
    { 104, 72 }, { 128, 72 }, { 128, 96 }, { 104, 96 }, { 128, 72 }, { 128, 72 }, { 128, 96 }, { 128, 96 },
    { 128, 72 }, { 152, 72 }, { 152, 96 }, { 128, 96 }, { 104, 96 }, { 128, 96 }, { 128, 96 }, { 104, 96 },
    { 128, 96 }, { 152, 96 }, { 152, 96 }, { 128, 96 }, { 104, 96 }, { 128, 96 }, { 128, 120 }, { 104, 120 },
    { 128, 96 }, { 128, 96 }, { 128, 120 }, { 128, 120 }, { 128, 96 }, { 152, 96 }, { 152, 120 }, { 128, 120 },
};

/* 0x802E3D18 (0x210): Draws a window box: a `fill` rectangle (dark when 0) inside eight 24-pixel border quads in
 * `color` (brown when 0), at least 48x48. */
void draw_window_box(s16 x, s16 y, u16 width, u16 height, s32 color, s32 fill)
{
    DrawRect rect;
    _mh_ivec2_ verts[32];

    if (width < 48) {
        width = 48;
    }
    if (height < 48) {
        height = 48;
    }
    if (fill == 0) {
        fill = 0x1E1C0DB2;
    }
    draw_rect_set(&rect, x + 7, y + 10, width - 14, height - 20, fill);
    draw_rect_fill(&rect);

    s16 right = x + width;
    s16 bottom = y + height;

    verts[0].x = verts[3].x = verts[12].x = verts[15].x = verts[20].x = verts[23].x = x;
    verts[1].x = verts[2].x = verts[13].x = verts[14].x = verts[21].x = verts[22].x = verts[4].x = verts[7].x = verts[24].x = verts[27].x = x + 24;
    verts[5].x = verts[6].x = verts[25].x = verts[26].x = verts[8].x = verts[11].x = verts[16].x = verts[19].x = verts[28].x = verts[31].x = right - 24;
    verts[9].x = verts[10].x = verts[17].x = verts[18].x = verts[29].x = verts[30].x = right;
    verts[0].y = verts[1].y = verts[4].y = verts[5].y = verts[8].y = verts[9].y = y;
    verts[2].y = verts[3].y = verts[6].y = verts[7].y = verts[10].y = verts[11].y = verts[12].y = verts[13].y = verts[16].y = verts[17].y = y + 24;
    verts[14].y = verts[15].y = verts[19].y = verts[18].y = verts[20].y = verts[21].y = verts[24].y = verts[25].y = verts[28].y = verts[29].y = bottom - 24;
    verts[22].y = verts[23].y = verts[26].y = verts[27].y = verts[30].y = verts[31].y = bottom;
    if (color == 0) {
        color = 0x926239FF;
    }
    drawshape_init(3, 32);
    drawshape_set_vertex_array(verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array(5, window_box_uvs);
    drawshape_exec();
}

/* 0x802E3F28 (0x1E8): Draws a window's body inside its frame: a flat `fill` rectangle for `kind` 0, otherwise
 * horizontal strips of body texture `kind` (in batches of ten quads), plus the title bar under `title`. */
void window_body_draw(s16 x, s16 y, u16 width, u16 height, u8 kind, u32 fill, u8 title)
{
    _mh_ivec2_ verts[40];
    _mh_tex_uv_ uvs[40];
    DrawRect rect;
    DrawRect title_rect;

    x += 12;
    y += 8;
    width -= 24;
    height -= 16;
    if (kind == 0) {
        if (fill == 0) {
            fill = 0x1E1C0DB2;
        }
        draw_rect_set(&rect, x, y, width, height, fill);
        draw_rect_fill(&rect);
    } else {
        _mh_ivec2_* vert = verts;
        _mh_tex_uv_* uv = uvs;
        s16 strip = window_body_strips[kind].height;
        s16 right = x + width;
        s16 row = y;
        s16 v0 = window_body_strips[kind].v;
        s16 v1 = v0 + strip;
        s16 left_h = height;
        u16 count = 0;

        if (fill == 0) {
            fill = 0x0A1407FF;
        }
        for (;;) {
            for (;;) {
                if (left_h <= 0) {
                    break;
                }
                if (left_h < strip) {
                    strip = left_h;
                    v1 = v0 + left_h;
                }
                vert[3].x = x;
                vert[0].x = x;
                vert[2].x = right;
                vert[1].x = right;
                vert[1].y = row;
                vert[0].y = row;
                row += strip;
                vert[3].y = row;
                vert[2].y = row;
                vert += 4;
                uv[3].u = 0;
                uv[0].u = 0;
                uv[2].u = width;
                uv[1].u = width;
                uv[1].v = v0;
                uv[0].v = v0;
                uv[3].v = v1;
                uv[2].v = v1;
                uv += 4;
                left_h -= strip;
                count += 4;
                if (count >= 40) {
                    break;
                }
            }
            drawshape_init(3, count);
            drawshape_set_vertex_array(verts);
            drawshape_set_flat_color(fill);
            drawshape_set_texture_array(6, uvs);
            drawshape_exec();
            if (left_h <= 0) {
                break;
            }
            vert = verts;
            uv = uvs;
            count = 0;
        }
    }
    if (title != 0) {
        fill = (fill & 0xFFFFFF00) | 0xB2;
        draw_rect_set(&title_rect, x + 1, y - 16, 175, 16, fill);
        draw_rect_fill(&title_rect);
    }
}

/* 0x802E4110 (0x24): Draws a window frame in style 1 with no body fill colour. */
void draw_layout_window(s16 x, s16 y, u16 w, u16 h)
{
    draw_layout_window_style(x, y, w, h, 1, 0, 0xFFFFFFFF, 0);
}

/* 0x802E4134 (0x24): Draws a style-0 window frame whose body is filled with `color`. */
void draw_window_frame(s16 x, s16 y, u16 width, u16 height, u32 color)
{
    draw_layout_window_style(x, y, width, height, 0, 0, 0xFFFFFFFF, color);
}

/* 0x802E4158 (0x5C): Draws a style-0 window frame from the three given border textures. */
void put_frame_sub_tex(s16 x, s16 y, u16 width, u16 height, u32 fill, u16 edge_x_tex, u16 edge_y_tex,
                       u16 corner_tex)
{
    window_frame_tex_draw(x, y, width, height, 0, 0, 0xFFFFFFFF, fill, edge_x_tex, edge_y_tex, corner_tex);
}

/* 0x802E41B4 (0xBC): Draws `text` as one line inside a prompt frame sized to it. */
void note_line_draw(s16 x, s16 y, s16 size, char* text)
{
    set_blendmode(4, 5, 1);
    s32 width = (size * flfntStrLen(text)) / 2;
    x += width / 2;
    s32 box = (width + 79) & 0xFFF0;
    draw_prompt_frame(x - box / 2, y - 14, box, 1, 1);
}

/* The note box the cockpit's message window is laid out from: `note_text_max_len` measures its widest
 * line, the two anchor entries hand the box's position to the lobby, and the three extent helpers
 * offset a row by the line count the box is showing.  Names are derived from the behaviour (the
 * only caller-side evidence is `note_box_draw` in `enemy/em_prog_support.cpp`) - GUESS. */

/* 0x802E4270 (0xD0) - the length of the widest line of `str`, where a line ends at the first '\n'.
 * `size` is what the callers pass as the font size and is unused here (the font's own width
 * function accounts for it). */
s16 note_text_max_len(u32 size, char* str)
{
    char text[128];
    char rest[128];
    char* p;
    s32 wrapped;
    s32 first;
    s32 second;

    text[0] = 0;
    rest[0] = 0;
    strcpy(text, str);
    p = text;
    wrapped = 0;
    for (;;) {
        s32 len = utf82unicode2((u8*)p);

        if (len == 1) {
            if (p[0] == 0) {
                break;
            }
            if (p[0] == '\n') {
                wrapped = 1;
                break;
            }
        }
        p += len;
    }
    if (wrapped == 0) {
        return (s16)flfntStrLen(text);
    }
    *p = 0;
    strcpy(rest, p + 1);
    first = flfntStrLen(text);
    second = flfntStrLen(rest);
    if (second > first) {
        first = second;
    }
    return (s16)first;
}

/* 0x802E4340 (0x168): Draws `text` in a window box sized to its widest line, or as one prompt line when it has
 * no line break. */
void note_text_draw(s16 x, s16 y, s16 size, char* text)
{
    char line[128];
    char rest[128];
    char* p;
    s32 wrapped;
    s32 width;
    s32 half;
    s32 second;

    line[0] = 0;
    rest[0] = 0;
    strcpy(line, text);
    p = line;
    wrapped = 0;
    for (;;) {
        s32 len = utf82unicode2((u8*)p);

        if (len == 1) {
            if (p[0] == 0) {
                break;
            }
            if (p[0] == '\n') {
                wrapped = 1;
                break;
            }
        }
        p += len;
    }
    if (wrapped == 0) {
        note_line_draw(x, y, size, text);
        return;
    }
    *p = 0;
    strcpy(rest, p + 1);
    set_blendmode(4, 5, 1);
    half = size / 2;
    width = half * flfntStrLen(line);
    second = half * flfntStrLen(rest);
    if (second > width) {
        width = second;
    }
    x += (s16)(width / 2);
    s32 box = (width + 79) & 0xFFF0;
    draw_window_box(x - box / 2, y - 14, box, size * 4, 0x92623900, 0x0A1407FF);
}

/* 0x802E44A8 (0x44) - the upper note-box anchor. */
void note_box_anchor_upper(_mh_ivec2_* pos)
{
    pos->x = (s16)(get_wide_offset_s16(2) + 30);
    pos->y = 360;
}

/* 0x802E44EC (0x44) - the lower note-box anchor. */
void note_box_anchor_lower(_mh_ivec2_* pos)
{
    pos->x = (s16)(get_wide_offset_s16(2) + 30);
    pos->y = 386;
}

/* 0x802E4530 (0xA4): Draws the help window under the HUD, in the plain colours for `kind` 0 and the warning ones
 * otherwise. */
void draw_help_window(u8 kind)
{
    set_blendmode(4, 5, 1);
    s16 x = get_wide_offset_s16(2) + 12;
    if (kind == 0) {
        draw_title_window(x, 368, 400, 96, 1, 0, 0x785D42FF, 0xB2);
    } else {
        draw_title_window(x, 368, 400, 96, 2, 0, 0x8C4455FF, 0x261600B2);
    }
}

/* 0x802E45D4 (0x30) - the note box's own extent for `lines` rows: nothing until the fifth row,
 * then a negative height that grows by 16 per row. */
void note_box_size(_mh_ivec2_* extent, s16 lines)
{
    extent->x = 0;
    extent->y = 0;
    if (lines > 4) {
        s16 height = (lines - 4) * 16;

        height = -height;
        extent->y = height;
    }
}

/* 0x802E4604 (0x7C) - a note-box row's position: the 2953 sprite's own position is handed back
 * through `pos`, shifted by the row's own inset, and the box's height is added for the rows past
 * the fourth. */
void note_box_pos_upper(_mh_ivec2_* pos, s16 lines)
{
    get_lsp_data(2953, pos);
    pos->x += 18;
    pos->y -= 8;
    if (lines > 4) {
        pos->y -= (s16)((lines - 4) * 16);
    }
}

/* 0x802E4680 (0x7C) - the same for the rows below it. */
void note_box_pos_lower(_mh_ivec2_* pos, s16 lines)
{
    get_lsp_data(2953, pos);
    pos->x += 18;
    pos->y += 18;
    if (lines > 4) {
        pos->y -= (s16)((lines - 4) * 16);
    }
}

/* 0x802E46FC (0x9C): Draws the note box's title window at the note anchor, grown upwards by 16 per row past the
 * fourth of `lines`; `kind` is not read. */
void note_box_window_draw(u8 kind, s16 lines)
{
    _mh_ivec2_ pos;
    u16 height;

    set_blendmode(4, 5, 1);
    get_lsp_data(2953, &pos);
    if (lines > 4) {
        s32 extra = (lines - 4) * 16;

        pos.y -= (s16)extra;
        height = extra + 96;
    } else {
        height = 96;
    }
    draw_title_window(pos.x, pos.y, 400, height, 0, 6, 0x6E823CFF, 0);
}

/* One window-frame preset: the frame's size and the style and shade it is drawn in.  size: 0x8 */
typedef struct WindowFramePreset {
    /* +0x00 */ u16 width;
    /* +0x02 */ u16 height;
    /* +0x04 */ u16 style;
    /* +0x06 */ u16 shade;
} WindowFramePreset;

/* .data 0x805D5B30: the three window-frame presets `draw_window_frame_preset` picks from. */
WindowFramePreset window_frame_presets[3] = {
    { 256, 80, 4, 5 },
    { 400, 96, 0, 0 },
    { 304, 96, 0, 0 },
};

/* 0x802E4798 (0x90): Draws window frame preset `kind`'s style and shade over the given rectangle. */
void draw_window_frame_style(s16 x, s16 y, u16 width, u16 height, u8 kind)
{
    set_blendmode(4, 5, 1);
    draw_layout_window_style(x, y, width, height, window_frame_presets[kind].style,
                             window_frame_presets[kind].shade, 0xFFFFFFFF, 0);
}

/* 0x802E4828 (0x28): Draws window frame preset `flag` at its own size. */
void draw_window_frame_preset(s16 x, s16 y, s32 flag)
{
    u8 kind = flag;

    draw_window_frame_style(x, y, window_frame_presets[kind].width, window_frame_presets[kind].height, kind);
}

/* 0x802E4850 (0x128): Draws the waiting spinner at (`x`, `y`), pulsing its scale every 16 frames, in `color`
 * (red when 0). */
void draw_spin_icon(s16 x, s16 y, s32 color)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;
    _mh_ivec2_ center;
    f32 scale;

    center.x = x + 9;
    center.y = y + 9;
    if (system_w.field_0x05 == 0) {
        if ((system_w.field_0x0c & 31) < 16) {
            scale = 0.8f;
        } else {
            scale = 1.0f;
        }
    } else {
        scale = 1.0f;
    }
    uv0.u = 160;
    uv0.v = 109;
    uv1.u = 178;
    uv1.v = 127;
    if (color == 0) {
        color = 0xE02020FF;
    }
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, 18, 18);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(5, &uv0, &uv1);
    drawshape_set_rot_scale_ivec2(&center.x, 0x4000, scale);
    drawshape_exec();
}
