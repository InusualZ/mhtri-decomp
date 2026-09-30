/* lobby/fn_8021E1EC.cpp - a lobby screen layer (item/equipment page family).
 *
 * `.text` 0x8021E1EC..0x80224AC4 (108 functions, 26840 B), registered from
 * `proposal/8021E1EC_fn_8021E1EC.cpp`.
 *
 * Module `lobby`.  The range's callees are the lobby UI API - `LbStr__FUcUs` (13 call sites),
 * `draw_sprite_ary` (23), `draw_font_idx` (17), `get_lsp_data` (40), `ItemName`, `put_menu_cursor`,
 * `GetMenuFontColor` - and the `.bss` labels it reads inside this band (`lbl_806AA8C8`, `lobby_w`,
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
 * Types and globals.  This unit carries its own view of the records it reads, the way
 * `src/lobby/lb_npc.cpp` does: `include/unsplit/lobby.h` spells `lobby_world_block` as an array and
 * `lobby_w` as `LbLobbyWork`, neither of which is the shape this range uses (it loads `lobby_world_block`
 * as the 4-byte pointer the map records, and reads `lobby_w` at +0x01/+0x27/+0x52/+0xB1).  The
 * per-unit view keeps the offsets honest instead of re-interpreting another unit's type.
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
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

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
void fn_8021FCE0(u8 a, s32 b, u16 c, s32 d);
u32 fn_8021DF50(u32 idx, u32 limit);
void fn_8021DDA8(LbMenuCandidate* candidate, LbMenuRow* row);
f32 fn_8021E300(f32 v);
s32 fn_80208AC0(void);
s32 game_ready_ck(void);
u8 fn_8004DD74(void);
s32 fn_80217934(void);
u8* fn_80223A18(u8 table, s32 index);
LbGlobalBlock* fn_80064080(void);
u8* fn_80223A44(u8 table, s32 index);
void fn_802FAFC8(s32 id);
void fn_80221C28(LbMenuItem* item, s32 open);
s32 fn_802205D8(u16 id);
void fn_80221E04(LbMenuItem* item);
void* fn_802235E0(u32* base, s32 offset);
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
            fn_8021DDA8(&candidate, &lbl_806AA8C8.rows_0x00C[0]);
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
        if (lobby_w.mode_0x000) {
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
        if (lobby_w.mode_0x000) {
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
        if (lobby_w.mode_0x000) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* The item-list page's "still idle" gate. */
s32 fn_8021E454(void)
{
    if (lobby_w.mode_0x000 == 0 && lobby_w.flag_0x027 == 0) {
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
        if (lobby_world_block->list_kind_0x484D) {
            return 0;
        }
        break;
    case 1:
        if (lobby_world_block->list_kind_0x484D != 1) {
            return 0;
        }
        break;
    case 2:
        if (lobby_world_block->list_kind_0x484D != 2) {
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
    if (lbl_806AA8C8.index_0x14C >= 0 && lbl_806AA8C8.rows_0x00C[lbl_806AA8C8.index_0x14C].model_0x04 != 0) {
        return lbl_806AA8C8.rows_0x00C[lbl_806AA8C8.index_0x14C].model_0x04;
    }
    return 0;
}

/* The current row's table pointer, or 0 when there is none. */
s32 fn_8021EC20(void)
{
    if (lbl_806AA8C8.index_0x14C >= 0 && lbl_806AA8C8.rows_0x00C[lbl_806AA8C8.index_0x14C].table_0x00 != 0) {
        return lbl_806AA8C8.rows_0x00C[lbl_806AA8C8.index_0x14C].table_0x00;
    }
    return 0;
}

/* The special (fixed) entry's two pointers, one per accessor; only valid in the -2 mode. */
s16* fn_8021EC50(void)
{
    if (lbl_806AA8C8.index_0x14C == -2) {
        return lbl_806AA8C8.fixed_a_0x004;
    }
    return 0;
}

u8* fn_8021EC74(void)
{
    if (lbl_806AA8C8.index_0x14C == -2) {
        return lbl_806AA8C8.fixed_b_0x008;
    }
    return 0;
}

/* The no-argument form the constructor table calls.  It is defined before the callee so the retail
 * out-of-line tail call survives. */
void fn_8021EFBC(void)
{
    fn_8021EFC8(&lbl_806AA8C8);
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

/* The item-list page's animation clocks. */
f32 fn_8021F218(void)
{
    return lbl_806AAA88.step_0x18;
}

f32 fn_8021F228(void)
{
    return lbl_806AAA88.depth_0x34;
}

/* The selected row; also published to the sound unit through `include/unsplit/lobby.h`. */
s32 fn_8021F238(void)
{
    return lbl_806AAA88.selected_0x38;
}

/* Selects a row and remembers whether the selection actually moved. */
void fn_8021F248(s32 selected)
{
    s32 prev = lbl_806AAA88.selected_0x38;
    u8 masked = selected;

    lbl_806AAA88.selected_0x38 = masked;
    if (prev == masked) {
        lbl_806AAA88.changed_0x3C = 0;
    } else {
        lbl_806AAA88.changed_0x3C = 1;
    }
}

/* True on the frame the selection changed. */
s32 fn_8021F27C(void)
{
    return lbl_806AAA88.changed_0x3C - 1 == 0;
}

/* Acknowledges the selection change. */
void fn_8021F298(void)
{
    lbl_806AAA88.changed_0x3C = 0;
}

/* The two-argument row draw the three table variants share. */
void fn_8021FCD0(u8 a, s32 b, u16 c)
{
    fn_8021FCE0(a, b, c, 1);
}

/* The 8-byte row copy. */
void fn_802208A8(LbMenuRow8* dst, LbMenuRow8* src)
{
    *dst = *src;
}

/* The row-table accessors: `a` picks the table, `i` the row; -1 means "no row". */
u8* fn_80223A18(u8 table, s32 index)
{
    u8* base = lbl_807922B0[table];

    if (index == -1) {
        return 0;
    }
    return base + index * 8;
}

u8* fn_80223A44(u8 table, s32 index)
{
    u8* base = lbl_807922B8[table];

    if (index == -1) {
        return 0;
    }
    return base + index * 2;
}

/* The page's row classifier. */
u32 fn_80221864(u8* row)
{
    if (row[0] == 3 && (u32)(row[1] - 1) <= 1) {
        return 1;
    }
    return 0;
}

/* The offset accessor for the 4-byte base the page keeps at +0. */
void* fn_802235E0(u32* base, s32 offset)
{
    u32 addr = *base;

    if (offset == 0) {
        return 0;
    }
    return (u8*)addr + offset;
}

/* The no-argument page entry the item page's dispatcher tails into. */
void fn_80220818(void)
{
    fn_80217934();
}

/* One shared sprite row: the layout entry plus its row table.  (peephole on: see fn_8021E304.) */
#pragma peephole on
void fn_802216B4(u16 id)
{
    _mh_ivec2_ pos;

    get_lsp_data(id, &pos);
    draw_sprite_ary(lbl_80791F30, &pos);
}

#pragma peephole off

/* The item row's icon header strip. */
void fn_80221BBC(LbMenuItem* item)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x131B, &pos);
    draw_sprite_ary(lbl_805BA820, &pos);
    fn_802216B4(0x13C9);
    fn_802216B4(0x13CD);
    fn_802216B4(0x13CE);
    draw_sprite_anim_idx(0x132B, item->icon_0x2C, &pos);
}

/* The equipment slot's frame: open or closed depending on the row's own flag. */
void fn_80222238(LbMenuItem* item)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x138E, &pos);
    draw_sprite_ary(lbl_805BA968, &pos);
    if (item->active_0x01 == 0) {
        draw_sprite_ary(lbl_805BA974, &pos);
        fn_80221C28(item, 1);
        return;
    }
    draw_sprite_ary(lbl_805BA980, &pos);
    fn_80221E04(item);
}

/* The wide-mode backdrop row. */
void fn_802227BC(void)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x129C, &pos);
        draw_sprite_ary(lbl_805BAA14, &pos);
    } else {
        get_lsp_data(0x1297, &pos);
        draw_sprite_ary(lbl_805BAA08, &pos);
    }
    draw_sprite_idx(0x1290, &pos);
    get_lsp_data(0x1286, &pos);
    draw_sprite_ary(lbl_805BA9E4, &pos);
}

/* The identity-ish 3x3 the page starts from. */
void fn_802230C8(void* unused_0x00, LbMat3x3* m)
{
    m->m_0x00 = lbl_80799CD8;
    m->m_0x04 = lbl_80799CD8;
    m->m_0x08 = lbl_80799CD8;
    m->m_0x0C = lbl_80799CDC;
    m->m_0x10 = lbl_80799CDC;
    m->m_0x14 = lbl_80799CDC;
    m->m_0x18 = lbl_80799CDC;
    m->m_0x1C = lbl_80799CDC;
    m->m_0x20 = lbl_80799CDC;
}

/* Whether the item id is one of the first `count` rows of the row table. */
s32 fn_80220B50(LbMenuItem* item, s32 count, u16 id)
{
    s32 i;

    for (i = 0; i < count; i++) {
        if (item->ids_0x60[i] == id) {
            return 1;
        }
    }
    return 0;
}

/* Opens the item page: arms the SE, then raises its own flags. */
void fn_80220038(void)
{
    LbMenuBigBlock* block = lobby_world_block;

    fn_802FAFC8(0x19);
    block->rows_0x4654[0].open_0x00 = 1;
    block->rows_0x4654[0].ready_0x01 = 1;
    block->count_0x4670 = 1;
}

/* The three countdown bytes the item page runs down. */
void fn_80220114(void)
{
    LbMenuRowSlot* row = &lobby_world_block->rows_0x4654[1];

    if (row[0].open_0x00) {
        row[0].open_0x00--;
    }
    if (row[1].open_0x00) {
        row[1].open_0x00--;
    }
    if (lobby_world_block->fade_c_0x466C) {
        lobby_world_block->fade_c_0x466C--;
    }
}

/* The item row's per-row reset: the four flag bytes rise and fall around the row's state word. */
void fn_802208D4(LbMenuItem* item)
{
    LbMenuBigBlock* block = lobby_world_block;

    item->flag_a_0x24 = 1;
    item->flag_b_0x25 = 1;
    item->flag_c_0x26 = 1;
    item->flag_d_0x27 = 1;
    memset(&item->base_a_0x4C, 0, 8);
    item->slot_0x46 = 0;
    item->value_0x3C = 0;
    item->kind_0x30 = 0;
    item->flag_a_0x24 = 0;
    item->flag_b_0x25 = 0;
    item->flag_e_0x56 = 0;
    item->flag_f_0x57 = 0;
    if (block->rows_0x4654[item->row_0x2E].state_0x04 != 0) {
        item->icon_0x2C = 1;
    } else {
        item->icon_0x2C = 0;
    }
}

/* The item row's deactivate: the four state fields first, then the shared reset above. */
void fn_8022097C(LbMenuItem* item)
{
    item->row_0x2E = 0;
    item->kind_0x30 = 0;
    item->active_0x01 = 0;
    item->icon_0x2C = 0;
    fn_802208D4(item);
}

/* The row's icon variant from its state byte (`fn_802205D8` maps the item to a byte index). */
s16 fn_80220AF0(u8* state, u16* id)
{
    switch (state[fn_802205D8(*id)]) {
    case 3:
        return 0xA;
    case 2:
        return 5;
    default:
        return 3;
    }
}

/* The page's tick: the row count as a float and the per-tick step, then the changed flag. */
void fn_8021F020(u32 stamp, u32 count, f32 depth)
{
    lbl_806AAA88.count_0x14 = count;
    lbl_806AAA88.step_0x18 = lbl_80799C78 * count;
    lbl_806AAA88.scroll_0x30 = depth;
    lbl_806AAA88.depth_0x34 = depth;
    lbl_80794868 = stamp;
    lbl_806AAA88.stamp_0x04 = stamp;
    if ((f32)depth == lbl_80799C7C) {
        lbl_806AAA88.selected_0x38 = 1;
    } else {
        lbl_806AAA88.selected_0x38 = 0;
    }
    lbl_806AAA88.changed_0x3C = 0;
    lbl_806AAA88.events_0x48 = (s8)CalculateEvents();
    lbl_806AAA88.active_0x00 = 0;
}

/* The base-relative accessor `fn_80064080`'s block wants. */
void fn_802235A4(u32* base)
{
    fn_802235E0(base, fn_80064080()->field_0x08);
}

}  /* extern "C" */
