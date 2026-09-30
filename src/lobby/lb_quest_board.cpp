/* lobby/lb_quest_board.cpp - the lobby's quest-board screen: its step machine, its cursor, the
 * panel frame it draws into and the effects it spawns.  `.text` 0x80394038..0x803967F0, 41
 * functions / 10168 B.
 *
 * MODULE AND FILE NAME (brief section 2, evidence order).  1. No `__FILE__` string covers the range:
 * every `lis`/`addi` pair in it resolves to the `.sdata2` float pool, a switch table or one of the
 * band's own tables (`xref` over the whole DOL text; the only nearby source-name literal,
 * `s_menu_note_cpp` at `.data` 0x805E91F8, has exactly one referrer, 0x8034C1B4, inside the
 * registered `menu/menu_note.cpp` unit).  2. `dumpmap.py lookup` answers `zz_XXXXXXXX_` for every
 * address except 0x80394DA4, where the map already carries the real name `draw_quest_board`.
 * 3. The code places the unit in `lobby`: it reads the lobby work block `lobby_w` (.bss 0x806AAB44)
 * 42 times, calls the lobby string table (`LbStr`), the lobby NPC motion query
 * (`lb_npc_Get_motion_no`), the lobby scene's helpers (`get_now_areano`, `get_move_work_adrs`,
 * `get_option_cfg`) and the lobby/HUD 2D layer (`get_lsp_data`, `draw_sprite_*`, `draw_font_idx`,
 * `font_flush`, `set_zmode`, `set_blendmode`); the module's own units (`lobby/lb_npc.cpp`,
 * `lobby/lb_menu_page.cpp`, `lobby/lb_companion_ui.cpp`) carry the same call set.  The file name is
 * derived from the range's own entry point `draw_quest_board` and its dominant content, in the
 * `lb_*` scheme of the module's named units - MARKED GUESS: the original file name is not
 * recoverable, so `lb_quest_board.cpp` is the best descriptive fit.
 *
 * SECTIONS.  `.text` 0x80394038..0x803967F0; `extab` 0x8001833C..0x8001843C (32 records) and
 * `extabindex` 0x80038340..0x800384C0 (32 x 12 B) - both runs are exactly the gap the bracketing
 * split objects leave, and both tile in link order with the neighbours' (the run below ends at
 * 0x8001833C / 0x80038340).  The unit's own `.data` run is 0x805F13D0..0x805F1500: `lbl_805F13D0`
 * (0x30) and `jumptable_805F1400` (0x3C) are read by 0x80394038 / 0x80394200, the tables at
 * 0x805F143C..0x805F14A4 and 0x805F14B0 / 0x805F14D8 / 0x805F14EC by `draw_quest_board`,
 * 0x803950E0 and 0x80396070; 0x805F13C0 (a pointer pair) belongs to the unit below and 0x805F1500
 * to the one above, both measured by referrer address.  `.sdata` 0x80793470..0x80793488 and
 * `.sdata2` 0x8079C2B4..0x8079C2EC are the same story.  NONE of them is emitted by the source yet -
 * they are residual, see STATUS.
 *
 * SEAM.  Unproven, and consistent with one TU: the `.sdata2` label runs of the band are ordered by
 * their referrer's `.text` address across both edges (0x8079C2A8/0x8079C2B0 -> 0x80393B28/0x80393D4C
 * below, 0x8079C2B4..0x8079C2E8 -> this range, 0x8079C2EC.. -> 0x803967F0 above), and the same
 * holds for `.data` (0x805F13B8 -> 0x80393D4C below, 0x805F13D0..0x805F14EC -> this range,
 * 0x805F1500 -> 0x80396CAC above) and `.sdata` (0x8079346C -> 0x803933E8 below,
 * 0x80793470..0x80793480 -> this range, 0x80793488 -> 0x8039AFBC above).  The attribution pass's
 * "candidate seam inside it was not taken" note for this proposal is the `.sdata2` *shared* words
 * 0x8079C2C8/0x8079C2E0 (each also referenced by 0x804BF530 / 0x8027D968), which defeat the
 * single-referrer pair test; the other words' owners rise monotonically, so no internal seam exists.
 *
 * Naming note: references only to OTHER units' unrenamed fn_XXXXXXXX symbols - every `fn_` this
 * file names (`fn_80396070`, `fn_80396248`, `fn_8039631C`, `fn_803963F4`, `fn_80396654`,
 * `fn_803967F0`, `fn_80396934`, `fn_80396944`, `fn_80395DF4`, `fn_8004DF10`, `sysSE_stop`,
 * `fn_80214EF0`, `fn_80215170`, `fn_803772A8`, `fn_803B7154`, `fn_804338E0`) is still
 * `fn_XXXXXXXX` in the map and none is a row this unit defines (checked with
 * `python tools/symbols/symedit.py range 0x80394000 0x80397000` against the 18 renamed rows this
 * file owns).
 *
 * NAMES.  Every name below is derived from the body (what it stores, compares, passes on) and the
 * `lb_*` scheme; each is marked GUESS in the function's own comment.  The map rows were renamed with
 * `tools/symbols/symedit.py rename` in the same commit (playbook 31/48).
 *
 * STATUS / RESIDUALS.  See the outbox `config_requests` for the measured numbers.  The 25 functions
 * that are not written yet, with their sizes (B), are: 80394038/268, 80394200/1216, 80394758/348,
 * 803948C8/296, 80394A64/368, 80394BD4/464, draw_quest_board/828, 803950E0/640, 80395360/224,
 * 80395440/188, 803954FC/420, 803957EC/292, 80395910/160, 803959C8/132, 80395A4C/568,
 * 80395C84/128, 80395D04/240, 80395DF4/292, 80395F18/96, 80396248/212, 8039631C/180,
 * 803963F4/608, 80396654/412.  Their bodies are the draw/update passes whose struct views
 * (`LbQuestBoardWork`'s payload and the `lobby_w` screen block) are not yet reconstructed.
 */

#include "types.h"
#include "lobby/lb_quest_board.h"
#include "menu/arena_result.h"   /* `quest_record_find` (the owner's header, rule 2) */
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
void fn_803967F0(LbQuestBoardWork* work);
void fn_80396934(LbQuestBoardWork* work);
void fn_80396944(LbQuestBoardWork* work);
}

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* Advances the work's step byte.  GUESS: the step is what the two dispatchers below switch on, but
 * nothing in the range calls this body, so the caller that owns its meaning is outside the unit. */
extern "C" void lb_quest_board_state_next(LbQuestBoardWork* work) {
    work->state_0x005++;
}

/* Retires one pooled effect runtime record.  GUESS: a forwarding thunk to the effect resource
 * helper of the same name, mirroring the band's other one-line forwards. */
extern "C" void lb_quest_board_effect_retire(void* work) {
    fn_800F886C(work);
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

/* Leaves the board screen: clears the selector and the screen's active byte.  GUESS: `close` rather
 * than `cancel` is arbitrary between this body and `lb_quest_board_cancel`, which are byte-identical
 * - the two callers that tell them apart are outside the unit. */
extern "C" void lb_quest_board_close(void) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
}

/* Moves a row cursor with wrap-around, playing the move sound only while the row the pressed mask
 * matches is still the one the two direction masks select.  The caller passes the cursor, the row
 * count and three masks.  `#pragma peephole off`: retail keeps the unfused `and` + `cmpwi` of the
 * mask test and the unfused `subi` + `cmpwi` of the wrap-around test, which the peephole folds into
 * `and.` / `subic.` (playbook 39). */
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

/* Reports whether the board accepts input right now: the selected row's label must have a live
 * entry and, if it does, the row must be the one the payload marks as current.  GUESS: the polarity
 * and the row/label semantics come from the two callees' own names. */
extern "C" bool lb_quest_board_accept_input(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    QuestRecord* row = quest_record_find(*fn_804338E0(work->index_0x249));
    u32 current;

    if (row == NULL) {
        return true;
    }
    current = fn_803B7154(row, work->value_0x38C, work->data_0x264->field_0x014, work->data_0x264);
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
        fn_80396934(work);
        return;
    case 3:
        fn_80396944(work);
        return;
    }
}

/* Runs the effect kind's own first step, the step `lb_quest_board_step` enters the screen with.
 * GUESS: the map merged this body into `lb_quest_board_step`'s symbol row (the row covered both
 * functions' 0x60 bytes); it was split with the registration (see the unit header). */
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
        fn_803967F0(work);
        return;
    default:
        fn_803963F4(work);
        return;
    }
}
