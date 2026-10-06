/* lobby/lb_quest_board.cpp - the lobby's quest-board screen: its step machine, its cursor, the panel frame it draws
 *   into and the effects it spawns.
 * RANGE. .text 0x80394038-0x803967F0 (42 functions); .data 0x805F13D0-0x805F1500 (`lbl_805F13D0`, `jumptable_805F1400`
 *   and the tables `draw_quest_board`, 0x803950E0 and 0x80396070 read), .sdata 0x80793470-0x80793488, .sdata2
 *   0x8079C2B0-0x8079C2E8, extab, extabindex.  The data order is consistent with one TU (docs/lobby.md).
 * FLAGS. `cflags_lobby`; `#pragma peephole off` around `lb_quest_board_cursor_step` (retail keeps `and` + `cmpwi` and
 *   `subi` + `cmpwi` unfused; playbook 39).
 * NAMES. `lb_quest_board` is a GUESS from the range's one real map name, `draw_quest_board` (0x80394DA4), in the
 *   module's `lb_*` scheme; every other name is a GUESS from its body.  Module `lobby`: 42 `lobby_w` reads, `LbStr`,
 *   `lb_npc_Get_motion_no`, `get_now_areano`, `get_move_work_adrs`, `get_option_cfg` and the lobby/HUD 2D layer.
 * RESIDUALS. 25 rows unwritten: 0x80394038-0x80394144, 0x80394200-0x803946C0, 0x80394758-0x803948B4,
 *   0x803948C8-0x803949F0, 0x80394A64-0x803956A0, 0x803957EC-0x803959B0, 0x803959C8-0x80395F80, 0x80396070-0x803963D0,
 *   0x803963F4-0x803967F0 (they need `LbQuestBoardWork`'s payload and the `lobby_w` screen block).  flipcheck:
 *   `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of the claim; the
 *   `.sdata`/`.sdata2` pool is shared with `lobby/lb_quest_ui.cpp` and `menu/menu_result.cpp` (fold candidate).
 *   `lb_quest_board_step` hands its work to `menu/menu_result.cpp`'s `q_result_anim_counter_inc`/
 *   `q_result_release_effect` as a `QResultScreen` (one block, two views: a cast until the views are merged).
 */

#include "types.h"
#include "lobby/lb_quest_board.h"
#include "quest/quest_entry.h"   /* `quest_record_find` (the owner's header, rule 2) */
#include "Network/net_session_close.h"   /* `getProfileQuestRecord` (the owner's header, rule 2) */
#include "menu/menu_result.h"   /* `q_result_*` (the owner's header, rule 2) */
#include "enemy/em_pop.h"   /* `quest_ex_condition_ck` (the owner's header, rule 2) */
#include "Runtime.PPCEABI.H/memset.h"

/* Declarations of this range's own symbols that are still unwritten (`fn_` in the map), so the
 * written bodies can call them.  They are `extern "C"` because the map spells them unmangled
 * (playbook 42). */
extern "C" {
void lb_quest_board_effect_push_models(LbQuestBoardWork* work);
void lb_quest_board_effect_retire_list(LbQuestBoardWork* work);
void fn_80396070(LbQuestBoardWork* work);
void fn_80396248(LbQuestBoardWork* work);
void fn_8039631C(LbQuestBoardWork* work);
void lb_quest_board_effect_update(LbQuestBoardWork* work);
void lb_quest_board_step_kind(LbQuestBoardWork* work);
void fn_803963F4(LbQuestBoardWork* work);
void fn_80396654(LbQuestBoardWork* work);
}

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* Advances the work's step byte.  GUESS: the step is what the two dispatchers below switch on, but
 * nothing in the range calls this body, so the caller that owns its meaning is outside the unit. */
extern "C" void lb_quest_board_state_next(LbQuestBoardWork* work) {
    work->state_0x005++;
}

/* Retires one pooled effect runtime record.  GUESS: a forwarding thunk to the effect resource
 * helper of the same name, mirroring the band's other one-line forwards. */
extern "C" void lb_quest_board_effect_retire(LbQuestBoardWork* work) {
    eft_res_slot_release(work);
}

/* Opens the board screen: clears the whole screen block, files the payload it was opened with and
 * puts the lobby into screen 19. */
extern "C" void lb_quest_board_open_board(LbQuestBoardData* data) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    u32 value_b;
    u32 value_a;

    memset(work, 0, 0x2000);
    work->data_0x264 = data;
    lobby_w.active_0x008 = 1;
    lobby_w.state_0x000 = 19;
    lobby_w.value_0x082 = 0;
    sysSE_stop(32);
    fn_8004DF10(&work->value_0x38C, &value_a, &value_b);
}

/* Leaves the board screen: clears the selector and the screen's active byte.  GUESS: `close` against the byte-identical
 * `lb_quest_board_cancel` is arbitrary (the callers that tell them apart are outside the unit). */
extern "C" void lb_quest_board_close(void) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
}

/* Moves a row cursor with wrap-around, playing the move sound while the pressed row is the selected one. */
#pragma peephole off
extern "C" s32 lb_quest_board_cursor_step(s32 index, s32 count, u32 current, u32 next, u32 prev) {
    if (count > 1) {
        if (((u16)current & (u16)next) != 0) {
            sysSE_stop(33);
            index = index - 1;
            if (index < 0) {
                index = count - 1;
            }
        } else if (((u16)current & (u16)prev) != 0) {
            sysSE_stop(33);
            index = index + 1;
            if (index >= count) {
                index = 0;
            }
        }
    }
    return index;
}
#pragma peephole on

/* Puts the row cursor back on the first row of a four-row page. */
extern "C" void lb_quest_board_cursor_reset(LbQuestBoardWork* work) {
    work->cursor_0x24E = 0;
    work->count_0x250 = 4;
}

/* Whether the board accepts input: the selected row's label has a live entry and the row is the payload's current one.
 * GUESS: the polarity and the row/label semantics come from the two callees' names. */
extern "C" bool lb_quest_board_accept_input(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    QuestRecord* row = quest_record_find(*getProfileQuestRecord(work->index_0x249));
    u32 current;

    if (row == NULL) {
        return true;
    }
    current = quest_ex_condition_ck(row, work->value_0x38C, work->data_0x264->field_0x014, work->data_0x264);
    return current != 1;
}

/* Opens the notice screen (screen 20) - the same setup as the board, minus the payload lookup, with
 * the menu's cancel sound. */
extern "C" void lb_quest_board_open_notice(LbQuestBoardData* data) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    memset(work, 0, 0x2000);
    work->data_0x264 = data;
    lobby_w.active_0x008 = 1;
    lobby_w.state_0x000 = 20;
    lobby_w.value_0x082 = 0;
    sysSE_req(0);
}

/* Sets the 2D state the board draws in and pushes its two panel labels. */
extern "C" void lb_quest_board_begin_panel(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    fn_80214EF0(0x1877, 0x185);
    fn_80215170(0x1879, work->field_0x258);
}

/* Undoes `lb_quest_board_begin_panel`: clears the panel-pending bit on the payload and on the
 * lobby block, then lets the payload hand its two bytes on. */
extern "C" void lb_quest_board_end_panel(LbQuestBoardWork* work) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
    work->data_0x264->flags_0xB01 &= ~4;
    fn_803772A8(work->data_0x264->flags_0xB01, work->data_0x264->param_0xB02);
    lobby_w.flags_0x052 &= ~4;
}

/* Leaves the notice screen.  GUESS: see `lb_quest_board_close`, whose body this repeats byte for
 * byte; the map carries both addresses. */
extern "C" void lb_quest_board_cancel(void) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
}

/* Releases whatever the screen's effect kind built: the 8/9 kinds own a model list, every other
 * kind a single record. */
extern "C" void lb_quest_board_effect_release(LbQuestBoardWork* work) {
    if ((u32)(work->kind_0x002 - 8) <= 1) {
        lb_quest_board_effect_retire_list(work);
    } else {
        lb_quest_board_effect_push_models(work);
    }
}

/* Returns the effect kind's model records to the effect-model pool and empties the list. */
extern "C" void lb_quest_board_effect_push_models(LbQuestBoardWork* work) {
    LbEftList* list = work->eft_0x038;

    push_eft_effect_heap_num(list->effects_0x004, list->count_0x000);
    list->count_0x000 = 0;
}

/* Retires the effect kind's whole handle list and empties it. */
extern "C" void lb_quest_board_effect_retire_list(LbQuestBoardWork* work) {
    LbEftList* list = work->eft_0x038;

    fn_800F8A44(list->effects_0x004, list->count_0x000);
    list->count_0x000 = 0;
}

/* Runs the screen's current step, dispatching to the updater that owns it; the first step further
 * splits on the effect kind the screen was opened with. */
extern "C" void lb_quest_board_step(LbQuestBoardWork* work) {
    switch (work->state_0x005) {
    case 0:
        lb_quest_board_step_kind(work);
        return;
    case 1:
        lb_quest_board_effect_update(work);
        return;
    case 2:
        q_result_anim_counter_inc((QResultScreen*)work);
        return;
    case 3:
        q_result_release_effect((QResultScreen*)work);
        return;
    }
}

/* Runs the effect kind's own first step, the one `lb_quest_board_step` enters the screen with.  GUESS name; the map row
 * is split out of `lb_quest_board_step`'s 0x60 bytes. */
extern "C" void lb_quest_board_step_kind(LbQuestBoardWork* work) {
    switch (work->kind_0x002) {
    case 8:
        fn_80396248(work);
        return;
    case 9:
        fn_8039631C(work);
        return;
    default:
        fn_80396070(work);
        return;
    }
}

/* Updates the effect the screen holds, by effect kind. */
extern "C" void lb_quest_board_effect_update(LbQuestBoardWork* work) {
    switch (work->kind_0x002) {
    case 8:
        fn_80396654(work);
        return;
    case 9:
        q_result_effect_follow_npc((QResultScreen*)work);
        return;
    default:
        fn_803963F4(work);
        return;
    }
}
