/*
 * Naming note: the one callee this file still spells by its map stem (`fn_8021B890`) is a bare .text
 * entry in config/RMHE08/symbols.txt with `zz_` in the runtime dump, and its address is not this
 * unit's own row.
 * One player's move-indicator draw: `move_indicator_draw`, `.text` 0x80324F7C-0x803250B0 (0x134 B).
 *
 * It samples a HUD anchor with `get_lsp_data(0x20D3, …)`, derives two more anchors from it with the
 * 2D-vector copy `uv_pair_copy`, and blits sprite-id runs at each with `draw_sprite_ary`.  The second
 * variant of the layout (the extra sprite plus the two ±10 y offsets) is selected by the caller's move
 * record: when its flag byte is clear and the lobby is up (`fn_8021B890`) the plain layout is drawn,
 * otherwise the expanded one.  `get_option_cfg(7)` picks between two further sprite runs.
 *
 * The argument is one element of the per-player "move work" array `get_move_work_adrs` indexes at stride
 * 0xB20 - the caller (`move_work_update`, 0x264) passes exactly such a pointer, and that function's own
 * `addi r26,r26,2848` gives the stride.  `_mh_move_work_` below states only the byte this unit reads.
 *
 * The sprite-id runs are the unit's own `.data`/`.sdata` pool (`lbl_805DD9C8` 0x18 B in `.data`, the
 * nine `lbl_80792Cxx` 4/8 B in `.sdata`); they are `extern`-declared here and never defined (playbook 29),
 * so the object emits only the references.  `lbl_805DD9C8` is declared a *sized array* on purpose: that
 * is what makes MWCC address it `lis`/`addi` where the `.sdata` scalars get `…@sda21` - the same split
 * the target object has (pl_act's header records the same finding).
 *
 * The name is derived, not the dump's: the shared runtime dump's name list has no entry at 0x80324F7C
 * (only its `zz_0324f7c_` placeholder), so it is named for what it draws - one record's move indicator.
 *
 * The one codegen deviation is `-opt nopeephole`, which the `hud` lib now carries as `cflags_hud`
 * (2026-09-27; this file said `the pragma` before that): with `-opt nopeephole` on the real command line
 * this object is byte-identical to the target, so the original TU was built peephole-off.  The visible
 * effect is the target's `get_option_cfg` test - `clrlwi r0,r3,24` + `cmpwi r0,0`, the non-record form -
 * which MWCC fuses into `clrlwi.` + `bne` with the peephole on, leaving the function 4 bytes short
 * (98.64 %).  It is not a project-wide default: only three of the DOL's split objects carry the fused
 * form at all, and the three `hud` ones agree (playbook 33: the flag is the preferred home).  ~25 source
 * shapes were measured against the target object first and none produces the non-fused pair: `== 0` /
 * `!= 0` / `> 0` / `< 1` / `>= 1` / `>= 1U` / `& 0xFF`, a `u8`/`u32` local, `(u8)`/`(u32)` casts, a
 * `switch`, a `do { … } while (0)` and both branch orders; `-O2`, `-O4,p` and `-O1` do not help either
 * (`-O4,p` costs 20 points).  Only this unit's file is affected (one function).
 */

#include "types.h"

/* The 2D integer vector the HUD helpers exchange (`_mh_ivec2_` in the map's mangling). */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_; /* size: 0x4 */

/* One player's move-work record: `get_move_work_adrs(player)` indexes an array of these at stride 0xB20
 * (`move_work_update`: `mulli r0,r0,2848` / `addi r26,r26,2848`).  Only the flag byte this unit reads is
 * named; `fn_80321830` sets its bits 0/1 with `ori`.  size: 0xB20 */
typedef struct _mh_move_work_ {
    /* +0x000 */ u8 unused_0x000[0xB01];
    /* +0xB01 */ u8 flags;
    /* +0xB02 */ u8 unused_0xB02[0x1E];
} _mh_move_work_; /* size: 0xB20 */

/* These callees are still unsplit, so their declarations live here (the owning TUs have no source yet);
 * the spellings are the map's mangled names, which is what the target object relocates against. */
void get_lsp_data__FUsP10_mh_ivec2_(u16 id, _mh_ivec2_* out);
void uv_pair_copy(_mh_ivec2_* dst, const _mh_ivec2_* src);
u32 fn_8021B890(void);
void draw_sprite_ary__FPCUsPC10_mh_ivec2_(const u16* sprite_ids, const _mh_ivec2_* pos);
u8 get_option_cfg__FUc(u8 id);

extern const u16 lbl_805DD9C8[12]; /* 0x18 B */
extern const u16 lbl_80792CC8;
extern const u16 lbl_80792CD0;
extern const u16 lbl_80792CD8;
extern const u16 lbl_80792CE0;
extern const u16 lbl_80792CE8;
extern const u16 lbl_80792CF0;
extern const u16 lbl_80792CF8;
extern const u16 lbl_80792CFC;
extern const u16 lbl_80792D00;

/* This unit's single codegen deviation: the original object carries the non-fused `clrlwi` + `cmpwi`
 * for the `get_option_cfg` test (see the file header).  The pragma is file-wide because the file is one
 * function, and its only observable effect is that fusion - the rest of the function is unaffected. */
/* Draws one player's move indicator: the base sprite at the HUD anchor plus the extra sprite and the
 * shifted anchors the expanded layout needs. */
void move_indicator_draw(_mh_move_work_* self)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ left;
    _mh_ivec2_ right;
    u32 expanded;

    get_lsp_data__FUsP10_mh_ivec2_(0x20D3, &anchor);
    uv_pair_copy(&left, &anchor);
    uv_pair_copy(&right, &anchor);

    if (self->flags == 0 && fn_8021B890() == 1) {
        expanded = 0;
    } else {
        expanded = 1;
        left.y += 10;
        right.y -= 10;
    }

    draw_sprite_ary__FPCUsPC10_mh_ivec2_(lbl_805DD9C8, &anchor);
    draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CC8, &left);
    draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CD0, &right);

    if (expanded == 0) {
        draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CF8, &anchor);
    }

    if (get_option_cfg__FUc(7) == 0) {
        draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CD8, &left);
        draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CE0, &right);
        if (expanded == 0) {
            draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CFC, &left);
        }
    } else {
        draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CE8, &left);
        draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792CF0, &right);
        if (expanded == 0) {
            draw_sprite_ary__FPCUsPC10_mh_ivec2_(&lbl_80792D00, &left);
        }
    }
}
