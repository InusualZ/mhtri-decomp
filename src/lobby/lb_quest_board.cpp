/* lobby/lb_quest_board.cpp - the lobby's quest-board screen: its step machine, its cursor, the panel frame it draws
 *   into and the effects it spawns.
 * RANGE. .text 0x80394038-0x803967F0 (42 functions); .data 0x805F13D0-0x805F1500 (the camera key table,
 *   `jumptable_805F1400` and the sprite-id tables), .sdata 0x80793470-0x80793488, .sdata2 0x8079C2B0-0x8079C2E8, extab,
 *   extabindex.  The range holds three TUs' worth of code: `lb_scene_model_slide` (0x80394038, called only from
 *   `lobby/lb_quest_ui.cpp`'s `lb_scene_eft_move`, with the key table and the `.sdata2` run 0x8079C2B0-0x8079C2D0) and
 *   the two helpers after it (called from `lb_scene_eft_step`) close `lobby/lb_quest_ui.cpp`'s TU; the board screen runs
 *   0x80394158-0x80395D04 (its own int->float double at 0x8079C2D8); the `_EFT` effect band from 0x80395D04 continues
 *   into `menu/menu_result.cpp`'s first three rows (seam requests filed).
 * FLAGS. `cflags_lobby`; `#pragma pool_data off` (retail materialises each table with its own `lis`/`addi`);
 *   `#pragma peephole off` file-wide (retail keeps `and`/`subi`/`extsb` + `cmpwi` unfused; playbook 39) except
 *   `lb_quest_board_step_screen` and `lb_quest_board_effect_spawn`, which measure better with it on.
 * NAMES. `lb_quest_board` is a GUESS from the range's one real map name, `draw_quest_board` (0x80394DA4), in the
 *   module's `lb_*` scheme; every other name is a GUESS from its body.  Module `lobby`: 42 `lobby_w` reads, `LbStr`,
 *   `lb_npc_Get_motion_no`, `get_now_areano`, `get_move_work_adrs`, `get_option_cfg` and the lobby/HUD 2D layer.
 *   GUESS: `lb_scene_model_slide`, `lb_quest_board_state_next`, `lb_quest_board_effect_retire`,
 *   GUESS: `lb_quest_board_step_screen`, `lb_quest_board_list_input`, `lb_quest_board_detail_input`,
 *   GUESS: `lb_quest_board_accept_input`, `lb_quest_board_name_input`, `lb_quest_board_enter_step`,
 *   GUESS: `lb_quest_board_summary_draw`, `lb_quest_board_detail_draw`, `lb_quest_board_panel_draw`,
 *   GUESS: `lb_quest_board_draw_task`, `lb_quest_board_ready_step`, `lb_quest_board_open_ready`,
 *   GUESS: `lb_quest_board_ready_panel_draw`, `lb_quest_board_reset`, `lb_quest_board_effect_spawn`,
 *   GUESS: `lb_quest_board_model_spawn`, `lb_quest_board_flash_spawn`, `lb_quest_board_follow_spawn`,
 *   GUESS: `lb_quest_board_effect_release`, `lb_quest_board_effect_push_models`,
 *   GUESS: `lb_quest_board_effect_retire_list`, `lb_quest_board_step`, `lb_quest_board_step_kind`,
 *   GUESS: `lb_quest_board_flash_init`, `lb_quest_board_follow_init`, `lb_quest_board_effect_update`,
 *   GUESS: `lb_quest_board_flash_move`
 * RESIDUALS. Unwritten: `lb_quest_board_effect_init` (0x80396070, 0x1D8: `SetRootMtxTrans` has no declaration in its
 *   owner's header) and `lb_quest_board_effect_move` (0x803963F4, 0x260: `ef/effect.cpp`'s `fn_800F996C` is unnamed and
 *   the ef lanes own it).  flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short
 *   of the claim; the pools are three TUs' (see RANGE).  Unwritten: `lb_quest_board_party_step` (0x80395A4C, 0x238):
 *   it tests the results of `requestReadyOnAlias`/`requestReadyOnAlias2`, which the Network owner's header declares
 *   `void` (decl request filed).  `lb_quest_board_step_screen`: retail shares the case-2/case-3 error tails
 *   (`step = 2; wait = 30`) after the first error branch; the if/else-chain and duplicated-tail spellings both lose.
 *   `lb_quest_board_draw_task`: retail reaches `lb_quest_board_detail_draw` from two call sites, ours from three (the
 *   range test plus cases 14 and 5); the one-switch spellings that share the call measure lower (91-94 %).
 */

#pragma pool_data off
#pragma peephole off

#include "types.h"
#include "lobby/lb_quest_board.h"
#include "quest/quest_entry.h"   /* `quest_record_find` (the owner's header, rule 2) */
#include "Network/net_session_close.h"   /* `getProfileQuestRecord` (the owner's header, rule 2) */
#include "menu/menu_result.h"   /* `q_result_*` (the owner's header, rule 2) */
#include "enemy/em_pop.h"   /* `quest_ex_condition_ck` (the owner's header, rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/LbStr.h"
#include "lobby/lb_panel_msg_draw.h"
#include "lobby/lb_npc.h"
#include "hud/layout.h"
#include "font/flfnt.h"
#include "menu/menu_message.h"
#include "menu/menu_item.h"
#include "menu/fn_8031EA8C.h"
#include "menu/get_pop_dat_ptr.h"
#include "hud/cockpit.h"
#include "camera/camera.h"
#include "camera/camera_frame_get.h"
#include "fn_8004CAD8.h"
#include "main.h"
#include "mh3_pad/system_w.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef/get_move_work_adrs.h"
#include "ef/eft052.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800DD1F0.h"
#include "fn_80040598.h"
#include "stage/stg_w.h"
#include "mh3_pad/vec3.h"
#include "MSL_C/alloc.h"


void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* The camera model's key-frame table `lb_scene_model_slide` reads: six (frame, value) pairs. */
f32 lb_quest_detail_camera_keys[12] = {
    0.0f, -3160.0f, 82.0f, -3080.9f, 144.0f, -3035.2f, 212.0f, -2965.3f, 310.0f, -2960.0f, -1.0f, -2960.0f,
};
u16 lb_quest_board_frame_ids[14] = {
    0x1646, 0x1647, 0x1640, 0x1641, 0x1642, 0x1643, 0x1644, 0x1645, 0x163E, 0x163F, 0xFFFF, 0x0000, 0x0000, 0x0000,
};
u16 lb_quest_board_row_ids[12] = {
    0x165E, 0x165F, 0x1660, 0x1661, 0x1662, 0x1663, 0x1664, 0x1665, 0x1666, 0x1657, 0x1654, 0xFFFF,
};
u16 lb_quest_board_entry_ids[10] = {0x1658, 0x165A, 0x165B, 0x165C, 0x165D, 0x164B, 0x164C, 0x1655, 0x1656, 0xFFFF};
u16 lb_quest_board_shade_ids[10] = {0x1667, 0x1668, 0x1669, 0x166A, 0x166B, 0x166C, 0x166D, 0x166E, 0x166F, 0xFFFF};
u16 lb_quest_board_row_lsp[6] = {0x0000, 0x1670, 0x1671, 0x1672, 0x1673, 0x0000};
char lb_quest_board_blank_name[11] = "          ";
u16 lb_quest_board_summary_ids[20] = {
    0x1684, 0x1685, 0x1686, 0x1687, 0x1688, 0x1689, 0x168A, 0x168B, 0x168C, 0x1675,
    0x1676, 0x1677, 0x167F, 0x1680, 0x1681, 0x1682, 0x1683, 0x168D, 0x168E, 0xFFFF,
};
u16 lb_quest_board_effect_groups[10] = {0x06A1, 0x06A2, 0x06A3, 0x06A4, 0x06A5, 0x06A6, 0x06A7, 0x0630, 0x00FF, 0x00FF};
u16 lb_quest_board_effect_ids[10] = {0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x0018, 0x00FF, 0x00FF};

u16 lb_quest_board_empty_ids[2] = {0x1659, 0xFFFF};
u16 lb_quest_board_label_ids[4] = {0x1692, 0x1693, 0x1694, 0xFFFF};
u16 lb_quest_board_label_lsp[4] = {0x1695, 0x1696, 0x1697, 0x1697};

/* The model record `lb_scene_eft_move` hands `lb_scene_model_slide`: its step byte and the position the
 * camera keys drive.  size: 0x10 (a view: the model continues) */
typedef struct LbQuestDetailModel {
    /* +0x00 */ u8 step;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ VEC3 pos;
} LbQuestDetailModel; /* size: 0x10 */

/* Declarations of this range's own symbols, so the bodies can stay in address order.  They are `extern "C"`
 * because the map spells them unmangled (playbook 42). */
extern "C" {
u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index);
void lb_quest_board_close(void);
s32 lb_quest_board_cursor_step(s32 index, s32 count, u32 current, u32 next, u32 prev);
void lb_quest_board_cursor_reset(LbQuestBoardWork* work);
s32 lb_quest_board_accept_input(void);
void lb_quest_board_begin_panel(void);
void lb_quest_board_end_panel(LbQuestBoardWork* work);
void lb_quest_board_cancel(void);
_EFT* lb_quest_board_model_spawn(_LB_NPC* npc, s8 kind);
void lb_quest_board_step_screen(void);
s32 lb_quest_board_list_input(LbQuestBoardWork* work);
s32 lb_quest_board_detail_input(LbQuestBoardWork* work);
s32 lb_quest_board_name_input(LbQuestBoardWork* work);
s32 lb_quest_board_enter_step(LbQuestBoardWork* work);
void draw_quest_board(LbQuestBoardWork* work);
void lb_quest_board_summary_draw(LbQuestBoardWork* work);
void lb_quest_board_detail_draw(LbQuestBoardWork* work);
void lb_quest_board_panel_draw(LbQuestBoardWork* work);
void lb_quest_board_draw_task(void);
void lb_quest_board_ready_panel_draw(void);
void lb_quest_board_effect_push_models(_EFT* self);
void lb_quest_board_effect_retire_list(_EFT* self);
void lb_quest_board_step(_EFT* self);
void lb_quest_board_step_kind(_EFT* self);
void lb_quest_board_effect_init(_EFT* self);
void lb_quest_board_flash_init(_EFT* self);
void lb_quest_board_follow_init(_EFT* self);
void lb_quest_board_effect_update(_EFT* self);
void lb_quest_board_effect_move(_EFT* self);
void lb_quest_board_flash_move(_EFT* self);
void lb_quest_board_effect_release(_EFT* self);
}

/* 0x80394038 (0x10C): Steps one of the scene effect's models: it waits for the lobby's camera mode 2, slides back when
 * the camera work stops, and for model 3 follows the key table by the camera's frame; 1 for model 4. */
extern "C" u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index) {
    switch (model->step) {
    case 0:
        if (lobby_w.unused_0x001[2] == 2) {
            model->step++;
            setVector3(&model->pos, -3160.0f, -40.0f, 370.0f);
        }
        /* fall through */
    case 1:
        if (camera_work_ck() == 0) {
            setVector3(&model->pos, -2960.0f, -40.0f, 370.0f);
            model->step = 0;
            return 0;
        }
        if (index == 3) {
            model->pos.x = getKeyData(lb_quest_detail_camera_keys, (f32)(camera_frame_get(1) * 2));
        } else if (index == 4) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80394144 (0x10): Advances the work's step byte.  GUESS: the step is what the two dispatchers below switch on, but
 * nothing in the range calls this body, so the caller that owns its meaning is outside the unit. */
extern "C" void lb_quest_board_state_next(_EFT* self) {
    self->state_0x05++;
}

/* 0x80394154 (0x4): Retires one pooled effect runtime record. */
extern "C" void lb_quest_board_effect_retire(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x80394158 (0x90): Opens the board screen: clears the whole screen block, files the payload it was opened with and
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
    userdata_hunter_rank_info_get(&work->value_0x38C, &value_a, &value_b);
}

/* 0x803941E8 (0x18): Leaves the board screen: clears the selector and the screen's active byte.  GUESS: `close`
 * against the byte-identical `lb_quest_board_cancel` is arbitrary (the callers that tell them apart are outside the
 * unit). */
extern "C" void lb_quest_board_close(void) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
}

#pragma peephole on
/* 0x80394200 (0x4C0): The board screen's frame step: the list, the detail page, the yes/no, the name keyboard and the
 * profile enter, each answering a step to move to; then the draw callback. */
extern "C" void lb_quest_board_step_screen(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    LbQuestBoardEntry* entry = (LbQuestBoardEntry*)getProfileQuestRecord(work->index_0x249);
    NetProfileRec* profile = &refreshAllProfiles()[work->index_0x249];

    work->page_moved_0x25C = 0;
    switch (work->step_0x242) {
    case 0:
        work->step_0x242 = 1;
        work->message_0x25E = 0xFF;
        work->field_0x24C = 1;
        work->page_0x24A = 0;
        work->rows_0x252 = 4;
        work->anim_0x260 = 8;
        break;
    case 1:
        switch (lb_quest_board_list_input(work)) {
        case 1:
            if (getProfileQuestRecord(work->index_0x249) != NULL) {
                work->step_0x242 = 2;
                lb_quest_board_cursor_reset(work);
            }
            break;
        case 2:
            work->step_0x242 = 10;
            break;
        }
        break;
    case 2:
        if (entry == NULL) {
            NetCtrlWk::setSubError(10);
            work->step_0x242 = 1;
            sysSE_req(1);
        } else {
            switch (lb_quest_board_detail_input(work)) {
            case 1:
                if (lobby_w.flags_0x052 != 0) {
                    NetCtrlWk::setSubError(26);
                } else if (profile->flag_0x08D == 1) {
                    NetCtrlWk::setSubError(25);
                } else if (entry->players <= profile->active_0x040) {
                    NetCtrlWk::setSubError(24);
                } else if (lb_quest_board_accept_input() == 1) {
                    NetCtrlWk::setSubError(27);
                } else {
                    work->step_0x242 = 3;
                    work->field_0x258 = 0;
                    break;
                }
                work->step_0x242 = 2;
                work->wait_0x3A4 = 30;
                break;
            case 2:
                work->wait_0x3A4 = 0;
                work->step_0x242 = 1;
                break;
            }
        }
        break;
    case 3:
        if (entry == NULL) {
            NetCtrlWk::setSubError(10);
            work->step_0x242 = 1;
            sysSE_req(1);
            break;
        }
        if (profile->flag_0x08D == 1) {
            NetCtrlWk::setSubError(25);
        } else if (entry->players <= profile->active_0x040) {
            NetCtrlWk::setSubError(24);
        } else if (lb_quest_board_accept_input() == 1) {
            NetCtrlWk::setSubError(27);
        } else {
            switch (lb_yes_no_step(&work->field_0x258)) {
            case 1:
                work->step_0x242 = 4;
                break;
            case 2:
                work->step_0x242 = 2;
                break;
            }
            break;
        }
        work->step_0x242 = 2;
        work->wait_0x3A4 = 30;
        sysSE_req(1);
        return;
    case 4:
        if (entry == NULL) {
            NetCtrlWk::setSubError(10);
            work->step_0x242 = 1;
            sysSE_req(1);
        } else if (lb_quest_board_accept_input() == 0) {
            getProfileQuestRecord(work->index_0x249);
            if (refreshAllProfiles()[work->index_0x249].rank_0x08C == 1) {
                work->renamed_0x3A2 = 1;
                work->step_0x242 = 5;
                work->kbd_step_0x245 = 0;
                memset(work->name_0x26B, 0, 5);
                memset(work->kbd_buf_0x398, 0, 10);
            } else {
                work->renamed_0x3A2 = 0;
                work->step_0x242 = 6;
            }
        } else {
            work->step_0x242 = 11;
            work->return_step_0x246 = 2;
        }
        break;
    case 5:
        if (entry == NULL) {
            NetCtrlWk::setSubError(10);
            work->step_0x242 = 1;
            sysSE_req(1);
        } else {
            switch (lb_quest_board_name_input(work)) {
            case 1:
                work->step_0x242 = 6;
                break;
            case 2:
                work->step_0x242 = 2;
                sysSE_req(1);
                break;
            case 3:
                work->step_0x242 = 1;
                break;
            }
        }
        break;
    case 6:
        work->step_0x242 = 7;
        work->enter_step_0x243 = 0;
        work->enter_error_0x244 = 0;
        break;
    case 7:
        switch (lb_quest_board_enter_step(work)) {
        case 1:
            work->step_0x242 = 8;
            sysSE_stop(30);
            quest_board_accept(work->data_0x264, work->quest_id_0x254, 2);
            break;
        case 3:
            work->step_0x242 = 11;
            work->return_step_0x246 = 2;
            break;
        }
        break;
    case 14:
        work->step_0x242 = 11;
        break;
    case 8:
        if (isProfileUnselected() == 1) {
            lb_quest_board_close();
        }
        if (lb_cmd_pressed_ck(16) != 0) {
            lb_quest_board_close();
            sysSE_req(0);
        }
        break;
    case 9:
        if (lb_cmd_pressed_ck(16) != 0) {
            lb_quest_board_close();
        }
        break;
    case 10:
        lb_quest_board_close();
        break;
    case 11:
        if (lb_cmd_pressed_ck(16) != 0) {
            work->step_0x242 = work->return_step_0x246;
        }
        break;
    }
    subTransSet((u32)lb_quest_board_draw_task, 0, NULL);
}

#pragma peephole off
/* 0x803946C0 (0x98): Moves a row cursor with wrap-around, playing the move sound while the pressed row is the
 * selected one. */
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

/* 0x80394758 (0x15C): The list's input: confirm picks the entry under the cursor (1, or 3 when it has no quest),
 * cancel leaves (2), the pad moves the row. */
extern "C" s32 lb_quest_board_list_input(LbQuestBoardWork* work) {
    s32 result = 0;
    LbQuestBoardEntry* entry;

    if (work->anim_0x260 < 8) {
        work->anim_0x260++;
    }
    if (lb_cmd_pressed_ck(16) != 0) {
        work->index_0x249 = work->row_0x247 + work->page_0x24A * 4;
        entry = (LbQuestBoardEntry*)getProfileQuestRecord((u8)work->index_0x249);
        if (entry == NULL || entry->quest_id == 0) {
            sysSE_req(2);
            return 3;
        }
        if (quest_record_find(entry->quest_id) == NULL) {
            NetCtrlWk::setSubError(16);
            sysSE_req(2);
            result = 3;
        } else {
            lobby_w.value_0x082 = work->index_0x249;
            work->quest_id_0x254 = entry->quest_id;
            sysSE_req(27);
            result = 1;
        }
    } else if (lb_cmd_pressed_ck(32) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (lb_cmd_repeat_ck(3) != 0) {
        work->row_0x247 = lb_quest_board_cursor_step(work->row_0x247, work->rows_0x252, lb_cmd_repeat_get(), 1, 2);
        work->anim_0x260 = 0;
    }
    return result;
}

/* 0x803948B4 (0x14): Puts the row cursor back on the first row of a four-row page. */
extern "C" void lb_quest_board_cursor_reset(LbQuestBoardWork* work) {
    work->cursor_0x24E = 0;
    work->count_0x250 = 4;
}

/* 0x803948C8 (0x128): The detail page's input: confirm (once the wait has run out) takes the quest (1), cancel goes
 * back (2), the pad pages the tabs. */
extern "C" s32 lb_quest_board_detail_input(LbQuestBoardWork* work) {
    s32 result = 0;

    if (getProfileQuestRecord(work->index_0x249) == NULL) {
        result = 2;
        NetCtrlWk::setSubError(10);
        sysSE_req(1);
    } else {
        if (work->wait_0x3A4 > 0) {
            work->wait_0x3A4--;
        }
        if (lb_cmd_pressed_ck(16) != 0 && work->wait_0x3A4 == 0) {
            lobby_w.value_0x082 = work->index_0x249;
            sysSE_req(0);
            result = 1;
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step_0x242 = 10;
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(12) != 0) {
            work->cursor_0x24E = menu_cursor_step_forward(work->cursor_0x24E, work->count_0x250, lb_cmd_repeat_get(), 4,
                                                          8, 6, &work->page_moved_0x25C);
        }
    }
    return result;
}

/* 0x803949F0 (0x74): Whether the board refuses the selected entry: true when its quest is unknown or the payload's
 * extra condition fails. */
extern "C" s32 lb_quest_board_accept_input(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    QuestRecord* row = quest_record_find(*getProfileQuestRecord(work->index_0x249));
    u32 current;

    if (row == NULL) {
        return 1;
    }
    current = quest_ex_condition_ck(row, work->value_0x38C, work->data_0x264->field_0x014, work->data_0x264);
    return current != 1;
}

/* 0x80394A64 (0x170): The session-name keyboard: opens it, then waits for it; 1 with a name, 2 cancelled, 3 when the
 * entry went away. */
extern "C" s32 lb_quest_board_name_input(LbQuestBoardWork* work) {
    LbQuestBoardEntry* entry = (LbQuestBoardEntry*)getProfileQuestRecord(work->index_0x249);
    s32 result = 0;
    NetProfileRec* profile = &refreshAllProfiles()[work->index_0x249];

    if (entry == NULL) {
        NetCtrlWk::setSubError(10);
        result = 3;
    } else if (entry->players <= profile->active_0x040) {
        hud_msg_push(0, (const char*)LbStr(0, 0x1FD));
        result = 2;
    }
    switch (work->kbd_step_0x245) {
    case 0:
        if (kbd_open(4) == 1) {
            menu_work.slot[0].field_0x32D = 1;
            set_kbd_param(work->kbd_buf_0x398, 8);
            work->kbd_step_0x245++;
        }
        break;
    case 1:
        switch (kbd_move()) {
        case 1:
            menu_work.slot[0].field_0x32D = 0;
            if (work->kbd_buf_0x398[0] == 0) {
                result = 2;
            } else {
                memcpy(work->name_0x26B, work->kbd_buf_0x398, 5);
                result = 1;
            }
            break;
        case -1:
            menu_work.slot[0].field_0x32D = 0;
            result = 2;
            break;
        }
        break;
    }
    return result;
}

/* 0x80394BD4 (0x1D0): Enters the selected profile: requests it, waits for the answer, downloads the quest when it is
 * not on the disc; 1 once entered, 3 on failure (the screen closes). */
extern "C" s32 lb_quest_board_enter_step(LbQuestBoardWork* work) {
    s32 result = 0;
    s32 quest_id;
    s32 status;

    switch (work->enter_step_0x243) {
    case 0:
        if (work->renamed_0x3A2 == 1) {
            setSessionDisplayName(work->name_0x26B);
        }
        if (requestProfileEnter(work->index_0x249) == 1) {
            work->enter_step_0x243 = 1;
            work->enter_error_0x244 = 0;
            work->blink_0x240 = 0;
        } else {
            result = 3;
            lb_quest_board_close();
        }
        break;
    case 1:
        status = getProfileEnterResult();
        if (status == 1) {
            quest_id = work->quest_id_0x254;
            if (quest_id != getSelectedQuestId()) {
                runPendingAction6();
                work->enter_step_0x243 = 3;
                work->enter_error_0x244 = 1;
            } else if (quest_id < 60000) {
                result = 1;
            } else {
                work->enter_step_0x243 = 2;
            }
        } else if (status < 0) {
            result = 3;
            lb_quest_board_close();
        }
        break;
    case 2:
        status = stepStagingDownloadForVersion(work->quest_id_0x254);
        if (status != 0) {
            if (isProfileUnselected() == 1) {
                NetCtrlWk::setSubError(20);
                result = 3;
                lb_quest_board_close();
            } else if (status < 0) {
                runPendingAction6();
                work->enter_step_0x243++;
                work->enter_error_0x244 = 2;
            } else {
                result = 1;
            }
        }
        break;
    case 3:
        if (hasSelectedProfile() != 1) {
            if (work->enter_error_0x244 == 1) {
                NetCtrlWk::setSubError(20);
            }
            if (work->enter_error_0x244 == 2) {
                NetCtrlWk::setSubError(16);
            }
            result = 3;
            lb_quest_board_close();
        }
        break;
    }
    return result;
}

/* 0x80394DA4 (0x33C): Draws the board list: four rows of the current page, each with its number and, for a live
 * entry, the quest name, the poster, the players wanted and the sub-label count. */
extern "C" void draw_quest_board(LbQuestBoardWork* work) {
    _mh_ivec2_ anchor;
    _mh_ivec2_ row_pos;
    _SPR_DATA_ spr;
    char text[0x10];
    s16 row;
    u16* lsp;
    s32 index;
    LbQuestBoardEntry* entry;
    NetProfileRec* profile;
    u8 frame;
    s32 selected;
    char* name;
    s8 count;

    get_lsp_data(0x163D, &anchor);
    draw_sprite_ary(lb_quest_board_frame_ids, &anchor);
    get_lsp_data(0x1648, &anchor);
    uv_pair_copy(&row_pos, &anchor);
    for (row = 0, lsp = lb_quest_board_row_lsp; row < 4; row++, lsp++) {
        index = row + work->page_0x24A * 4;
        entry = (LbQuestBoardEntry*)getProfileQuestRecord(index);
        profile = &refreshAllProfiles()[index];
        if (row == work->row_0x247) {
            frame = work->anim_0x260;
            selected = 1;
        } else {
            frame = 0;
            selected = 0;
        }
        sprite_frame_apply(&spr, 0x1648, frame, &anchor);
        if (row != 0) {
            get_lsp_data(*lsp, &row_pos);
        } else {
            uv_pair_copy(&row_pos, &anchor);
        }
        anchor.y = row_pos.y;
        draw_sprite_ary(lb_quest_board_row_ids, &anchor);
        sprintf(text, "%d", row + work->page_0x24A * 4 + 1);
        draw_font_idx(0x1650, (s8*)text, 1, &anchor);
        if (entry != NULL) {
            if (entry->quest_id != 0) {
                draw_sprite_ary(lb_quest_board_entry_ids, &anchor);
                draw_font_idx(0x164D, (s8*)quest_result_field_text_get(quest_record_find(entry->quest_id), 0), 5,
                              &anchor);
                name = NetCtrlWk::findPeerName(&profile->address_0x01C);
                if (name == NULL) {
                    draw_font_idx(0x164E, (s8*)lb_quest_board_blank_name, 1, &anchor);
                } else {
                    draw_font_idx(0x164E, (s8*)name, 1, &anchor);
                }
                sprintf(text, "%d", entry->players);
                draw_font_idx(0x1652, (s8*)text, 0, &anchor);
                count = 0;
                if (entry->labels.sublabel_0x40[0][0] != 0) {
                    count = 1;
                }
                if (entry->labels.sublabel_0x40[1][0] != 0) {
                    count++;
                }
                if (entry->labels.sublabel_0x40[2][0] != 0) {
                    count++;
                }
                if (entry->labels.sublabel_0x40[3][0] != 0) {
                    count++;
                }
                sprintf(text, "%d", count);
                draw_font_idx(0x1651, (s8*)text, 0, &anchor);
                if (profile->rank_0x08C == 1) {
                    draw_sprite_idx(0x1653, &anchor);
                }
            } else {
                draw_sprite_ary(lb_quest_board_empty_ids, &anchor);
                draw_font_idx(0x164F, (s8*)LbStr(0, 0x1A2), 4, &anchor);
            }
        } else {
            draw_sprite_ary(lb_quest_board_empty_ids, &anchor);
            draw_font_idx(0x164F, (s8*)LbStr(0, 0x1A2), 4, &anchor);
        }
        if (selected != 0) {
            if (chk_pointer() == 0) {
                draw_sprite_idx(0x164A, &anchor);
                draw_sprite_idx(0x1649, &anchor);
            }
        } else {
            draw_sprite_ary(lb_quest_board_shade_ids, &anchor);
        }
    }
}

/* 0x803950E0 (0x280): Draws the detail page's summary tab: the sub-label count, the players wanted, the session icon,
 * the wrapped comment and the four labels. */
extern "C" void lb_quest_board_summary_draw(LbQuestBoardWork* work) {
    LbQuestBoardEntry* entry = (LbQuestBoardEntry*)getProfileQuestRecord(work->index_0x249);
    NetProfileRec* profile = &refreshAllProfiles()[work->index_0x249];
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    char text[0x10];
    char comment[0xA0];
    s8 count;
    NetProfileLabels* labels;
    s32 i;
    char* label;
    char* sublabel;
    u16* lsp;

    get_lsp_data(0x1674, &anchor);
    draw_sprite_ary(lb_quest_board_summary_ids, &anchor);
    count = 0;
    if (entry->labels.sublabel_0x40[0][0] != 0) {
        count = 1;
    }
    if (entry->labels.sublabel_0x40[1][0] != 0) {
        count++;
    }
    if (entry->labels.sublabel_0x40[2][0] != 0) {
        count++;
    }
    if (entry->labels.sublabel_0x40[3][0] != 0) {
        count++;
    }
    sprintf(text, "%d", count);
    draw_font_idx(0x167A, (s8*)text, 0, &anchor);
    draw_font_idx(0x167B, (s8*)LbStr(1, 7), 1, &anchor);
    sprintf(text, "%d", entry->players);
    draw_font_idx(0x167C, (s8*)text, 0, &anchor);
    draw_font_idx(0x167D, (s8*)LbStr(1, 6), 0, &anchor);
    draw_sprite_idx((profile->rank_0x08C == 1) + 0x1678, &anchor);
    postQuestBoardRecord((u8*)entry->comment);
    quest_comment_wrap(entry->comment, comment);
    draw_font_idx(0x167E, (s8*)comment, 0, &anchor);
    labels = &entry->labels;
    get_lsp_data(0x168F, &anchor);
    uv_pair_copy(&pos, &anchor);
    for (i = 0, label = labels->label_0x00[0], sublabel = labels->sublabel_0x40[0], lsp = lb_quest_board_label_lsp; i < 4;
         i++, label += 0x10, sublabel += 0x10, lsp++) {
        if (label[0x40] != 0) {
            draw_sprite_ary(lb_quest_board_label_ids, &pos);
            if (get_option_cfg(12) == 0) {
                draw_font_idx(0x1690, (s8*)sublabel, 0, &pos);
            } else {
                draw_font_idx(0x1690, (s8*)label, 0, &pos);
            }
            draw_sprite_uv_color_idx(0x1691, labels->count_0x80[i], -1, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += anchor.x;
        pos.y += anchor.y;
    }
}

/* 0x80395360 (0xE0): Draws the detail page: the header, the tab arrows and the tab the cursor is on, then the
 * footer. */
extern "C" void lb_quest_board_detail_draw(LbQuestBoardWork* work) {
    u16* id = getProfileQuestRecord(work->index_0x249);
    QuestRecord* rec;

    if (id != NULL) {
        rec = quest_record_find(*id);
        if (rec != NULL) {
            quest_detail_header_draw(rec);
            lb_page_arrow_draw(0x156F, work->cursor_0x24E, work->count_0x250, work->page_moved_0x25C,
                               work->step_0x242 == 2);
            switch (work->cursor_0x24E) {
            case 0:
                lb_quest_board_summary_draw(work);
                break;
            case 1:
                quest_detail_target_draw(rec);
                break;
            case 2:
                quest_detail_reward_draw(rec);
                break;
            case 3:
                quest_detail_label_draw((MenuQuestWork*)rec);
                break;
            }
            quest_detail_footer_draw(rec);
        }
    }
}

/* 0x80395440 (0xBC): Draws the message panel line the step calls for (and the yes/no row in step 3). */
extern "C" void lb_quest_board_panel_draw(LbQuestBoardWork* work) {
    u16 msg = 0xFFFF;

    switch (work->step_0x242) {
    case 1:
        msg = 0x17E;
        break;
    case 2:
        msg = 0x17F;
        break;
    case 3:
        msg = 0x180;
        lb_panel_yes_no_draw(0x1879, work->field_0x258);
        break;
    case 9:
        msg = work->message_0x25E;
        break;
    case 8:
        msg = 0x181;
        break;
    case 5:
        msg = 0x182;
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
    }
}

/* 0x803954FC (0x1A4): The board's draw callback: the list or the detail page for the step, the message panel, and in
 * step 7 the blinking "connecting" dialog. */
extern "C" void lb_quest_board_draw_task(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    _MH_VEC2 size;
    s16 width = 590;
    s16 x;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if ((u32)(work->step_0x242 - 2) <= 2 || (u32)(work->step_0x242 - 6) <= 2) {
        lb_quest_board_detail_draw(work);
    } else {
        switch (work->step_0x242) {
        case 1:
            draw_quest_board(work);
            break;
        case 14:
            lb_quest_board_detail_draw(work);
            break;
        case 5:
            lb_quest_board_detail_draw(work);
            break;
        }
    }
    lb_quest_board_panel_draw(work);
    font_flush();
    if (work->step_0x242 == 7) {
        get_ScreenSize(&size);
        x = (size.x - width) * 0.5f;
        set_zmode(false, 0, false);
        set_blendmode(4, 5, 1);
        put_frame_dialog(x, 40, 590, 400, 0, 0x14120AF0);
        if (system_w.field_0x05 == 0) {
            work->blink_0x240++;
        }
        if (!(work->blink_0x240 & 0x10)) {
            MH3DispErrorString(x + 24, 68, (s8*)MH3GetErrorString2(22));
        }
    }
    font_flush();
}

/* 0x803956A0 (0x80): Opens the notice screen (screen 20) - the same setup as the board, minus the payload lookup,
 * with the menu's cancel sound. */
extern "C" void lb_quest_board_open_notice(LbQuestBoardData* data) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    memset(work, 0, 0x2000);
    work->data_0x264 = data;
    lobby_w.active_0x008 = 1;
    lobby_w.state_0x000 = 20;
    lobby_w.value_0x082 = 0;
    sysSE_req(0);
}

/* 0x80395720 (0x68): Sets the 2D state the board draws in and pushes its two panel labels. */
extern "C" void lb_quest_board_begin_panel(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_panel_msg_draw(0x1877, 0x185);
    lb_panel_yes_no_draw(0x1879, work->field_0x258);
}

/* 0x80395788 (0x64): Undoes `lb_quest_board_begin_panel`: clears the panel-pending bit on the payload and on the
 * lobby block, then lets the payload hand its two bytes on. */
extern "C" void lb_quest_board_end_panel(LbQuestBoardWork* work) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
    work->data_0x264->flags_0xB01 &= ~4;
    quest_board_flags_send(work->data_0x264->flags_0xB01, work->data_0x264->param_0xB02);
    lobby_w.flags_0x052 &= ~4;
}

/* 0x803957EC (0x124): The ready-check screen's step: waits for the profile, asks yes/no, then drops the ready flag
 * (yes) or goes back to the party screen (no); leaves when this player is no longer a member. */
extern "C" void lb_quest_board_ready_step(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    switch (work->step_0x242) {
    case 0:
        work->step_0x242++;
        work->field_0x258 = 1;
        break;
    case 1:
        if (getProfileFlag() == 1) {
            work->step_0x242++;
            return;
        }
        if (getOwnMemberFlag() == 0) {
            lb_quest_board_end_panel(work);
            return;
        }
        switch (lb_yes_no_step(&work->field_0x258)) {
        case 1:
            requestReadyOffAlias();
            work->step_0x242 = 3;
            break;
        case 2:
            lobby_w.active_0x008 = 1;
            lobby_w.state_0x000 = 10;
            return;
        }
        break;
    case 2:
        return;
    case 3:
        if (getOwnMemberFlag() == 0) {
            lb_quest_board_end_panel(work);
            return;
        }
        break;
    }
    subTransSet((u32)lb_quest_board_begin_panel, 0, NULL);
}

/* 0x80395910 (0xA0): Opens the party screen (screen 10) for the player work `plw` and starts its waiting act. */
extern "C" void lb_quest_board_open_ready(LbQuestBoardData* plw) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    memset(work, 0, 0x2000);
    lobby_w.active_0x008 = 1;
    lobby_w.state_0x000 = 10;
    work->data_0x264 = plw;
    work->field_0x390 = 0;
    work->step_0x242 = 0;
    work->enter_step_0x243 = 0;
    work->enter_error_0x244 = 0;
    lb_npc_act_set((struct _PLW*)plw, 0, 0, (u16)0x8000);
}

/* 0x803959B0 (0x18): Leaves the notice screen.  GUESS: see `lb_quest_board_close`, whose body this repeats byte for
 * byte; the map carries both addresses. */
extern "C" void lb_quest_board_cancel(void) {
    lobby_w.active_0x008 = 0;
    lobby_w.state_0x000 = 0;
}

/* 0x803959C8 (0x84): The party screen's panel: the ready line (the last member's own text) and the yes/no row. */
extern "C" void lb_quest_board_ready_panel_draw(void) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if (work->not_last_0x3A3 == 0) {
        lb_panel_msg_draw(0x1877, 0x186);
    } else {
        lb_panel_msg_draw(0x1877, 0x187);
    }
    lb_panel_yes_no_draw(0x1879, work->field_0x258);
}

/* 0x80395C84 (0x80): Resets the quest board: closes the lobby panel, resets the party state and clears the
 * payload's three party bits. */
extern "C" void lb_quest_board_reset(s32 unused, s32 mode) {
    LbQuestBoardWork* work = lobby_w.menu_0xAC;
    _PLW* plw = (_PLW*)get_move_work_adrs(2);

    lb_panel_close();
    quest_party_state_reset(plw);
    lobby_w.flags_0x052 = 0;
    work->data_0x264->flags_0xB01 &= ~7;
    quest_board_flags_send(work->data_0x264->flags_0xB01, work->data_0x264->param_0xB02);
}

#pragma peephole on
/* 0x80395D04 (0xF0): Spawns a kind-`kind` effect riding on the NPC `npc`'s joint `joint`, at `offset` from it, with
 * the parameter scale `scale` - when the NPC is in the current area. */
extern "C" void lb_quest_board_effect_spawn(_LB_NPC* npc, s8 kind, u32 joint, VEC3* offset, f32 scale) {
    _EFT* self;
    LbQuestBoardEft* work;

    if (npc->field_0x004 == get_now_areano()) {
        self = (_EFT*)eft_res_slot_get(0x20);
        if (self != NULL) {
            work = (LbQuestBoardEft*)self->work_0x38;
            work->count = 1;
            work->scale = scale;
            work->joint = joint;
            copyVec3(&work->offset, offset);
            self->field_0x03 = 0x37;
            self->type_0x02 = kind;
            self->source_0x30 = npc;
            self->area_0x44 = npc->field_0x004;
            self->rot_0x24.x = npc->field_0x028;
            self->rot_0x24.y = npc->field_0x02C;
            self->rot_0x24.z = npc->field_0x030;
            self->timer_0x0C = 0;
            eft_state_flags_set(self, 0, 0);
            self->release_0x40 = lb_quest_board_effect_release;
            self->dispatch_0x34 = lb_quest_board_step;
        }
    }
}

#pragma peephole off
/* 0x80395DF4 (0x124): Spawns a kind-`kind` model effect on the NPC `npc` (in the current area only): one model out of
 * the effect-model pool; null when either pool is empty. */
extern "C" _EFT* lb_quest_board_model_spawn(_LB_NPC* npc, s8 kind) {
    _EFT* self;
    LbQuestBoardEft* work;
    s32 i;
    MHchar** model;

    if (npc->field_0x004 != get_now_areano()) {
        return NULL;
    }
    self = (_EFT*)eft_res_slot_get(0x10);
    if (self == NULL) {
        return NULL;
    }
    self->type_0x02 = kind;
    self->release_0x40 = lb_quest_board_effect_release;
    self->dispatch_0x34 = lb_quest_board_step;
    work = (LbQuestBoardEft*)self->work_0x38;
    work->count = 1;
    for (i = 0, model = &work->model; i < work->count; i++, model++) {
        *model = (MHchar*)eft_res_model_get();
        if (*model == NULL) {
            eft_res_slot_release(self);
            return NULL;
        }
    }
    self->field_0x03 = 0x37;
    self->source_0x30 = npc;
    self->area_0x44 = npc->field_0x004;
    self->rot_0x24.x = npc->field_0x028;
    self->rot_0x24.y = npc->field_0x02C;
    self->rot_0x24.z = npc->field_0x030;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    eft_state_flags_set(self, 0, 0);
    return self;
}

/* 0x80395F18 (0x60): Spawns the kind-8 flash model for `frames` frames, flashing between `from` and `to`. */
extern "C" void lb_quest_board_flash_spawn(_LB_NPC* npc, s32 frames, s32 from, s32 to) {
    _EFT* self = lb_quest_board_model_spawn(npc, 8);
    LbQuestBoardEft* work;

    if (self != NULL) {
        work = (LbQuestBoardEft*)self->work_0x38;
        work->flash_from = from;
        work->flash_to = to;
        self->timer_0x0C = frames;
    }
}

/* 0x80395F78 (0x8): Spawns the kind-9 model that follows the NPC. */
extern "C" void lb_quest_board_follow_spawn(_LB_NPC* npc) {
    lb_quest_board_model_spawn(npc, 9);
}

/* 0x80395F80 (0x18): Releases whatever the effect built: the 8/9 kinds own a model list, every other kind a single
 * record. */
extern "C" void lb_quest_board_effect_release(_EFT* self) {
    if ((u32)(self->type_0x02 - 8) <= 1) {
        lb_quest_board_effect_retire_list(self);
    } else {
        lb_quest_board_effect_push_models(self);
    }
}

/* 0x80395F98 (0x3C): Returns the effect's records to the effect heap and empties the list. */
extern "C" void lb_quest_board_effect_push_models(_EFT* self) {
    LbQuestBoardEft* work = (LbQuestBoardEft*)self->work_0x38;

    push_eft_effect_heap_num(&work->effect, work->count);
    work->count = 0;
}

/* 0x80395FD4 (0x3C): Retires the effect's whole model list and empties it. */
extern "C" void lb_quest_board_effect_retire_list(_EFT* self) {
    LbQuestBoardEft* work = (LbQuestBoardEft*)self->work_0x38;

    fn_800F8A44(&work->model, work->count);
    work->count = 0;
}

/* 0x80396010 (0x3C): Runs the effect's current state, dispatching to the updater that owns it; the first state
 * further splits on the effect kind. */
extern "C" void lb_quest_board_step(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        lb_quest_board_step_kind(self);
        return;
    case 1:
        lb_quest_board_effect_update(self);
        return;
    case 2:
        q_result_anim_counter_inc(self);
        return;
    case 3:
        q_result_release_effect(self);
        return;
    }
}

/* 0x8039604C (0x24): Runs the effect kind's own first state.  GUESS name; the map row is split out of
 * `lb_quest_board_step`'s 0x60 bytes. */
extern "C" void lb_quest_board_step_kind(_EFT* self) {
    switch (self->type_0x02) {
    case 8:
        lb_quest_board_flash_init(self);
        return;
    case 9:
        lb_quest_board_follow_init(self);
        return;
    default:
        lb_quest_board_effect_init(self);
        return;
    }
}

/* 0x80396248 (0xD4): Creates the kind-8 flash model (model 0xA8, motion 0x4C), shows only its first part and scales
 * it, then starts updating it. */
extern "C" void lb_quest_board_flash_init(_EFT* self) {
    VEC3 pos;
    LbQuestBoardEft* work;

    VEC3_ctor(&pos);
    work = (LbQuestBoardEft*)self->work_0x38;
    self->state_0x05++;
    if (eft_res_spawn_gate_ck(self, 3) == 0) {
        q_result_release_effect(self);
        return;
    }
    if (res_eft_model_create(work->model, 0xA8, 0x4C) == NULL) {
        q_result_release_effect(self);
        return;
    }
    work->model->setVisibility(1, true);
    work->model->setVisibility(2, false);
    setVector3(&work->model->scale_0x1C, 0.7f, 0.7f, 0.7f);
    self->flag_0x01 = 1;
    lb_quest_board_effect_update(self);
}

/* 0x8039631C (0xB4): Creates the kind-9 follower model (model 0xAC) and scales it, then starts updating it. */
extern "C" void lb_quest_board_follow_init(_EFT* self) {
    VEC3 pos;
    LbQuestBoardEft* work;

    VEC3_ctor(&pos);
    work = (LbQuestBoardEft*)self->work_0x38;
    self->state_0x05++;
    if (eft_res_spawn_gate_ck(self, 3) == 0) {
        q_result_release_effect(self);
        return;
    }
    if (res_eft_model_create(work->model, 0xAC, 0) == NULL) {
        q_result_release_effect(self);
        return;
    }
    setVector3(&work->model->scale_0x1C, 1.3f, 1.3f, 1.3f);
    self->flag_0x01 = 1;
    lb_quest_board_effect_update(self);
}

/* 0x803963D0 (0x24): Updates the effect, by kind. */
extern "C" void lb_quest_board_effect_update(_EFT* self) {
    switch (self->type_0x02) {
    case 8:
        lb_quest_board_flash_move(self);
        return;
    case 9:
        q_result_effect_follow_npc(self);
        return;
    default:
        lb_quest_board_effect_move(self);
        return;
    }
}

/* 0x80396654 (0x19C): Moves the kind-8 flash model with the NPC's joint 10 for its frame count, tinting it half
 * green inside its flash window. */
extern "C" void lb_quest_board_flash_move(_EFT* self) {
    LbQuestBoardEft* work = (LbQuestBoardEft*)self->work_0x38;
    _LB_NPC* npc = (_LB_NPC*)self->source_0x30;
    _GXColor color;
    s32 i;

    if (eft_res_spawn_gate_ck(self, 3) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (++self->field_0x10 > self->timer_0x0C) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->field_0x10 >= work->flash_from && self->field_0x10 < work->flash_to) {
        color.r = 0xFF;
        color.g = 0x7F;
        color.b = 0xFF;
        color.a = 0xFF;
        for (i = 0; i < 2; i++) {
            work->model->setMatColor(i, GX_COLOR0A0, color, false);
        }
    } else {
        color.r = 0xFF;
        color.g = 0xFF;
        color.b = 0xFF;
        color.a = 0xFF;
        for (i = 0; i < 2; i++) {
            work->model->setMatColor(i, GX_COLOR0A0, color, false);
        }
    }
    ((MHchar*)npc->body_0x058)->get_joint_wpos(10, &self->pos_0x18);
    self->pos_0x18.y += 30.0f;
    copyVec3(&work->model->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy(&work->model->rot_0x54, &self->rot_0x24);
    work->model->move(0);
    eft_res_models_spawn(self, (void**)&work->model, 2, work->count, NULL);
}
