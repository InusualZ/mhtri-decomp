/* lobby/lb_menu_pos_tbl.cpp - the lobby menu's fixed position table, the item list's clock, the page state records and
 *   the lobby work block.
 * RANGE. .text 0x8021F020-0x80220038 (20 functions); .ctors 0x8056F364-0x8056F368, .data 0x805BA650-0x805BA660, .bss
 *   0x806AAA88-0x806AACC0 (`lb_item_list_state`, `lb_page_state_0/1`, `lb_menu_pos_tbl`, `lb_menu_pos_extra`,
 *   `lobby_w`, whose record `LbLobbyWork` is in `lobby/lobby_work.h`), .sdata2 0x80799C78-0x80799CD8, extab,
 *   extabindex.  Between `lobby/lb_menu_scratch.cpp` and `lobby/lb_equip_page.cpp`; the three share
 *   `lobby/fn_8021E1EC.h`.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps the narrowing `clrlwi`s; docs/lobby.md).
 * NAMES. `lb_menu_pos_tbl` is the position table the static constructor `fn_8021FF5C` fills; `lb_page_state_0/1` (the
 *   records `.data` 0x805BA4B0/0x805BA530 point at) and `lb_menu_pos_tbl`/`lb_menu_pos_extra` are GUESSes.
 *   GUESS (from each body and its callers): lb_event_schedule_step
 * RESIDUALS. 12 rows unwritten: `lb_event_schedule_step` (0x8021F0DC-0x8021F218), 0x8021F2AC-0x8021FCD0, 0x8021FCE0-0x80220038
 *   (the constructor `fn_8021FF5C` among them, which `setVec3`s the five positions and calls `fn_8021FFFC(&lobby_w)`);
 *   only `lb_item_list_state` of the `.bss` claim is defined.
 *  - `syncItemListClock`: retail keeps no third callee-saved register (ours saves r29 for the surviving argument) and
 *    emits `frsp f1,f1` + `fcmpo`/`cror` for `depth == lbl_80799C7C` where ours emits `fcmpu`;
 *  - `fn_8021F248`: register colouring at instructions 2-5 (the old value in r5 against ours r0).
 *   flipcheck: `.bss` 0x50 against 0x238; `.ctors`/`.data` claimed, not emitted; `.sdata2` 0x8 against 0x60;
 *   `.text`/extab/extabindex short of the claim.
 * SHAPES. `syncItemListClock` takes `count` as `u32`: retail converts it unsigned (no `xoris`).
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

/* Retail keeps the narrowing `clrlwi` the peephole pass folds away. */
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
}

/* The one `.bss` object of the claim this unit defines, after every use like the target. */
LbMenuItemState lb_item_list_state;  /* .bss 0x806AAA88 */

}  /* extern "C" */
