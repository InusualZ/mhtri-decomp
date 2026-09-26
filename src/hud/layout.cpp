/* hud/layout.cpp - the HUD's 2D element library (the 88-function `layout.cpp` translation unit,
 * `.text` 0x802E0740..0x802E4978 / 0x4238 B).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for most of this range (checked with
 * tools/symbols/symedit.py --section .text range 0x802E0740 0x802E4978: 68 of the 88 rows are bare
 * `fn_` stems; the other 20 carry real names the runtime dump confirms, e.g. draw_sprite,
 * draw_font_idx, put_button_icon_tex, color_mult, and those are written as real functions here).
 * The dump labels the rest `zz_02exxxx_`.
 *
 * Home, name and seam, from evidence (docs/plan.md 12, brief section 2):
 *   - class 1, a `__FILE__` string.  `.data` 0x805D5800, 0xB = "layout.cpp", is referenced by
 *     `fn_802E2440` (0x802E2464: `lis`/`addi` of the string) and `fn_802E2524` (0x802E2564), both
 *     in this range, and by nobody else; the dump's local symbol for it is
 *     `s_layout.cpp_805d5800`.  So the file is `layout.cpp` (the `.cpp` suffix is the string's).
 *   - the seam.  `tudiscover.py at 0x802E0740` must-links this range (0x802E0740..0x802E4978, 88
 *     functions, anchor `lbl_8079A8C8`, a private `.sdata2` pool label referenced by both ends of
 *     the run), and the data fragments tile exactly at both ends: this unit's `.data`
 *     0x805D5798..0x805D5B48 is bracketed by the cockpit range's last label (0x805D5740+0x58 =
 *     0x805D5798, playbook 29) and by `jumptable_805D5B48`, whose only referrer is `fn_802E4C5C` in
 *     the 0x802E4978 proposal (playbook 53: the table's own range, never the band around it); its
 *     `.sdata2` 0x8079A8C4..0x8079A8E0 is bracketed by 0x8079A8C0 (0.5f, the cockpit range) and
 *     0x8079A8E0 (`fn_802E4C5C`); its `.sdata` 0x807927B0..0x807927BA by 0x807927C0
 *     (`fn_802E5E90`, cockpit_quest.cpp).  The proposal's own cut at 0x802DDC04 is the *edge of the
 *     cockpit range's must-link anchor*, not a TU boundary: the 61 functions below this one belong
 *     to `cockpit.cpp` (0x802D9EB4..0x802E0740), which `ai/fn_802D44F4.cpp` currently owns up to
 *     0x802DDC04 - filed as a `range` config request, not registered here.
 *   - module `hud`: this is the 2D element drawing library the HUD uses, `src/hud/fn_80324F7C.c`
 *     (the registered `hud` unit) calls `get_lsp_data`/`draw_sprite_ary` and carries the same
 *     `_mh_ivec2_`, and the lobby screens call it directly; the neighbouring *address* band is `ai`
 *     (`ai/fn_802D44F4.cpp`), which is a link-order accident, not evidence about this file.
 *
 * Sections this unit **owns**: .text 0x802E0740..0x802E4978, extab 0x80014DAC..0x80014FFC (74 8-byte
 * records, one per framed function) and extabindex 0x800332E8..0x80033660 (74 12-byte records - both
 * runs are exactly the count of the range's framed functions and tile record by record).  The lane
 * that wrote this file also claimed `.data` 0x805D5798..0x805D5B48, `.sdata` 0x807927B0..0x807927C0
 * and `.sdata2` 0x8079A8C4..0x8079A8E0; the landing dropped all three:
 *   - **a partial `.sdata2` claim is not linkable.**  With it, `ninja build/RMHE08/ok` dies in
 *     `mwldeppc.exe` with `internal linker error: File: 'ELF_gen.c' Line 2802` - reproduced twice
 *     with a forced re-split, and green without it (playbook 23's dropped `R_PPC_NONE` pool relocs).
 *     The target's pool run is 28 B and our object's is 16 B: the pool words are *declared* here, never
 *     defined (playbook 29), so the claim has to wait until every word of the run is written.
 *   - **`.sdata` was over-claimed.**  This header's own evidence ends the run at 0x807927BA and gives
 *     the next 6 B to `cockpit_quest.cpp`'s `fn_802E5E90`; `splits.txt` said ..0x807927C0.
 *   - `.data` 0x805D5798..0x805D5B48 *does* link and the tables are this unit's (the `draw_*` family
 *     and both `__FILE__` referrers are inside the range), but with 41 of 88 bodies written the claim
 *     buys nothing yet.  Re-claim `.data` and `.sdata` (10 B), and `.sdata2` once the sizes agree.
 *
 * Playbook 29: the pool words and label tables this range references are **declared** here and never
 * defined - that is what keeps the extab/.text claims linkable and lets the unclaimed data go to its
 * own unit.  Playbook 55: our object's `.sdata2` starts 4-mod-8 (its own 16-byte pool), so a claim of
 * the run needs `sh_addralign` lowered to 4 before a flip - `tools/elf/objalign.py` does that from the
 * claim, and `objalign.py`/`flipcheck.py` are the two checks to run then.
 *
 * Flags: `cflags_main` (Wii/1.3, `-O3`, `-inline noauto`, `-Cpp_exceptions on`) - the range keeps
 * `bl`s to its own tiny helpers (`fn_802E0DA8` -> `fn_802E0CE4`), which is `-inline noauto`, and it
 * carries the 74 extab records `-Cpp_exceptions on` emits.
 *
 * Score at this commit: 31.5462 % fuzzy (3796/16952 B of `.text`), 41 of the 88 bodies at or above the
 * 80 % bar and 33 of them byte-identical; the 47 functions not written yet score 0 and dominate the unit
 * percentage.  Measured with `build/tools/objdiff-cli.exe report generate -p .` on this worktree's own
 * objects (`units[name = "main/hud/layout"]`); `python tools/units/recompile.py hud/layout.cpp --measure
 * <symbol>` gives the same number per symbol.  Residuals: `fn_802E0B54` 61.5 % (the kind switch),
 * `fn_802E0F78` 91.0 %, `fn_802E0A24` 93.9 %, `put_button_icon_tex` 97.9 %, `fn_802E099C` 98.1 %,
 * `fn_802E0C80` 99.4 %, `draw_sprite` 99.96 % (retail materialises the 0x4330 conversion high word twice;
 * MWCC CSEs ours, 4 bytes short).
 */

#include "types.h"
#include "hud/layout.h"
#include "menu/menu_item.h"

/* Retail keeps the unfused peephole pairs (`clrlwi`+`cmpwi`, `clrlwi`+`beq`) that the peephole pass
 * fuses into the record form (`clrlwi.`): `fn_802E0CE4`'s colour gate is the smallest witness
 * (0x802E0D5C `clrlwi r0,r0,24` + 0x802E0D60 `cmpwi r0,0x0` where our build emits one `clrlwi.`),
 * and the same pair is what `src/hud/fn_80324F7C.c` (the other `hud` lib unit) carries in its own
 * source as the same finding.  Carried as a pragma until the lib flag below lands - playbook 33:
 * the flag is the preferred home, and it is a `config_request` of kind `flag` against `cflags_hud`,
 * which is why it is not applied to the whole lib from here. */
#pragma peephole off

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own `.data` tables and `.sdata2` pool, declared and never defined (playbook 29).
 * `lbl_805D5798` is a *sized array* on purpose: that is what makes MWCC address it `lis`/`addi`
 * like the target's `lis r4, lbl_805D5798@ha; addi r31, r4, lbl_805D5798@l` (a scalar would come
 * out `@sda21`). */
extern const _mh_tex_uv_ lbl_805D5798[]; /* .data 0x805D5798, 0x20 B of uv pairs */
extern const _mh_tex_uv_ lbl_807927B0[]; /* .sdata 0x807927B0, two uv pairs */
extern const u8 lbl_805D57B8[];          /* .data 0x805D57B8, the equip-kind -> texture table */
extern const u8 lbl_805D57C8[];          /* .data 0x805D57C8, the player-type -> texture table */
extern const s8 lbl_805D57D4[];          /* .data 0x805D57D4, the monster texture table (-1 = none) */
extern const f32 lbl_8079A8C4;           /* .sdata2 0x8079A8C4 = 100.0f, the uv scale divisor */
extern const f32 lbl_8079A8D8;           /* .sdata2 0x8079A8D8 = 1.0f */
extern const f32 lbl_8079A8DC;           /* .sdata2 0x8079A8DC = 0.8f */

#ifdef __cplusplus
}
#endif

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

    fn_800526F8(&anchor, &spr.pos);
    fn_800526E4(&uv0, &spr.uv0);
    fn_800526E4(&uv1, &spr.uv1);
    if (pos != 0) {
        anchor.x += pos->x;
        anchor.y += pos->y;
    }
    posf[0] = (f32)anchor.x;
    posf[1] = (f32)anchor.y;
    anchor.x += spr.ofs_x;
    anchor.y += spr.ofs_y;
    scale[0] = (f32)spr.u_scale / lbl_8079A8C4;
    scale[1] = (f32)spr.v_scale / lbl_8079A8C4;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(anchor.x, anchor.y, spr.width, spr.height);
    drawshape_set_flat_color(spr.color);
    drawshape_set_texture_rect(spr.tex_flag, &uv0, &uv1);
    fn_80053E78(scale, posf, spr.tex_id);
    drawshape_exec();
}

/* 0x802E08AC (0x3C) - blit the sprite record the id names, at `pos`. */
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos)
{
    draw_sprite(*get_lsp_data(id, 0), pos);
}

/* 0x802E08E8 (0x5C) - the same, with the record's colour word replaced by `color`. */
void fn_802E08E8(u16 id, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ rec;

    fn_801E6850(&rec, get_lsp_data(id, 0));
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
 * `fn_802E29F4`/`fn_802E2C08`/`fn_802E2D84`/`fn_802E2F54`/`fn_802E326C` family member.  Returns 0
 * for a kind it does not know. */
u32 fn_802E099C(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame)
{
    switch (anim->kind) {
    case 0:
        return fn_802E29F4(rec, anim, frame);
    case 1:
        return fn_802E2C08(&rec->tex_id, anim, frame);
    case 2:
        return fn_802E2D84(&rec->u_scale, anim, frame);
    case 3:
        return fn_802E2F54(&rec->uv0, &rec->uv1, anim, frame);
    case 4:
        return fn_802E326C(&rec->color, anim, frame);
    }
    return 0;
}

/* 0x802E0A24 (0xB0) - walk up to five animation steps of `anim`, stopping at the first whose frame
 * count has run out, and report whether any step drew.  `out` (optional) receives the record's
 * anchor, widened by `get_wide_offset(rec->wide_idx)`. */
u32 fn_802E0A24(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame, _mh_ivec2_* out)
{
    u32 drawn = 0;
    s32 i;

    if (anim != 0) {
        for (i = 5; i > 0; i--) {
            if (anim->count <= 0) {
                break;
            }
            if (fn_802E099C(rec, anim, frame) == 1) {
                drawn = 1;
            }
            anim++;
        }
    }
    if (out != 0) {
        out->x = rec->pos.x + get_wide_offset(rec->wide_idx);
        out->y = rec->pos.y;
    }
    return drawn;
}

/* 0x802E0AD4 (0x80) - copy the id's record into the caller's block, then run the id's animation
 * table against it with `part` as the frame. */
u32 fn_802E0AD4(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out)
{
    fn_801E6850(rec, get_lsp_data(id, 0));
    return fn_802E0A24(rec, fn_802E0714(id), part, out);
}

/* 0x802E0B54 (0x12C) - the highest frame value the id's animation table reaches (1 when the table
 * is absent, which is the smallest a caller can use).  Each step's frame count indexes its own
 * table, whose stride depends on the step's kind: 2 halfwords for kinds 0/2/4, 3 for kind 1 and 6
 * for kind 3. */
u16 fn_802E0B54(u16 id)
{
    const _SPR_ANIM_* anim = (const _SPR_ANIM_*)fn_802E0714(id);
    u16 highest = 1;
    s32 i;

    if (anim == 0) {
        return highest;
    }
    for (i = 5; i != 0; i--) {
        u16 value;

        if (anim->count <= 0) {
            break;
        }
        switch (anim->kind) {
        case 0:
        case 2:
        case 4:
            value = anim->frames[(anim->count - 1) * 4];
            break;
        case 1:
            value = anim->frames[(anim->count - 1) * 3];
            break;
        case 3:
            value = anim->frames[(anim->count - 1) * 6];
            break;
        default:
            value = 0;
            break;
        }
        if (value > highest) {
            highest = value;
        }
    }
    return highest;
}

/* 0x802E0C80 (0x64) - the highest frame value over the 0xFFFF-terminated id run. */
u16 fn_802E0C80(const u16* ids)
{
    u16 highest = 1;

    for (; *ids != 0xFFFF; ids++) {
        u16 value = fn_802E0B54(*ids);

        if (value > highest) {
            highest = value;
        }
    }
    return highest;
}

/* 0x802E0CE4 (0xC4) - run `part` of the animation table against a *copy* of the record and blit
 * the copy when the step drew something and the copy still carries a colour and a uv scale. */
u32 fn_802E0CE4(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 part, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;
    u32 drawn;

    drawn = fn_802E0A24(&work, anim, part, 0);
    if ((work.color & 0xFF) != 0 && work.u_scale != 0 && work.v_scale != 0) {
        draw_sprite(work, pos);
    }
    return drawn;
}

/* 0x802E0DA8 (0x14) - the same from the record's own animation table. */
u32 fn_802E0DA8(_SPR_DATA_* rec, u16 part, const _mh_ivec2_* pos)
{
    return fn_802E0CE4(rec, rec->anim, part, pos);
}

/* 0x802E0DBC (0x50) - blit frame `anim` of the id's animation table. */
void draw_sprite_anim_idx(u16 id, u16 anim, const _mh_ivec2_* pos)
{
    _SPR_DATA_* rec = get_lsp_data(id, 0);

    fn_802E0CE4(rec, rec->anim, anim, pos);
}

/* 0x802E0E0C (0x88) - the same over the 0xFFFF-terminated id run, reporting whether any of them
 * drew. */
u32 draw_sprite_anim_ary(const u16* ids, u16 anim, const _mh_ivec2_* pos)
{
    u32 drawn = 0;

    for (; *ids != 0xFFFF; ids++) {
        _SPR_DATA_* rec = get_lsp_data(*ids, 0);

        if (fn_802E0CE4(rec, rec->anim, anim, pos) != 0) {
            drawn = 1;
        }
    }
    return drawn;
}

/* 0x802E0E94 (0xC8) - draw texture `tex_idx` of the button-icon uv table as a square of side
 * `size`, widened along x by `get_wide_offset(wide_idx)`. */
void put_button_icon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u8 wide_idx, u16 tex_flag)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;
    u32 index;

    x += get_wide_offset(wide_idx);
    index = (u32)tex_idx * 2;
    fn_800526E4(&uv0, &lbl_805D5798[index]);
    fn_800526E4(&uv1, &lbl_805D5798[index + 1]);
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

/* 0x802E0F78 (0xAC) - a rectangle filled with the fixed `.sdata` uv pair at scale 8. */
void fn_802E0F78(s16 x, s16 y, s16 w, s16 h, u32 color, u8 wide_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    x += get_wide_offset(wide_idx);
    fn_800526E4(&uv0, &lbl_807927B0[0]);
    fn_800526E4(&uv1, &lbl_807927B0[1]);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, w, h);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(8, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1024 (0xAC) - blit a *copy* of the record with texture `tex_idx` and the colour
 * multiplied by `color`. */
void fn_802E1024(_SPR_DATA_* rec, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;

    fn_80055C5C(&work.uv0, &work.uv1, tex_idx);
    work.color = color_mult(work.color, color);
    draw_sprite(work, pos);
}

/* 0x802E10D0 (0x64) - the item icon: the item record's texture index and its kind's colour-table
 * entry feed the copy helper. */
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(id);

    fn_802E1024((_SPR_DATA_*)&spr, item->tex_idx_0x004, lbl_805CDE78[item->kind], pos);
}

/* 0x802E1134 (0x5C) - the same from the sprite id. */
void fn_802E1134(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    fn_802E1024(get_lsp_data(id, 0), tex_idx, color, pos);
}

/* 0x802E1190 (0x64) - the item icon drawn with an arbitrary sprite id. */
void fn_802E1190(u16 id, u16 item_id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(item_id);

    fn_802E1134(id, item->tex_idx_0x004, lbl_805CDE78[item->kind], pos);
}

/* 0x802E11F4 (0x94) - a square filled with texture `tex_idx` of the item uv table. */
void put_itemicon_tex(s16 x, s16 y, s16 size, u32 color, u8 tex_idx, u16 tex_flag)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    fn_80055C5C(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(tex_flag, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1288 (0x18) - the same with the standard texture flag. */
void fn_802E1288(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    put_itemicon_tex(x, y, size, color, tex_idx, 0);
}

/* 0x802E12A0 (0x80) - the animation-step blit with a texture override: the record is built by
 * `fn_802E0AD4`, then handed to the copy-and-recolour helper. */
u32 fn_802E12A0(u16 id, u16 part, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = fn_802E0AD4(&work, id, part, 0);

    fn_802E1024(&work, tex_idx, color, pos);
    return drawn;
}

/* 0x802E1320 (0x74) - the same keyed by the item record. */
void fn_802E1320(u16 id, u16 part, u16 item_id, const _mh_ivec2_* pos)
{
    ItemDataRecord* item = GetItemData(item_id);

    fn_802E12A0(id, part, item->tex_idx_0x004, lbl_805CDE78[item->kind], pos);
}

/* 0x802E1394 (0x6C) - the equipment icon: nothing is drawn for an empty slot. */
void draw_equipicon(const _SPR_DATA_& spr, _EQUIP* equip, const _mh_ivec2_* pos)
{
    if (equip->kind != 0) {
        fn_802E1400((_SPR_DATA_*)&spr, equip, pos, Get_equip_rare(equip));
    }
}

/* 0x802E1400 (0xCC) - the equipment icon with the rarity colour: a gunner-only slot the player
 * cannot use falls back to the weapon-icon path, everything else uses the kind's texture. */
void fn_802E1400(_SPR_DATA_* spr, _EQUIP* equip, const _mh_ivec2_* pos, u8 rare)
{
    if (equip->kind != 0) {
        if (equip->kind == 0xB && !Gunner_opt_ok_ck(equip)) {
            fn_802E1518(spr, Get_pl_type(equip, 0), get_rare_color(rare), pos);
        } else {
            fn_802E1024(spr, lbl_805D57B8[equip->kind], get_rare_color(rare), pos);
        }
    }
}

/* 0x802E14CC (0x4C) - the equipment icon for a sprite id. */
void fn_802E14CC(u16 id, _EQUIP* equip, const _mh_ivec2_* pos)
{
    draw_equipicon(*get_lsp_data(id, 0), equip, pos);
}

/* 0x802E1518 (0x14) - the weapon icon: the player type selects the texture out of a byte table. */
void fn_802E1518(_SPR_DATA_* spr, u8 kind, u32 color, const _mh_ivec2_* pos)
{
    fn_802E1024(spr, lbl_805D57C8[kind], color, pos);
}

/* 0x802E152C (0x5C) - the same for a sprite id. */
void draw_weaponicon_idx(u16 id, u8 kind, u32 color, const _mh_ivec2_* pos)
{
    fn_802E1518(get_lsp_data(id, 0), kind, color, pos);
}

/* 0x802E1588 (0xB4) - the copy with texture `tex_idx` of the *colour* uv table and a colour word. */
void fn_802E1588(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    fn_80055D8C(&work.uv0, &work.uv1, tex_idx);
    work.color = color;
    draw_sprite(work, pos);
}

/* 0x802E163C (0xA4) - the same without the colour override. */
void fn_802E163C(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    fn_80055D20(&work.uv0, &work.uv1, tex_idx);
    draw_sprite(work, pos);
}

/* 0x802E16E0 (0xB4) - the same with it. */
void fn_802E16E0(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    fn_80055D20(&work.uv0, &work.uv1, tex_idx);
    work.color = color;
    draw_sprite(work, pos);
}

/* 0x802E1794 (0xA0) - a square of `_SPR_DATA_` uv pairs from the `fn_80055D20` table at flag 1. */
void fn_802E1794(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    fn_80055D20(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(1, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E1834 (0xA0) - the same at texture flag 0x1E. */
void fn_802E1834(s16 x, s16 y, s16 size, u32 color, u8 tex_idx)
{
    _mh_tex_uv_ uv0;
    _mh_tex_uv_ uv1;

    fn_80055D20(&uv0, &uv1, tex_idx);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, size, size);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(0x1E, &uv0, &uv1);
    drawshape_exec();
}

/* 0x802E18D4 (0xA4) - the sprite record with a `fn_80055CC4` uv pair. */
void fn_802E18D4(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    fn_80055CC4(&work.uv0, &work.uv1, tex_idx);
    draw_sprite(work, pos);
}

/* 0x802E1978 (0x14) - the monster texture index a kind byte maps to; negative means "no icon". */
s8 fn_802E1978(u8 index)
{
    return lbl_805D57D4[index];
}

/* 0x802E198C (0xA4) - the sprite record with a `fn_80055DC8` uv pair, skipped when the kind has no
 * texture in the table (the `-1` the byte-table helper answers). */
void fn_802E198C(_SPR_DATA_* rec, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *rec;

    if (fn_802E1978(tex_idx) >= 0) {
        fn_80055DC8(&work.uv0, &work.uv1, tex_idx);
        draw_sprite(work, pos);
    }
}

/* 0x802E1A30 (0x4C) - the monster icon for a sprite id. */
void draw_monstericon_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
{
    fn_802E198C(get_lsp_data(id, 0), tex_idx, pos);
}

/* 0x802E1A7C (0x70) - the monster icon on an animation-built record. */
u32 fn_802E1A7C(u16 id, u16 part, u8 tex_idx, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = fn_802E0AD4(&work, id, part, 0);

    fn_802E198C(&work, tex_idx, pos);
    return drawn;
}

/* 0x802E1AEC (0xAC) - the record with its u coordinates scrolled one frame further along the span
 * `uv0.u`..`uv1.u`, and an optional colour override. */
void fn_802E1AEC(_SPR_DATA_* rec, u8 frame, u32 color, const _mh_ivec2_* pos)
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
    fn_802E1AEC(get_lsp_data(id, 0), frame, color, pos);
}

/* 0x802E1BF4 (0x80) - the same on an animation-built record. */
u32 fn_802E1BF4(u16 id, u16 part, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = fn_802E0AD4(&work, id, part, 0);

    fn_802E1AEC(&work, frame, color, pos);
    return drawn;
}

/* 0x802E1D30 (0x5C) - the same for a sprite id. */
void fn_802E1D30(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    fn_802E1C74(get_lsp_data(id, 0), frame, color, pos);
}
