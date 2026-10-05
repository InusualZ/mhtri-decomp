/*
 * lobby/lb_menu_pos_tbl.cpp - the lobby menu's fixed position table, the page state records and the lobby work block.
 *
 * `.text` 0x8021F020..0x80220038 (20 functions), `.bss` 0x806AAA88..0x806AACC0 (`lb_item_list_state`, `lb_page_state_0/1`,
 * `lb_menu_pos_tbl`, `lb_menu_pos_extra`, `lobby_w`; 0x238 B), `.data` 0x10 B, `.sdata2` 0x60 B, `.ctors` 4 B, extab 0x60 B and
 * extabindex 0x90 B.  Phase 4 recut of `lobby/fn_8021E1EC`: see `lobby/lb_menu_scratch.cpp` for the
 * three-way split.
 *
 * Name: the candidate's (`lb_menu_pos_tbl` is the position table `fn_8021FF5C` fills).
 * Flags: `cflags_lobby` as the former unit, with its file-scope `#pragma peephole off`.
 */

/* ==== recut from lobby/fn_8021E1EC.cpp (0x8021F020..0x80220038) ==== */
/* lobby/fn_8021E1EC.cpp - a lobby screen layer (item/equipment page family).
 *
 * `.text` 0x8021E1EC..0x80224AC4 (108 functions, 26840 B), registered.
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
 *  - syncItemListClock 78.89 % (188 B): below the bar.  Two rows: retail keeps no third callee-saved register
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



void fn_8021FCE0(u8 a, s32 b, u16 c, s32 d);















}

extern "C" {

/* The entry filter: the entry's kind byte must not be 0x49 on a busy actor, and its mode must not be
 * the "special" one.  (peephole on: this body has no narrowing store the pass could fold.) */
#pragma peephole on

#pragma peephole off

/* The item-list page's animation clocks. */
f32 fn_8021F218(void)
{
    return lb_item_list_state.step_0x18;
}

f32 fn_8021F228(void)
{
    return lb_item_list_state.depth_0x34;
}

/* The selected row; also published to the sound unit through `unsplit/lobby.h`. */
s32 getItemListSelection(void)
{
    return lb_item_list_state.selected_0x38;
}

/* Selects a row and remembers whether the selection actually moved. */
void fn_8021F248(s32 selected)
{
    s32 prev = lb_item_list_state.selected_0x38;
    u8 masked = selected;

    lb_item_list_state.selected_0x38 = masked;
    if (prev == masked) {
        lb_item_list_state.changed_0x3C = 0;
    } else {
        lb_item_list_state.changed_0x3C = 1;
    }
}

/* True on the frame the selection changed. */
s32 fn_8021F27C(void)
{
    return lb_item_list_state.changed_0x3C - 1 == 0;
}

/* Acknowledges the selection change. */
void fn_8021F298(void)
{
    lb_item_list_state.changed_0x3C = 0;
}

/* The two-argument row draw the three table variants share. */
void fn_8021FCD0(u8 a, s32 b, u16 c)
{
    fn_8021FCE0(a, b, c, 1);
}

/* One shared sprite row: the layout entry plus its row table.  (peephole on: see fn_8021E304.) */
#pragma peephole on

#pragma peephole off

/* The page's tick: the row count as a float and the per-tick step, then the changed flag. */
void syncItemListClock(u32 stamp, u32 count, f32 depth)
{
    lb_item_list_state.count_0x14 = count;
    lb_item_list_state.step_0x18 = lbl_80799C78 * count;
    lb_item_list_state.scroll_0x30 = depth;
    lb_item_list_state.depth_0x34 = depth;
    frame_counter = stamp;
    lb_item_list_state.stamp_0x04 = stamp;
    if ((f32)depth == lbl_80799C7C) {
        lb_item_list_state.selected_0x38 = 1;
    } else {
        lb_item_list_state.selected_0x38 = 0;
    }
    lb_item_list_state.changed_0x3C = 0;
    lb_item_list_state.events_0x48 = (s8)CalculateEvents();
    lb_item_list_state.active_0x00 = 0;
}       /* +0x806AA8C8 */
LbMenuItemState lb_item_list_state;  /* +0x806AAA88 */
         /* +0x806AAAD8: the record `.data` 0x805BA490 points at (GUESS name) */
         /* +0x806AAAF0: the record `.data` 0x805BA530 points at (GUESS name) */
             /* +0x806AAB08: four fixed positions `fn_8021FF5C` sets (GUESS name) */
              /* +0x806AAB38: a fifth fixed position (GUESS name) */
             /* +0x806AAB44 */

}  /* extern "C" */
