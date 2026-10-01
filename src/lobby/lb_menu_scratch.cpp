/*
 * lobby/lb_menu_scratch.cpp - the lobby menu scratch block and its item-list page head.
 *
 * `.text` 0x8021E1EC..0x8021F020 (14 functions), `.bss` 0x806AA8C8..0x806AAA88 (`lb_menu_scratch`, 0x1C0 B), `.data` 0x480 B,
 * `.sdata` 0x10 B, `.sdata2` 0x18 B, `.ctors` 4 B, extab 0x30 B and extabindex 0x48 B.  Phase 4 recut of
 * `lobby/fn_8021E1EC` (docs/splits/phase4): the registered range 0x8021E1EC..0x80224AC4 is three TUs of the candidate -
 * this one, `lobby/lb_menu_pos_tbl` and `lobby/lb_equip_page`; each part keeps the former source's text for its own
 * functions, the shared declarations and its own `.bss` definitions.
 *
 * Name: the candidate's (`lb_menu_scratch` is the `.bss` block the unit constructs, `fn_8021EFC8`).
 * Flags: `cflags_lobby` as the former unit, with its file-scope `#pragma peephole off`.
 */

/* ==== recut from lobby/fn_8021E1EC.cpp (0x8021E1EC..0x8021F020) ==== */
/* lobby/fn_8021E1EC.cpp - a lobby screen layer (item/equipment page family).
 *
 * `.text` 0x8021E1EC..0x80224AC4 (108 functions, 26840 B), registered from
 * `proposal/8021E1EC_fn_8021E1EC.cpp`.
 *
 * Module `lobby`.  The range's callees are the lobby UI API - `LbStr__FUcUs` (13 call sites),
 * `draw_sprite_ary` (23), `draw_font_idx` (17), `get_lsp_data` (40), `ItemName`, `put_menu_cursor`,
 * `GetMenuFontColor` - and the `.bss` labels it reads inside this band (`lb_menu_scratch`, `lobby_w`,
 * `lb_npc`); both bracketing registered units in the address band are `lobby`
 * (`lobby/fn_80212810.cpp` below at 0x80212810, `lobby/fn_801E7530.cpp` and `lobby/lb_npc.cpp`
 * further below).  Language C++: every call out of the range is a mangled symbol
 * (`LbStr__FUcUs`, `draw_sprite_ary__FPCUsPC10_mh_ivec2_`, `setMatColor__6MHcharFUl12_GXChannelID8_GXColorb`).
 *
 * Seam.  `tools/splits/tudiscover.py at 0x8021E1EC` reports the left edge as a *strong* cut
 * (`.sdata2` pool run `lbl_80799C58 -> lbl_80799C60`) and the right edge only weakly (the closure edge
 * 0x8021E538, or the far jump 0x8021F3A8); the proposal's right edge 0x80224AC4 is the `--max-bytes`
 * cap, not a TU boundary.  The extab/extabindex runs agree with *both* edges exactly: this range owns
 * 77 consecutive extabindex records, the first of which is `fn_8021E1EC` (the record before it is
 * `fn_8021E1B4`, size 0x38, which ends exactly at 0x8021E1EC) and the last of which is `fn_80224A28`
 * (the record after it is `fn_80224AC4`).  Registered whole; the next proposal (0x80224AC4) continues
 * the same band.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range (`tools/symbols/dumpmap.py lookup` over the
 * whole inventory answers `zz_XXXXXXXX_` placeholders, and every name but the callees is a bare
 * `.text` entry in config/RMHE08/symbols.txt), so the file keeps the map's stem (brief section 2,
 * class 4).
 *
 * Types and globals.  This unit's `.bss` 0x806AA8C8..0x806AACC0 is claimed and defined at the foot of the file:
 * `lb_menu_scratch`, `lb_item_list_state`, two `.data`-referenced page records (GUESS names
 * `lb_page_state_0/1`), five fixed positions `fn_8021FF5C` sets (GUESS names `lb_menu_pos_*`) and `lobby_w`
 * (record `LbLobbyWork` in `lobby/lobby_work.h`, the merge of this unit's and the lobby band's views).  Its
 * two static constructors (`fn_8021EFBC`, `fn_8021FF5C`) are not reconstructed.  `lobby_world_block` is
 * read as the 4-byte pointer the map records, through this unit's own `LbMenuBigBlock` view.
 *
 * Flags.  `tools/flags/infer.py` reads the target object: `-func_align 4` (83 of 108 functions start off a
 * 16-byte boundary), no `lmw/stmw`, and "3 kept `clrlwi` before a narrowing store" - the peephole pass
 * was off.  The unit therefore carries `#pragma peephole off` (playbook 39), with the pass turned back on
 * for the two bodies whose retail form has no such `clrlwi` (`fn_8021E304`, `fn_802216B4`).  The pass-off
 * build took fn_8021E1EC 94.78 -> 97.10, fn_8021E484 95.33 -> 100.00, fn_80220B50 92.31 -> 100.00, and
 * cost fn_8021E304 100.00 -> 93.33 and fn_802216B4 100.00 -> 87.69 (hence the two `on` regions).  A
 * unit-level `-opt nopeephole` would express the same intent more honestly; it is in the outbox as a
 * `flag` request (the `lobby` lib's own `lobby_scene.c` is `Matching`, so the lib's cflags must not move
 * for a one-unit measurement).
 *
 * Residuals (recompile.py --measure, official report metric; the bar is 80 %).  25 of the 108 functions
 * are byte-identical and 35 clear the bar; the rest are not reconstructed yet.
 *  - the 73 unreconstructed functions, largest first: fn_8021E538 1720 B, fn_80220DFC 1456 B,
 *    fn_8021F7DC 1268 B, fn_80221E04 1076 B, fn_80223EF0 968 B, fn_8022015C 916 B, fn_80221890 812 B,
 *    fn_8021EC98 804 B, fn_802213AC 776 B, fn_80222908 700 B, fn_80224550 672 B, fn_80223318 652 B ...
 *  - fn_8021E1EC 97.10 % (276 B): the only row left is the allocator's tie-break - retail copies the 4th
 *    integer argument (`mr r30, r6`) *after* the float argument (`fmr f31, f1`), ours before.  Swapping
 *    the parameter declaration order, the local declaration order and the `~limit` spelling
 *    (`subfic r0,r30,-1`) all fixed everything else; the register/parameter order does not move.
 *  - fn_802235E0 85.00 % (28 B): hoisting `*base` before the `offset == 0` test fixed the load order;
 *    the remaining row is a scheduling/ordering delta in the same 10-instruction body.
 *  - fn_802216B4 80.00 % (52 B): retail zero-extends the `u16 id` parameter (`clrlwi r3,r3,16`) and
 *    reaches `lbl_80791F30` through `lbl_80791F30@sda21`; ours passes the parameter through and emits
 *    `lis`/`addi`.  Declaring the table with an explicit small-data size is the next probe.
 *  - fn_80220AF0 95.62 % (96 B): retail materialises the `extsh`-ed index before the `lbzx`; ours folds
 *    it.  Best-scoring variant kept.
 *  - fn_8021F020 78.89 % (188 B): below the bar.  Two rows: retail keeps no third callee-saved register
 *    (ours saves `r29` for the surviving argument) and it emits `frsp f1,f1` + `fcmpo`/`cror` for the
 *    `depth == lbl_80799C7C` test where ours emits `fcmpu`.  `count` is converted unsigned (no `xoris`),
 *    which is why the parameter is `u32`.
 *  - fn_80221BBC 41.48 % (108 B): reconstructed but the sprite-row argument ordering differs; kept as the
 *    best-scoring variant (the 0 % baseline is worse).
 *  - fn_80223A18/fn_80223A44 85.45 % (44 B each): the two table accessors - the row-stride `add` comes
 *    out right, the leading `clrlwi`/`slwi` pair is scheduled differently.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup over the range's inventory: all 108 definitions are bare `.text` entries in
 * config/RMHE08/symbols.txt and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"
#include "nw4r/math.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "lobby/fn_8021E1EC.h"
#include "lobby/fn_80208AC0.h" /* fn_80208AC0, owned by lobby/lb_npc.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* This range's record view of the user-data block; the owner's `lobby_world_block` is a plain `u8*`. */
static inline LbMenuBigBlock* lb_menu_block(void)
{
    return (LbMenuBigBlock*)lobby_world_block;
}

/* The retail object keeps the narrowing `clrlwi` the peephole pass folds away (flags/infer.py: "3 kept
 * `clrlwi` before a narrowing store"), so the unit is built with the pass off.  Two functions keep the
 * pass on: their retail bodies have no `clrlwi` to keep, and the pass-off build re-narrows. */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * Foreign callees.
 *
 * The plain `fn_XXXXXXXX` names are this unit's own spelling of unsplit addresses whose bracketing
 * registered units name different modules (rule 2's named gap), the same way
 * `src/lobby/fn_80212810.cpp` records them; the C++-linkage lobby ABI lives in this unit's header.
 */
extern "C" {
void* fn_8021EFC8(LbMenuScratch* work);
u32 fn_8021E1B4(LbMenuActor* self, VEC3* pos);
f32 fn_80463F04(f32 v);

u32 fn_8021DF50(u32 idx, u32 limit);
void fn_8021DDA8(LbMenuCandidate* candidate, LbMenuRow* row);
f32 fn_8021E300(f32 v);
s32 game_ready_ck(void);
u8 fn_8004DD74(void);









}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * Definitions, in address order.
 */

/* The candidate recorder: copies the entry's position (or the fallback work record's), rejects it when
 * it is outside the y window or its slot is, and otherwise appends it to the scratch row. */
s32 fn_8021E1EC(LbMenuActor* self, LbMenuActor* rec, LbMenuFallback* work, u32 limit, f32 radius)
{
    LbMenuCandidate candidate;
    u32 idx;
    f32 dist_sq;
    VEC3 pos;

    VEC3_ctor(&pos);
    if (rec != 0) {
        copyVec3(&pos, &rec->pos_0x03C);
    } else {
        copyVec3(&pos, &work->pos_0x10);
    }
    if (fn_8021E300(pos.y - self->pos_0x03C.y) < lbl_80799C60) {
        idx = fn_8021E1B4(self, &pos);
        dist_sq = calcDistanceSqXZ(&self->pos_0x03C, &pos);
        if ((idx < limit || idx > (u32)(-1 - (s32)limit)) && dist_sq < radius) {
            candidate.rec_0x00 = rec;
            candidate.work_0x04 = work;
            candidate.dist_sq_0x08 = dist_sq;
            candidate.slot_0x0C = (s16)fn_8021DF50(idx, limit);
            fn_8021DDA8(&candidate, &lb_menu_scratch.rows_0x00C[0]);
            return 1;
        }
    }
    return 0;
}

/* The y-window filter's absolute value helper (retail: `b <fabsf>`). */
f32 fn_8021E300(f32 v)
{
    return fn_80463F04(v);
}

/* The entry filter: the entry's kind byte must not be 0x49 on a busy actor, and its mode must not be
 * the "special" one.  (peephole on: this body has no narrowing store the pass could fold.) */
#pragma peephole on
s32 fn_8021E304(LbMenuActor* self, LbMenuEntryRec* entry)
{
    if (entry->kind_0x002 == 0x49 && self->busy_0xB03) {
        return 0;
    }
    if (entry->mode_0x256 == 1) {
        return 0;
    }
    return 1;
}

#pragma peephole off

/* The page's per-kind gate: whether the entry kind may be opened from the current lobby state. */
s32 fn_8021E340(LbMenuActor* self, s16* kind)
{
    s32 ok = 1;

    if (fn_80208AC0() != 0) {
        ok = 0;
    }
    switch (*kind) {
    case 10:
    case 11:
    case 14:
    case 15:
        if (game_ready_ck() == 0) {
            ok = 0;
        }
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    case 1:
    case 6:
        if (self->busy_0xB03) {
            ok = 0;
        }
        break;
    case 3:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        if (!lobby_w.page_0x052) {
            ok = 0;
        }
        break;
    case 4:
        if (lobby_w.page_0x052 != 6) {
            ok = 0;
        }
        break;
    default:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* The item-list page's "still idle" gate. */
s32 fn_8021E454(void)
{
    if (lobby_w.state_0x000 == 0 && lobby_w.flag_0x027 == 0) {
        return 1;
    }
    return 0;
}

/* The page's availability gate: the item list must be open and its kind must match. */
s32 fn_8021E484(s32 kind)
{
    if (fn_8004DD74() == 0) {
        return 0;
    }
    switch ((u8)kind) {
    case 0:
        if (lb_menu_block()->list_kind_0x484D) {
            return 0;
        }
        break;
    case 1:
        if (lb_menu_block()->list_kind_0x484D != 1) {
            return 0;
        }
        break;
    case 2:
        if (lb_menu_block()->list_kind_0x484D != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

/* The current row's model pointer, or 0 when there is none. */
s32 fn_8021EBF0(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04;
    }
    return 0;
}

/* The current row's table pointer, or 0 when there is none. */
s32 fn_8021EC20(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00;
    }
    return 0;
}

/* The special (fixed) entry's two pointers, one per accessor; only valid in the -2 mode. */
s16* fn_8021EC50(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_a_0x004;
    }
    return 0;
}

u8* fn_8021EC74(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_b_0x008;
    }
    return 0;
}

/* The no-argument form the constructor table calls.  It is defined before the callee so the retail
 * out-of-line tail call survives. */
void fn_8021EFBC(void)
{
    fn_8021EFC8(&lb_menu_scratch);
}

/* The scratch block's constructor: clears its 8-entry vector run. */
void* fn_8021EFC8(LbMenuScratch* work)
{
    VEC3* p = work->slots_0x160;
    VEC3* end = work->slots_0x160 + 8;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < end);
    return work;
}

/* One shared sprite row: the layout entry plus its row table.  (peephole on: see fn_8021E304.) */
#pragma peephole on

#pragma peephole off

/* This unit's own `.bss` (`splits.txt` `.bss 0x806AA8C8..0x806AACC0`), in address order.  Defined at the foot
 * of the file, after every use, so MWCC keeps one `lis`/`addi` pair per symbol like the target.  The two static
 * constructors the `.ctors` claim carries build `lb_menu_scratch` (`fn_8021EFBC`) and the position tables plus
 * `lobby_w` (`fn_8021FF5C`, which `setVec3`s the five vectors and calls `fn_8021FFFC(&lobby_w)`); the
 * constructors are not reconstructed, so the objects are plain storage here. */
LbMenuScratch lb_menu_scratch;             /* +0x806AAB44 */

}  /* extern "C" */
