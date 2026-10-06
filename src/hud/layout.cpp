/* hud/layout.cpp - the HUD's 2D element library (the 88-function `layout.cpp` translation unit,
 * `.text` 0x802E0740..0x802E4978 / 0x4238 B).
 *
 * Naming note: the symbol map had only fn_XXXXXXXX for most of this range; this run named every row it
 * wrote and the rows they call inside the unit (see "Renames" below).  The rows still carrying a
 * `fn_` stem are the ones with no body yet, plus the pre-existing debt (`fn_802E1978`,
 * `fn_802E198C`, `fn_802E0A24`, `fn_802E0B54`, `sprite_ary_last_frame`, `fn_802E0CE4`, `fn_802E0DA8`,
 * `fn_802E08E8`, `fn_802E099C`, `sprite_frame_apply`, `fn_802E0F78`, `fn_802E1AEC`, `fn_802E1C74`,
 * `fn_802E1D30`, `fn_802E0714`) - filed as a naming sweep, not fixed here because each needs its
 * own evidence and the two that are called from outside this unit are cross-unit sweeps.
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
 * flipcheck: **NOT READY**. Three of its reasons are references that use the plain spelling where the
 * map carries the mangling - `get_lsp_data` (`get_lsp_data__FUsP10_mh_ivec2_`), `get_rare_color`
 * (`get_rare_color__FUc`) and `get_wide_offset` (`get_wide_offset__FUc`), the cockpit-side getters the
 * block of `hud/layout.h` declares - so a flip's link answers `undefined: 'get_lsp_data'`.
 * Pre-existing and unchanged by this run: the base tree returns the same three names, and the note-box
 * bodies add two more *sites*, not a new name.  Not a source defect to fix here (the names are the ones
 * the map and the target object use); the fix is the owner pass that gives the cockpit band
 * (0x802D9EB4..0x802E0740) its own unit, which moves the three declarations to that unit's header.  The
 * remaining lines flipcheck prints - `.text` 0x2458 of the 0x4238 claimed, extab/extabindex short,
 * 14697 of 16952 bytes differing from the target - are what a 46-of-88-body unit must show.
 *
 * Playbook 29: the pool words and label tables this range references are **declared** here and never
 * defined - that is what keeps the extab/.text claims linkable and lets the unclaimed data go to its
 * own unit.  Playbook 55: our object's `.sdata2` starts 4-mod-8 (its own 16-byte pool), so a claim of
 * the run needs `sh_addralign` lowered to 4 before a flip - `tools/elf/objalign.py` does that from the
 * claim, and `objalign.py`/`flipcheck.py` are the two checks to run then.
 *
 * Flags: `cflags_hud` (Wii/1.3, `-O3`, `-inline noauto`, `-Cpp_exceptions on`, `-opt nopeephole`) - the
 * range keeps `bl`s to its own tiny helpers (`fn_802E0DA8` -> `fn_802E0CE4`), which is `-inline noauto`,
 * and it carries the 74 extab records `-Cpp_exceptions on` emits; the `nopeephole` half is the lib's
 * (this file's own `fmuls`+`fadds` pairs are the `#pragma fp_contract off` below, not a lib flag).
 *
 * Score at this commit: 53.8504 % fuzzy (5444/16952 B of `.text`), 46 of the 88 rows at 100 % and 42
 * still open (26 of them with no body at all).  Measured with `python tools/objdiff/unitscore.py
 * hud/layout --measure` on this worktree's own objects (`units[name = "main/hud/layout"]`).
 *
 * Written this run: the decimal-digit family (`draw_number_digits_ary`, `draw_number_anim_ary`,
 * `draw_number_2digit_ary`, `draw_number_2digit_anim_ary`, `draw_font`, `draw_font_idx`), the
 * colour helpers (`color_lerp`, `color_mult`, `color_scale`), three of the five animation steps
 * (`anim_step_tex_id`, `anim_step_uv`, `anim_step_color`), `anim_frame_wrap` and the six note-box
 * layout entries - all byte-identical but `note_box_size` (91.67 %, 52 vs 48 B: MWCC re-narrows the
 * negated `s16` before the `sth`; the store's `extsh` is the one extra instruction, and four source
 * shapes that keep it out of the target - a cast, a named `s16` local, an assign-back, a `(u16)`
 * cast - all reproduce it).
 *
 * Two corrections the target's own extractions forced, both measured:
 *   - `_SPR_ANIM_` is **not** a bitfield pair.  The step kind is the low 12 bits of the halfword
 *     (`fn_802E099C`/`fn_802E0B54` both read it as `clrlwi r0,r0,20`) and the "wrap the frame at the
 *     table's last key" flag is bit 15 (`rlwinm r0,r0,0,16,16`) - MSB-first bitfields gave
 *     `extrwi 12,16` / `extrwi 1,28`, which is a *different* field.  As a plain `u16 flags` with the
 *     two masks: `fn_802E099C` 98.12 -> 99.88 %, `fn_802E0B54` 61.49 -> 62.29 %.
 *   - the unit needs `#pragma fp_contract off` (scoped to this file): retail keeps every `a*b + c` as
 *     its own `fmuls` + `fadds`, and with the default `-fp_contract on` `anim_step_uv` measured
 *     85.38 % and `color_lerp` 87.17 % against 98.91 % / 95.99 % with it off.
 *
 * Renames this run (rule 7; every referrer swept in the same change, all four verified by
 * `symedit.py rename-batch`'s reference scan): `fn_802E1BF4` -> `draw_number_anim`,
 * `fn_802E1D8C`/`fn_802E1E4C`/`fn_802E1F18`/`fn_802E1FC4` -> the `draw_number_*_ary` family,
 * `fn_802E26EC` -> `anim_frame_wrap`, `fn_802E270C` -> `color_lerp` (9 referrer lines across
 * `hud/cockpit_quest.cpp`, `menu/fn_802E4978.cpp`, `menu/menu_item_page.cpp` and their three
 * headers), `fn_802E28B0` -> `color_scale`, `fn_802E2C08`/`fn_802E2F54`/`fn_802E326C` -> the
 * `anim_step_*` family, and the six note-box rows.  `font/flfnt.h` declares the two
 * string helpers this range owns (`flfntStrLen`, `utf82unicode2`) - the owner's header is their
 * home - and `strcpy` comes from `unsplit/Runtime.PPCEABI.H.h` (0x8045F554 is owned by no
 * registered range, so the band header is its home).
 *
 * Still blocked on a **cross-unit rename** (filed, not half-done - each is another registered
 * unit's symbol, so the sweep is theirs to take): `fn_802E1C74` and `fn_802E29F4` call
 * `uv_pair_copy` (`fn_8004CAD8.cpp`, 16 referrer lines in 5 files, 66 call sites in the DOL);
 * `fn_802E2D84` calls `fn_802A2550` (`menu/menu_item.cpp`, 3 referrer lines); the two font-order
 * entries call `menu_text_center_x`/`menu_text_center_x_by_gaps`/`menu_text_block_center_x`
 * (`menu/menu_message.cpp`, already renamed there; 11 call sites) and
 * `fn_802E21C0` also `fn_8005C8F0` (`font/flfnt.cpp`, 4 call sites); `draw_font_anim_idx` calls
 * `sprite_frame_apply` (this unit's own row, but 44 referrer lines across 8 files).  `fn_802E2440`,
 * `fn_802E2524`, `draw_window_frame_style`/`fn_802E4828`, the four 0x802E33B4-family layout entries and
 * `fn_802E4850` are blocked on **data** (the `lbl_805D58xx`/`lbl_805D5Axx` tables and the `__FILE__`
 * and panic strings), i.e. on the `.data`/`.sdata` claim below.
 *
 * Residuals of the rows that are written: `fn_802E0B54` 62.3 % (the kind switch's arm order),
 * `note_box_size` 91.67 %, `color_lerp` 95.99 % (the four channel extractions' declaration order and
 * the pack's OR order), `color_scale` 97.04 % (the pack's OR order), `anim_step_tex_id` 97.37 %
 * (`cur`/`prev` land in the opposite register pair), `anim_step_uv` 98.91 %, `anim_step_color`
 * 98.96 %, plus the pre-existing `fn_802E1978` 79.0 %, `fn_802E198C` 89.98 %, `fn_802E0F78` 91.05 %,
 * `fn_802E0A24` 93.86 %, `fn_802E1D30` 97.39 %, `put_button_icon_tex` 97.9 %,
 * `fn_802E099C` 99.88 % (the switch's arm order), `sprite_ary_last_frame` 99.4 % and `draw_sprite` 99.96 %
 * (retail materialises the 0x4330 conversion high word twice; MWCC CSEs ours, 4 bytes short).
 *
 * Data: `datagap.py --unit hud/layout` reports `ours-extra .sdata2 20B` - the compiler's own pool
 * (the two 2^52 conversion doubles plus the `1.0f` `color_lerp` subtracts).  It grew by 4 B this run
 * because a new body cannot reference the `lbl_8079A8D8` word without a rule-7 finding of its own;
 * the pool claim stays blocked until every word of the 28-byte run is written, per the note above.
 * NAMES. GUESS: `draw_itemicon_anim_idx`, `draw_equipicon_idx`, `draw_sprite_uv_color_idx`, `draw_sprite_uv_idx`
 */

#include "types.h"
#include "hud/layout.h"
#include "menu/menu_item.h"
#include "font/flfnt.h"
#include "unsplit/Runtime.PPCEABI.H.h"

/* Retail keeps every `a*b + c` as its own `fmuls` + `fadds`: the animation-step interpolation and
 * the colour blends below are the witnesses (`fn_802E2F54`'s four interpolated halves).  The unit's
 * `cflags` do not carry the switch, so it is scoped to this file - the whole file needs it, and no
 * body here has a fused pair retail does not. */
#pragma fp_contract off

/* `color_lerp` (0x802E270C) is declared here rather than in `hud/layout.h`: three consumer headers
 * declare the same name with their own signatures, so the owner's header would be a C++ overload
 * clash for every consumer that includes two of them (see the note in the header).  The declaration
 * is this TU's own - it owns the address - and `extern "C"` is what gives the definition the
 * unmangled name the map and objdiff pair by. */
extern "C" u32 color_lerp(u32 a, u32 b, f32 t);

/* Retail keeps the unfused peephole pairs (`clrlwi`+`cmpwi`, `clrlwi`+`beq`) that the peephole pass
 * fuses into the record form (`clrlwi.`): `fn_802E0CE4`'s colour gate is the smallest witness
 * (0x802E0D5C `clrlwi r0,r0,24` + 0x802E0D60 `cmpwi r0,0x0` where our build emits one `clrlwi.`),
 * and the same pair is what `src/hud/fn_80324F7C.c` (the other `hud` lib unit) carries: the lib flag
 * `cflags_hud = cflags_main + -opt nopeephole` is the preferred home (playbook 33) and is applied there
 * as of 2026-09-27, so this file no longer carries a `#pragma peephole off`. */

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

    uv_pair_copy(&anchor, &spr.pos);
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
 * `fn_802E29F4`/`anim_step_tex_id`/`fn_802E2D84`/`anim_step_uv`/`anim_step_color` family member.  Returns 0
 * for a kind it does not know. */
u32 fn_802E099C(_SPR_DATA_* rec, const _SPR_ANIM_* anim, u16 frame)
{
    switch (anim->flags & SPR_ANIM_KIND_MASK) {
    case 0:
        return fn_802E29F4(rec, anim, frame);
    case 1:
        return anim_step_tex_id(&rec->tex_id, anim, frame);
    case 2:
        return fn_802E2D84(&rec->u_scale, anim, frame);
    case 3:
        return anim_step_uv(&rec->uv0, &rec->uv1, anim, frame);
    case 4:
        return anim_step_color(&rec->color, anim, frame);
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
u32 sprite_frame_apply(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out)
{
    spr_data_copy(rec, get_lsp_data(id, 0));
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
        switch (anim->flags & SPR_ANIM_KIND_MASK) {
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
u16 sprite_ary_last_frame(const u16* ids)
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
 * `sprite_frame_apply`, then handed to the copy-and-recolour helper. */
u32 fn_802E12A0(u16 id, u16 part, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

    fn_802E1024(&work, tex_idx, color, pos);
    return drawn;
}

/* 0x802E1320 (0x74) - the same keyed by the item record. */
void draw_itemicon_anim_idx(u16 id, u16 part, u16 item_id, const _mh_ivec2_* pos)
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
void draw_equipicon_idx(u16 id, _EQUIP* equip, const _mh_ivec2_* pos)
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
void draw_sprite_uv_color_idx(u16 id, u8 tex_idx, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work = *get_lsp_data(id, 0);

    fn_80055D8C(&work.uv0, &work.uv1, tex_idx);
    work.color = color;
    draw_sprite(work, pos);
}

/* 0x802E163C (0xA4) - the same without the colour override. */
void draw_sprite_uv_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos)
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
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

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
u32 draw_number_anim(u16 id, u16 part, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    _SPR_DATA_ work;
    u32 drawn = sprite_frame_apply(&work, id, part, 0);

    fn_802E1AEC(&work, frame, color, pos);
    return drawn;
}

/* 0x802E1D30 (0x5C) - the same for a sprite id. */
void fn_802E1D30(u16 id, u8 frame, u32 color, const _mh_ivec2_* pos)
{
    fn_802E1C74(get_lsp_data(id, 0), frame, color, pos);
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

    return (u8)c0 | ((u8)c1 << 8) | ((u8)c3 << 24) | ((u8)c2 << 16);
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
    return c0 | (c1 << 8) | (c3 << 24) | (c2 << 16);
}

/* The animation-step walk every `anim_step_*` entry shares: the step's `count` frames are a table
 * of `frame`/`interp`-tagged keys, and the walk finds the two keys `frame` falls between, then
 * interpolates their payloads.  Returns 1 when a payload was written and 0 when the frame ran past
 * the step's last key. */

/* 0x802E2C08 (0x17C) - the texture-id step (kind 1, a 6-byte key). */
u32 anim_step_tex_id(u16* out, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_TEX_* cur = (const _SPR_KEY_TEX_*)anim->frames;
    const _SPR_KEY_TEX_* prev = cur;
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
        u16 from = prev->tex_id;
        s16 delta = cur->tex_id - prev->tex_id;

        *out = from + (s16)(rate * (f32)delta);
    }
    return 1;
}

/* 0x802E2F54 (0x318) - the texture-coordinate step (kind 3, a 12-byte key holding the first pair
 * and the second pair's offset from it). */
u32 anim_step_uv(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, const _SPR_ANIM_* anim, u16 frame)
{
    const _SPR_KEY_UV_* cur = (const _SPR_KEY_UV_*)anim->frames;
    const _SPR_KEY_UV_* prev = cur;
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
    const _SPR_KEY_COLOR_* cur = (const _SPR_KEY_COLOR_*)anim->frames;
    const _SPR_KEY_COLOR_* prev = cur;
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

/* The note box the cockpit's message window is laid out from: `note_text_max_len` measures its widest
 * line, the two anchor entries hand the box's position to the lobby, and the three extent helpers
 * offset a row by the line count the box is showing.  Names are derived from the behaviour (the
 * only caller-side evidence is `note_box_draw` in `enemy/fn_80382310.cpp`) - GUESS. */

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

/* 0x802E44A8 (0x44) - the upper note-box anchor. */
void note_box_anchor_upper(_mh_ivec2_* pos)
{
    pos->x = (s16)(get_wide_offset(2) + 30);
    pos->y = 360;
}

/* 0x802E44EC (0x44) - the lower note-box anchor. */
void note_box_anchor_lower(_mh_ivec2_* pos)
{
    pos->x = (s16)(get_wide_offset(2) + 30);
    pos->y = 386;
}

/* 0x802E45D4 (0x30) - the note box's own extent for `lines` rows: nothing until the fifth row,
 * then a negative height that grows by 16 per row. */
void note_box_size(_mh_ivec2_* extent, s16 lines)
{
    extent->x = 0;
    extent->y = 0;
    if (lines > 4) {
        s16 height = (s16)((lines - 4) * 16);

        extent->y = -height;
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
