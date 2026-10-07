/* lobby/lb_quest_ui.cpp - the lobby NPC talk program, the kitchen screen, the trade screen and the scene effect.
 * RANGE. .text 0x8038EC44-0x80394158 (66 functions); .data 0x805F0CB8-0x805F1400 (ends with the camera key table),
 *   .sdata 0x807933D8-0x80793470, .sdata2 0x8079C298-0x8079C2D0, extab, extabindex.  Four groups in address order: the
 *   note-pane NPC talk program (0x8038EC44-0x8038F2BC, on `enemy/note_work.h`'s `NoteWork`), the kitchen screen (lobby
 *   screen 0x11, to 0x803928F0), the trade screen (screen 0x12, to 0x80393994) and the scene effect (effect 0x36),
 *   whose tail is `lb_scene_model_slide` and the two helpers after it (0x80394038-0x80394158).
 * FLAGS. `cflags_lobby`; `#pragma peephole off` file-wide (retail keeps `clrlwi`/`rlwinm` + `cmpwi` and `clrlwi` +
 *   `slwi` unfused; every written row measured better with it off).
 * NAMES. `lb_quest_ui` is the registered GUESS; the groups' names (`note_talk_*`, `lb_kitchen_*`, `lb_trade_*`,
 *   `lb_scene_eft_*`) and every data name are GUESSes from the bodies: the kitchen pays zenny or resource points for a
 *   pair of ingredients (`lb_kitchen_pair_find` keys the pair tables on the two groups) and fills `lb_param_w`'s meal
 *   skills; the trade screen exchanges `NetCtrlWk::getServerNotice`'s offers.  `lb_kitchen_idle_ck` answers whether
 *   the kitchen is outside its menu states 2..4.
 *   GUESS: `note_turn_step`, `note_talk_frame`, `note_talk_mode_step`, `note_talk_noop`,
 *   GUESS: `note_talk_state_step`, `lb_kitchen_tri_sum`, `lb_kitchen_pair_index`, `lb_kitchen_pair_seen_set`,
 *   GUESS: `lb_kitchen_pair_seen_ck`, `lb_kitchen_close`, `lb_kitchen_page_set`, `lb_kitchen_row_copy`,
 *   GUESS: `lb_kitchen_course_ck`, `lb_kitchen_pick_reset`, `lb_kitchen_pick_input`,
 *   GUESS: `lb_kitchen_list_page_set`, `lb_kitchen_special_open`, `lb_kitchen_pair_find`,
 *   GUESS: `lb_kitchen_skill_apply`, `lb_kitchen_special_apply`, `lb_kitchen_meal_serve`,
 *   GUESS: `lb_kitchen_frame_draw`, `lb_kitchen_list_draw`, `lb_kitchen_slot_pulse_draw`,
 *   GUESS: `lb_kitchen_skill_text`, `lb_kitchen_pick_draw`, `lb_kitchen_special_pick_draw`,
 *   GUESS: `lb_kitchen_confirm_draw`, `lb_kitchen_pick_screen_draw`, `lb_kitchen_special_list_draw`,
 *   GUESS: `lb_kitchen_msg_draw`, `lb_kitchen_draw_task`, `lb_kitchen_idle_ck`, `lb_trade_open`,
 *   GUESS: `lb_trade_close`, `lb_trade_page_set`, `lb_trade_exchange`, `lb_trade_step`, `lb_trade_frame_draw`,
 *   GUESS: `lb_trade_list_draw`, `lb_trade_cost_draw`, `lb_trade_msg_draw`, `lb_trade_draw_task`,
 *   GUESS: `lb_scene_eft_spawn`, `lb_scene_eft_release`, `lb_scene_eft_step`, `lb_scene_model_slide`,
 *   GUESS: `lb_quest_board_state_next`, `lb_quest_board_effect_retire`
 * RESIDUALS. 17 rows unwritten, each blocked on a declaration or a name other lanes own (requests filed):
 *   0x8038EC44-0x8038EF28 and 0x8038EF28-0x8038EF9C (`note_talk_step`, `note_idle_set`, `note_talk_init`) and
 *   0x8038EFEC-0x8038F264 (`note_talk_wait_step`/`_greet_step`/`_react_step`): the talk program needs `NoteWork`, whose
 *   `sound/mhchar.h` `MHchar` cannot sit beside `hud/layout.h`'s `pl.h` one, and `menu/multi_result.cpp`'s 0x803A357C;
 *   0x8038F4D8-0x8038F748 (`lb_kitchen_open`), 0x8038F9A4-0x8038FAC4 (`lb_kitchen_courses_roll`),
 *   0x8039065C-0x803907F4 (`lb_kitchen_bonus_roll`, `lb_kitchen_extra_roll`): `ef/system_core.cpp`'s 0x800CEF18;
 *   0x80390F20-0x803914D4 (`lb_kitchen_step`), 0x80391EF4-0x803921A4 (`lb_kitchen_course_draw`): its 0x800D2EA4;
 *   0x80390154-0x803905FC (`lb_kitchen_special_input`), 0x80392504-0x803925A0 (`lb_kitchen_special_screen_draw`):
 *   `ef/eft052.cpp`'s 0x80359628/0x80359B00; 0x80392964-0x80392A4C (`lb_trade_offer_state`): `eft052_page_count_ck`
 *   is not in `ef/eft052.h`; 0x80393B28-0x80393D4C (`lb_scene_eft_init`): `pl.h`'s `MHchar` has no `frame_init`;
 *   0x80393D4C-0x80394038 (`lb_scene_eft_move`): `ef/eft026_fx.cpp`'s 0x80119BB0.
 *  - `lb_kitchen_tri_sum`: retail's 8x-unrolled countdown keeps no separate counter; ours decrements one;
 *  - `lb_kitchen_list_draw`, `lb_kitchen_special_list_draw`: `active` takes r31 where retail has r25/r27;
 *  - `lb_kitchen_list_page_set`: retail steps a dead per-entry counter (`addi r7`) inside the 3x-unrolled copy;
 *  - `lb_trade_list_draw`: retail scales the offer index by 4 twice (a 4-byte-element table), ours by 16 once;
 *  - `lb_kitchen_meal_serve`: the two rare bytes load in the other order; `lb_trade_page_set`: register order.
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted (the tables are declared, not defined; the unwritten
 *   rows' jump tables would be missing anyway); `.text`/extab/extabindex short of the claim.
 */

#pragma peephole off

#include "types.h"
#include "lobby/lb_quest_ui.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "MSL_C/alloc.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft052.h"
#include "sound/fn_800DD1F0.h"
#include "sound/fn_800D7F54.h"
#include "sound/set_zmode__FbUcb.h"
#include "sound/se_req.h"
#include "enemy/em020_prog.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/LbStr.h"
#include "lobby/lb_panel_msg_draw.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/fn_801E7530.h"
#include "lobby/lb_unlock_cond_ck.h"
#include "fn_80047398/lobby_world_block.h"
#include "fn_80047398.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/vec3.h"
#include "quest/quest_types.h"
#include "hud/layout.h"
#include "hud/cockpit.h"
#include "font/flfnt.h"
#include "menu/menu_message.h"
#include "menu/ItemExp.h"
#include "menu/menu_row.h"
#include "camera/camera.h"
#include "camera/camera_frame_get.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_item_add.h"
#include "Pl/fn_80262940.h"
#include "Pl/pl_coll.h"
#include "Pl/pl_act_step_data.h"
#include "fn_8004CAD8.h"
#include "get_FqResult_work.h"
#include "userdata_item.h"
#include "main.h"
#include "stage/stg_w.h"
#include "Network/network_pat_control.h"

/* The note-pane NPC record (`enemy/note_work.h`'s 0x1F8-byte `NoteWork`) as the talk program reads it: this TU draws
 * through `hud/layout.h`, whose `pl.h` `MHchar` cannot sit beside the `sound/mhchar.h` one `enemy/note_work.h` embeds,
 * so the program keeps its own view of the bytes it touches. size: 0x1F8 (a view) */
typedef struct NoteTalkView {
    /* +0x000 */ u8 pad_0x000[0x18C];
    /* +0x18C */ u32 angle_0x18C;     /* the facing angle `note_turn_step` eases */
    /* +0x190 */ u8 pad_0x190[0xD];
    /* +0x19D */ u8 state_0x19D;      /* the arm `note_talk_state_step` runs */
    /* +0x19E */ u8 pad_0x19E;
    /* +0x19F */ u8 mode_0x19F;       /* the mode `note_talk_mode_step` runs */
    /* +0x1A0 */ u8 pad_0x1A0;
    /* +0x1A1 */ u8 again_0x1A1;      /* 1 runs the state arm a second time this frame */
    /* +0x1A2 */ u8 pad_0x1A2[0x6];
    /* +0x1A8 */ u32 target_0x1A8;    /* the angle the NPC turns to */
    /* +0x1AC */ u8 pad_0x1AC[0x4C];
} NoteTalkView;

/* The kitchen's tables (this unit's `.data`/`.sdata`, declared until the unit emits them). */
extern s32 lb_kitchen_cost_tbl[6];            /* per tier: the zenny cost, then the resource-point cost */
extern u8 lb_kitchen_special_tbl[12];         /* the special courses of the two list pages, 0-ended */
extern s16 lb_kitchen_special_values[12];     /* the value each special course gives */
extern s16* lb_kitchen_bonus_tbls[3];         /* the (skill, value) pairs of the skill rows 0x8000..0x8002 */
extern u16* lb_kitchen_pair_tbls[3];          /* per tier: the pair table, 6-byte records ended by 0xFFFF */
extern u16 lb_kitchen_special_holds[4];       /* the two special courses and their hold slots */
extern LbChoiceDef lb_kitchen_choice_defs[3];
extern u16 lb_kitchen_frame_ids[6];
extern u16 lb_kitchen_frame_wide_ids[6];
extern u16 lb_kitchen_title_ids[14];
extern u16 lb_kitchen_list_ids[10];
extern u16 lb_kitchen_row_lsp[6];
extern u16 lb_kitchen_rare_lsp[6];
extern u16 lb_kitchen_pick_ids[18];
extern u16 lb_kitchen_slot_ids[8];
extern u16 lb_kitchen_slot_pulse_ids[8];
extern char lb_kitchen_skill2_fmt[10];
extern u16 lb_kitchen_course_ids[20];
extern u16 lb_kitchen_course_row_ids[6];
extern u16 lb_kitchen_course_cursor_ids[6];
extern u16 lb_kitchen_bonus_ids[16];
extern u16 lb_kitchen_confirm_ids[8];
extern u16 lb_kitchen_confirm_row_ids[6];
extern u16 lb_kitchen_confirm_cursor_ids[6];
extern u16 lb_kitchen_choice_sprites[12];
extern u16 lb_kitchen_slot_frame_ids[4];
extern u16 lb_kitchen_slot_lsp[2];
extern u16 lb_kitchen_slot_label_ids[2];
extern char lb_kitchen_skill_none_fmt[4];
extern char lb_kitchen_skill_value_fmt[8];
extern char lb_kitchen_text_fmt[4];
extern char lb_kitchen_cost_fmt[8];
extern char lb_kitchen_skill1_fmt[8];
extern u16 lb_kitchen_special_label_ids[3];
extern u16 lb_kitchen_course_lsp[4];
extern u16 lb_kitchen_bonus_frame_ids[4];
extern u16 lb_kitchen_bonus_lsp[4];
extern u16 lb_kitchen_confirm_lsp[4];
extern u16 lb_kitchen_choice_ids[2];

/* The trade screen's tables. */
extern LbChoiceDef lb_trade_choice_def;
extern u16 lb_trade_frame_ids[6];
extern u16 lb_trade_frame_wide_ids[6];
extern u16 lb_trade_title_ids[14];
extern u16 lb_trade_list_ids[10];
extern u16 lb_trade_row_lsp[6];
extern u16 lb_trade_detail_ids[20];
extern u16 lb_trade_pouch_ids[6];
extern u16 lb_trade_box_ids[6];
extern u16 lb_trade_detail_lsp[8];
extern u16 lb_trade_cost_ids[24];
extern u16 lb_trade_cost_cursor_ids[16];
extern u16 lb_trade_cost_row_ids[16];
extern u16 lb_trade_label_ids[4];
extern u16 lb_trade_choice_ids[2];
extern u16 lb_trade_cost_lsp[3];
extern char lb_trade_count_fmt[4];

/* The camera model's key-frame table `lb_scene_model_slide` reads: six (frame, value) pairs. */
f32 lb_quest_detail_camera_keys[12] = {
    0.0f, -3160.0f, 82.0f, -3080.9f, 144.0f, -3035.2f, 212.0f, -2965.3f, 310.0f, -2960.0f, -1.0f, -2960.0f,
};

/* The model record `lb_scene_eft_move` hands `lb_scene_model_slide`: its step byte and the position the
 * camera keys drive.  size: 0x10 (a view: the model continues) */
typedef struct LbQuestDetailModel {
    /* +0x00 */ u8 step;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ VEC3 pos;
} LbQuestDetailModel; /* size: 0x10 */

extern "C" {

/* The unit's own entry points the bodies below reach before their definitions (or that are unwritten). */
void note_talk_wait_step(NoteTalkView* self);
void note_talk_greet_step(NoteTalkView* self);
void note_talk_react_step(NoteTalkView* self);
void note_talk_state_step(NoteTalkView* self);
void lb_kitchen_page_set(LbKitchenWork* work, s16 page);
void lb_kitchen_row_copy(LbKitchenRow* dst, const LbKitchenRow* src);
void lb_kitchen_courses_roll(LbKitchenWork* work, s16 course);
void lb_kitchen_list_page_set(LbKitchenWork* work, s16 page);
u16* lb_kitchen_pair_find(u8 kind, u8 a, u8 b);
u16 lb_kitchen_bonus_roll(LbKitchenWork* work, u8 chance);
u16 lb_kitchen_extra_roll(LbKitchenWork* work, u8 chance);
void lb_kitchen_special_apply(LbKitchenWork* work);
void lb_kitchen_list_draw(LbKitchenWork* work);
void lb_kitchen_pick_draw(LbKitchenWork* work);
void lb_kitchen_course_draw(LbKitchenWork* work, s32 active);
void lb_kitchen_confirm_draw(LbKitchenWork* work);
void lb_kitchen_special_screen_draw(LbKitchenWork* work);
u8 lb_trade_offer_state(LbTradeOffer* offer, s16 dest);
void lb_trade_page_set(LbTradeWork* work, s16 page);
void lb_trade_draw_task(void);
void lb_kitchen_draw_task(void);
void lb_scene_eft_release(_EFT* self);
void lb_scene_eft_step(_EFT* self);
void lb_scene_eft_init(_EFT* self);
void lb_scene_eft_move(_EFT* self);
u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index);
void lb_quest_board_state_next(_EFT* self);
void lb_quest_board_effect_retire(_EFT* self);

/* --------------------------------------------------------------------------------------------- *
 * The note-pane NPC talk program.
 * --------------------------------------------------------------------------------------------- */

/* 0x8038EEC8 (0x54): Moves the work's 16-bit angle one 1820-step towards its target, snapping when it is inside one
 * step, and wrapping through 0 the way the record's own 16-bit field does. */
void note_turn_step(NoteTalkView* self) {
    u32 target = self->target_0x1A8;
    u32 cur = self->angle_0x18C;
    u16 diff = (u16)(target - (u16)cur);
    if ((u16)(diff + 1820) < 3640) {
        self->angle_0x18C = target;
    } else if (diff < 0x8000) {
        self->angle_0x18C = (u16)(cur + 1820);
    } else {
        self->angle_0x18C = (u16)(cur - 1820);
    }
}

/* 0x8038EF9C (0x50): Runs the program's frame: the state arm, once more when the work raised its one-shot flag, then
 * the turn step. */
void note_talk_frame(NoteTalkView* self) {
    note_talk_state_step(self);
    if (self->again_0x1A1 == 1) {
        note_talk_state_step(self);
        self->again_0x1A1 = 0;
    }
    note_turn_step(self);
}

/* 0x8038F264 (0x30): Runs the mode the work's +0x19F byte selects. */
void note_talk_mode_step(NoteTalkView* self) {
    switch (self->mode_0x19F) {
    case 0:
        note_talk_wait_step(self);
        break;
    case 1:
        note_talk_greet_step(self);
        break;
    case 2:
        note_talk_react_step(self);
        break;
    }
}

/* 0x8038F294 (0x4): The empty state arm. */
void note_talk_noop(void) {
}

/* 0x8038F298 (0x24): Runs the arm the work's +0x19D byte selects. */
void note_talk_state_step(NoteTalkView* self) {
    switch (self->state_0x19D) {
    case 0:
        note_talk_mode_step(self);
        break;
    case 1:
        note_talk_noop();
        break;
    }
}

/* --------------------------------------------------------------------------------------------- *
 * The kitchen screen.
 * --------------------------------------------------------------------------------------------- */

/* 0x8038F2BC (0x88): The triangular number below a count - how many pair keys the lower groups take.  The second
 * argument is part of the retail call shape and unused here. */
s32 lb_kitchen_tri_sum(u16 count) {
    s32 sum = 0;
    s32 value = count - 1;
    u16 n;
    for (n = count - 1; n != 0; n--) {
        sum += value;
        value--;
    }
    return sum;
}

/* 0x8038F344 (0x54): The pair's bit index: the smaller ingredient plus the keys the larger one's pairs start at. */
u16 lb_kitchen_pair_index(u16 a, u16 b) {
    u16 low;

    if (a < b) {
        low = a;
    } else {
        low = b;
        b = a;
    }
    return low + lb_kitchen_tri_sum(b);
}

/* 0x8038F398 (0x9C): Marks the pair as tried - in the offline set while the game is not online, in the online one
 * afterwards. */
void lb_kitchen_pair_seen_set(u16 a, u16 b) {
    u16 key = lb_kitchen_pair_index(a, b);
    if (game_ready_ck() == 0) {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        user->kitchen_pairs_0x516C[(u32)key >> 3] |= 1 << (key & 7);
    } else {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        user->kitchen_pairs_0x525C[(u32)key >> 3] |= 1 << (key & 7);
    }
}

/* 0x8038F434 (0xA4): Whether the pair has been tried. */
int lb_kitchen_pair_seen_ck(u16 a, u16 b) {
    u16 key = lb_kitchen_pair_index(a, b);
    if (game_ready_ck() == 0) {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        return (user->kitchen_pairs_0x516C[(u32)key >> 3] & (1 << (key & 7))) != 0;
    }
    Q_UserData* user = (Q_UserData*)lobby_world_block;
    return (user->kitchen_pairs_0x525C[(u32)key >> 3] & (1 << (key & 7))) != 0;
}

/* 0x8038F748 (0x4): Closes the kitchen screen. */
s32 lb_kitchen_close(void) {
    return lb_panel_close();
}

/* 0x8038F74C (0x138): Shows ingredient page `page`: copies its six rows and marks each one pickable when the meal is
 * affordable and the row is not the first pick. */
void lb_kitchen_page_set(LbKitchenWork* work, s16 page) {
    u8 enabled;
    s32 i;

    work->row_count = 6;
    work->page = page;
    work->page_count = 2;
    memset(work->page_rows, 0, sizeof(work->page_rows));
    if (lb_kitchen_cost_tbl[work->tier * 2] > ((Q_UserData*)lobby_world_block)->zenny_0x18 &&
        (game_ready_ck() == 1 ||
         lb_kitchen_cost_tbl[work->tier * 2 + 1] > ((Q_UserData*)lobby_world_block)->points_0x3F04)) {
        enabled = 0;
    } else {
        enabled = 1;
    }
    for (i = 0; i < 6; i++) {
        lb_kitchen_row_copy(&work->page_rows[i], &work->rows[page * 6 + i]);
        if (work->picks[0].id == work->page_rows[i].id) {
            work->page_rows[i].enabled = 0;
        } else {
            work->page_rows[i].enabled = enabled;
        }
    }
    if (work->row_cursor >= work->row_count) {
        work->row_cursor = work->row_count - 1;
    }
}

/* 0x8038F884 (0x24): Copies one ingredient row's five bytes. */
void lb_kitchen_row_copy(LbKitchenRow* dst, const LbKitchenRow* src) {
    dst->id = src->id;
    dst->rare = src->rare;
    dst->group = src->group;
    dst->enabled = src->enabled;
}

/* 0x8038F8A8 (0xFC): Whether bonus course `course` can be rolled: a handful of courses need an unlock (offline or
 * online) or are offline-only. */
u32 lb_kitchen_course_ck(u8 course) {
    u32 ok = 1;

    switch (course) {
    case 26:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(35);
        } else {
            ok = 0;
        }
        break;
    case 27:
        if (game_ready_ck() == 0) {
            ok = 0;
        }
        break;
    case 47:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(33);
        } else {
            ok = lb_unlock_cond_ck(20);
        }
        break;
    case 48:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(31);
        } else {
            ok = lb_unlock_cond_ck(16);
        }
        break;
    case 25:
        if (game_ready_ck() == 1) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* 0x8038FAC4 (0x5C): Empties both picks and shows the first ingredient page and the first course roll. */
void lb_kitchen_pick_reset(LbKitchenWork* work) {
    memset(work->picks, 0xFF, sizeof(work->picks));
    work->step = 0;
    work->row_cursor = 0;
    lb_kitchen_page_set(work, 0);
    lb_kitchen_courses_roll(work, 0);
}

/* 0x8038FB20 (0x450): The pick page's input: the ingredient list, then the course choice, then the payment; 1 once
 * the meal is ordered, 2 when the page is left. */
s32 lb_kitchen_pick_input(LbKitchenWork* work) {
    s32 result = 0;
    s16 page;
    s16 course;
    s16 cursor;

    switch (work->step) {
    case 0:
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->page_rows[work->row_cursor].enabled == 1) {
                if (work->picks[0].id == 0xFFFF) {
                    lb_kitchen_row_copy(&work->picks[0], &work->page_rows[work->row_cursor]);
                    work->page_rows[work->row_cursor].enabled = 0;
                    sysSE_req(0);
                } else {
                    lb_kitchen_row_copy(&work->picks[1], &work->page_rows[work->row_cursor]);
                    work->step = 1;
                    work->course_cursor = 0;
                    lb_kitchen_courses_roll(work, 0);
                    sysSE_req(27);
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            if (work->picks[0].id == 0xFFFF) {
                result = 2;
            } else {
                work->picks[0].id = 0xFFFF;
                lb_kitchen_page_set(work, work->page);
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->row_cursor = menu_cursor_step(work->row_cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                               &work->page_moved);
            work->page = page;
            lb_kitchen_page_set(work, page);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(16) != 0) {
            work->step = 2;
            work->confirm_cursor = 0;
            work->confirm_enabled[0] = 1;
            work->confirm_enabled[1] = 1;
            work->confirm_enabled[2] = 1;
            if (work->choice.cursor == 0) {
                if (game_ready_ck() == 0) {
                    work->pay_menu = 0;
                    work->confirm_count = 3;
                    if (lb_kitchen_cost_tbl[work->tier * 2] > ((Q_UserData*)lobby_world_block)->zenny_0x18) {
                        work->confirm_enabled[0] = 0;
                    }
                    if (lb_kitchen_cost_tbl[work->tier * 2 + 1] > ((Q_UserData*)lobby_world_block)->points_0x3F04) {
                        work->confirm_enabled[1] = 0;
                    }
                } else {
                    work->pay_menu = 2;
                    work->confirm_count = 2;
                }
            } else {
                work->pay_menu = 1;
                work->confirm_count = 2;
            }
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 0;
            work->picks[1].id = 0xFFFF;
            lb_kitchen_page_set(work, work->page);
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            course = menu_cursor_step(work->course_cursor, 4, lb_cmd_repeat_get(), 1, 2);
            work->course_cursor = course;
            lb_kitchen_courses_roll(work, course);
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(16) != 0) {
            cursor = work->confirm_cursor;
            if (work->confirm_enabled[cursor] == 1) {
                switch (cursor) {
                default:
                    work->step = 1;
                    sysSE_req(1);
                    break;
                case 1:
                    if (work->pay_menu == 0) {
                        work->payment = 2;
                        result = 1;
                        sysSE_stop(21);
                    } else {
                        work->step = 1;
                        sysSE_req(1);
                    }
                    break;
                case 0:
                    if (work->pay_menu == 1) {
                        work->payment = 0;
                        result = 1;
                        sysSE_req(0);
                    } else {
                        work->payment = 1;
                        result = 1;
                        sysSE_req(9);
                    }
                    break;
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->confirm_cursor = menu_cursor_step(work->confirm_cursor, work->confirm_count, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    }
    return result;
}

/* 0x8038FF70 (0xDC): Fills the special list from page `page` of the special-course table (up to six, 0-ended). */
void lb_kitchen_list_page_set(LbKitchenWork* work, s16 page) {
    u8* course;
    s32 i;

    work->page = page;
    work->page_count = 2;
    work->row_count = 0;
    course = &lb_kitchen_special_tbl[(s16)(page * 6)];
    for (i = 0; i < 6; i++) {
        if (*course == 0) {
            break;
        }
        work->list[work->row_count] = *course;
        work->row_count++;
        course++;
    }
    if (work->list_cursor >= work->row_count) {
        work->list_cursor = work->row_count - 1;
    }
}

/* The item-hold strip's entry `lb_kitchen_special_open` hands `eft052_hold_entry_set`. size: 0x1C */
typedef struct LbKitchenHoldEntry {
    /* +0x00 */ s32 kind;
    /* +0x04 */ s16 id_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s16 id_0x08;
    /* +0x0A */ s16 value_0x0A;
    /* +0x0C */ s16 count;
    /* +0x0E */ s16 label;
    /* +0x10 */ s16 flag_0x10;
    /* +0x12 */ u8 pad_0x12[0x2];
    /* +0x14 */ u16* holds;
    /* +0x18 */ s8 flag_0x18;
    /* +0x19 */ u8 pad_0x19[0x3];
} LbKitchenHoldEntry;

/* 0x8039004C (0x108): Opens the special-course pages: clears the picks, collects the special courses the player holds
 * and shows them in the item-hold strip. */
void lb_kitchen_special_open(LbKitchenWork* work) {
    LbKitchenHoldEntry entry;
    s32 i;
    u16* hold;
    u16* slot;

    memset(work->special, 0, 6);
    work->step = 0;
    work->special_count = 0;
    work->special_id = 0;
    memset(work->holds, 0, sizeof(work->holds));
    hold = lb_kitchen_special_holds;
    slot = work->holds;
    for (i = 0; i < 2; i++) {
        if (userdata_item_count_total(*hold, lobby_world_block) != 0) {
            *slot = *hold;
            slot++;
        }
        hold += 2;
    }
    entry.kind = 0;
    entry.id_0x04 = 0x16C;
    entry.value_0x06 = -1;
    entry.value_0x0A = -1;
    entry.id_0x08 = 0x16D;
    entry.label = 0x13E7;
    entry.flag_0x10 = 0;
    entry.count = 2;
    entry.holds = work->holds;
    entry.flag_0x18 = 0;
    eft052_hold_entry_set(&entry, 1);
    lb_kitchen_list_page_set(work, 0);
}

/* 0x803905FC (0x60): Finds the pair (`a`, `b`) in tier `kind`'s pair table: 6-byte records keyed by the two groups
 * (smaller one high), ended by 0xFFFF, whose address the not-found answer is. */
u16* lb_kitchen_pair_find(u8 kind, u8 a, u8 b) {
    u16* p = lb_kitchen_pair_tbls[kind];
    u16 key;
    if (a < b) {
        key = (u16)((a << 8) | b);
    } else {
        key = (u16)((b << 8) | a);
    }
    while (*p != 0xFFFF) {
        if (key == *p) {
            return p;
        }
        p += 3;
    }
    return p;
}

/* 0x803907F4 (0x134): Copies a skill row's two skills into the meal parameters: rows 0x8000.. pick two of the bonus
 * table's five rows by the roll seed, the others read the meal skill table. */
void lb_kitchen_skill_apply(LbKitchenWork* work, u16 skill) {
    s16* table;
    u16 seed;
    s16 first;
    s16 second;
    s16* pair;
    MealSkill* row;

    if ((skill & 0x8000) != 0) {
        table = lb_kitchen_bonus_tbls[(u8)skill];
        seed = work->seed;
        first = (seed >> 2) % 5;
        pair = &table[first * 2];
        lb_param_w.flag_0x0C[0] = pair[0];
        lb_param_w.value_0x10[0] = pair[1];
        second = (first + (seed >> 3) % 4 + 1) % 5;
        pair = &table[second * 2];
        lb_param_w.flag_0x0C[1] = pair[0];
        lb_param_w.value_0x10[1] = pair[1];
        lb_param_w.flag_0x0C[2] = 0;
        lb_param_w.value_0x10[2] = 0;
        return;
    }
    row = &meal_skill_tbl[skill];
    lb_param_w.flag_0x0C[0] = row->kind_a;
    lb_param_w.value_0x10[0] = row->value_a;
    lb_param_w.flag_0x0C[1] = row->kind_b;
    lb_param_w.value_0x10[1] = row->value_b;
    lb_param_w.flag_0x0C[2] = 0;
    lb_param_w.value_0x10[2] = 0;
}

/* 0x80390928 (0x5C): Copies the three special picks into the meal parameters with each one's value. */
void lb_kitchen_special_apply(LbKitchenWork* work) {
    for (s32 i = 0; i < 3; i++) {
        lb_param_w.flag_0x0C[i] = work->special[i];
        s16 value = lb_kitchen_special_values[work->special[i]];
        lb_param_w.value_0x10[i] = value;
    }
}

/* 0x80390984 (0x59C): Serves the meal: pays for it, applies the pair's (or the special course's) skills, rolls the
 * bonus courses, hands the result to the cook and the meal parameters, and re-draws the roll seed. */
void lb_kitchen_meal_serve(LbKitchenWork* work) {
    u8 count = 0;
    u16* pair;
    s16 value;
    u16* bonus;
    MealSkill* skill;

    lobby_w.kitchen_busy_0x054 = 1;
    sysSE_stop(9);
    work->bonuses[0] = 0;
    work->bonuses[1] = 0;
    work->bonuses[2] = 0;
    work->bonuses[3] = 0;
    work->message = 0x66;
    switch (work->choice.cursor) {
    case 0:
        pair = lb_kitchen_pair_find(work->tier, work->picks[0].group, work->picks[1].group);
        if (work->picks[0].group != 5 || work->picks[1].group != 5) {
            lb_kitchen_pair_seen_set(work->picks[0].id, work->picks[1].id);
        }
        skill = &meal_skill_tbl[pair[1]];
        work->meal = pair[2];
        work->skill = pair[1];
        value = skill->value_a;
        if (value < 0) {
            work->act = 12;
        } else if (value > 0) {
            work->act = 15;
        } else {
            work->act = 14;
        }
        work->message = pair[2];
        if (pair[2] == 0x8F) {
            work->act = 36;
        }
        lb_kitchen_skill_apply(work, work->skill);
        switch (work->payment) {
        case 1:
            score_add_clamped(-lb_kitchen_cost_tbl[work->tier * 2], &((Q_UserData*)lobby_world_block)->zenny_0x18);
            break;
        case 2:
            userdata_zenny_add(-lb_kitchen_cost_tbl[work->tier * 2 + 1]);
            break;
        }
        switch (work->picks[1].rare + work->picks[0].rare) {
        case 2:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 80);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 60);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 30);
            break;
        case 1:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 75);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 10);
            break;
        case 0:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 80);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 5);
            break;
        }
        break;
    case 1:
        switch (work->special_id) {
        case 0x1B0:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 75);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 50);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 15);
            if (*bonus == 0) {
                *bonus = lb_kitchen_bonus_roll(work, 15);
            }
            break;
        default:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 90);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 50);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 25);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 10);
            if (*bonus == 0) {
                *bonus = lb_kitchen_bonus_roll(work, 10);
            }
            break;
        }
        lb_kitchen_special_apply(work);
        work->act = 15;
        work->message = 0x7A;
        eft052_page_count_add(work->special_id, 1);
        break;
    }
    lb_param_w.value_0x16 = work->bonuses[0];
    work->npc->deco_skill_id[0] = work->bonuses[0];
    lb_param_w.value_0x18 = work->bonuses[1];
    work->npc->deco_skill_id[1] = work->bonuses[1];
    lb_param_w.value_0x1A = work->bonuses[2];
    work->npc->deco_skill_id[2] = work->bonuses[2];
    lb_param_w.value_0x1C = work->bonuses[3];
    work->npc->deco_skill_id[3] = work->bonuses[3];
    if (game_ready_ck() == 1 && work->choice.cursor == 1 && work->online == 1) {
        meal_result_send(lobby_w.meal_area_0x149);
    }
    ((Q_UserData*)lobby_world_block)->kitchen_seed_0x484E = ran_suu(1);
}

/* 0x803914D4 (0x9C): Draws the kitchen's frame (its wide-screen variant), its title and the money and points. */
void lb_kitchen_frame_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x20B1, &pos);
        draw_sprite_ary(lb_kitchen_frame_wide_ids, &pos);
    } else {
        get_lsp_data(0x20AC, &pos);
        draw_sprite_ary(lb_kitchen_frame_ids, &pos);
    }
    get_lsp_data(0x20B6, &pos);
    draw_sprite_ary(lb_kitchen_title_ids, &pos);
    menu_money_draw(0x20ED);
    if (game_ready_ck() == 0) {
        lb_points_draw(0x20EC);
    }
}

/* 0x80391570 (0x200): Draws the ingredient page: each row's name in its pick/enabled colour, the rare mark and the
 * page arrows. */
void lb_kitchen_list_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    bool active = work->step == 0;
    LbKitchenRow* row;
    u16* lsp;
    u16* rare;
    s32 i;
    bool cursor;
    bool enabled;
    u32 color;
    u16 name;

    get_lsp_data(0x20EF, &base);
    draw_sprite_ary(lb_kitchen_list_ids, &base);
    draw_sprite_idx(0x20F0, &base);
    lb_page_arrow_draw(0x20EE, work->page, work->page_count, work->page_moved, active);
    for (i = 0, row = work->page_rows, lsp = lb_kitchen_row_lsp, rare = lb_kitchen_rare_lsp; i < 6; i++) {
        cursor = i == work->row_cursor;
        enabled = row->enabled != 0;
        if (work->picks[0].id == row->id || work->picks[1].id == row->id) {
            color = GetMenuFontColor(true, true, false, false);
        } else {
            color = GetMenuFontColor(enabled, cursor, active, false);
        }
        switch (work->tier) {
        case 0:
            name = row->id + 23;
            break;
        case 1:
            name = row->id + 41;
            break;
        case 2:
            name = row->id + 59;
            break;
        }
        if (!active) {
            cursor = false;
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_list_row_draw((s8*)LbStr(0, name), cursor, &pos, color, 13);
        if (row->rare != 0) {
            get_lsp_data(*rare, &pos);
            pos.x += base.x;
            pos.y += base.y;
            draw_sprite_idx(0x2108, &pos);
        }
        row++;
        lsp++;
        rare++;
    }
}

/* 0x80391770 (0x10): Draws the pulsing highlight of the slot being filled. */
void lb_kitchen_slot_pulse_draw(_mh_ivec2_* pos) {
    lb_sprite_pulse_draw(lb_kitchen_slot_pulse_ids, pos);
}

/* 0x80391780 (0x1BC): Writes a skill's effect text: nothing, its value (a sixth of it for kind 2), or the
 * strong/medium/weak word its value's size picks. */
void lb_kitchen_skill_text(char* out, u8 kind, u16 str, s16 value) {
    s32 limit = abs(value);
    s16 size = limit;

    switch (kind) {
    case 0:
        sprintf(out, lb_kitchen_skill_none_fmt);
        break;
    case 1:
    case 3:
        sprintf(out, lb_kitchen_skill_value_fmt, LbStr(1, str), size);
        break;
    case 2:
        sprintf(out, lb_kitchen_skill_value_fmt, LbStr(1, str), size / 6);
        break;
    case 5:
        if (value >= 15) {
            sprintf(out, lb_kitchen_text_fmt, LbStr(0, 0x93));
        } else if (value >= 10) {
            sprintf(out, lb_kitchen_text_fmt, LbStr(0, 0x94));
        } else {
            sprintf(out, lb_kitchen_text_fmt, LbStr(0, 0x95));
        }
        break;
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        limit = 5;
    default:
        if (value >= limit) {
            sprintf(out, lb_kitchen_text_fmt, LbStr(0, 0x93));
        } else {
            sprintf(out, lb_kitchen_text_fmt, LbStr(0, 0x95));
        }
        break;
    }
}

/* 0x8039193C (0x418): Draws the pick panel: the meal's costs, the two pick slots (the one being filled pulsing) and,
 * once the first pick is in, what the pair with the row under the cursor makes. */
void lb_kitchen_pick_draw(LbKitchenWork* work) {
    char text[0x40];
    char second[0x10];
    char first[0x10];
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s32 filling;
    s32 i;
    LbKitchenRow* pick;
    u16* label;
    u16* lsp;
    u16 name;
    LbKitchenRow* row;
    MealSkill* skill;
    void* str;

    get_lsp_data(0x211A, &base);
    draw_sprite_ary(lb_kitchen_pick_ids, &base);
    sprintf(text, lb_kitchen_cost_fmt, lb_kitchen_cost_tbl[work->tier * 2], LbStr(1, 2));
    draw_font_idx(0x212D, (s8*)text, 6, &base);
    if (game_ready_ck() == 0) {
        sprintf(text, lb_kitchen_cost_fmt, lb_kitchen_cost_tbl[work->tier * 2 + 1], LbStr(1, 44));
        draw_font_idx(0x212C, (s8*)text, 6, &base);
    }
    get_lsp_data(0x212E, &base);
    uv_pair_copy(&pos, &base);
    if (work->picks[0].id == 0xFFFF) {
        filling = 0;
    } else {
        filling = -1;
        if (work->picks[1].id == 0xFFFF) {
            filling = 1;
        }
    }
    pick = work->picks;
    label = lb_kitchen_slot_label_ids;
    lsp = lb_kitchen_slot_lsp;
    for (i = 0; i < 2; i++) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        if (filling == i) {
            lb_kitchen_slot_pulse_draw(&pos);
        }
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(*label, &pos);
        if (pick->id != 0xFFFF) {
            if (pick->rare != 0) {
                draw_sprite_idx(0x2147, &pos);
            }
            switch (work->tier) {
            case 0:
                name = pick->id + 23;
                break;
            case 1:
                name = pick->id + 41;
                break;
            case 2:
                name = pick->id + 59;
                break;
            }
            draw_font_idx(0x212F, (s8*)LbStr(0, name), 4, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        pick++;
        label++;
        lsp++;
    }
    if (work->picks[0].id != 0xFFFF) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(0x2132, &pos);
        row = &work->page_rows[work->row_cursor];
        if (work->picks[0].id == row->id) {
            sprintf(text, lb_kitchen_text_fmt, LbStr(0, 100));
        } else if (lb_kitchen_pair_seen_ck(work->picks[0].id, row->id) != 0) {
            skill = &meal_skill_tbl[lb_kitchen_pair_find(work->tier, work->picks[0].group, row->group)[1]];
            if (skill->kind_a == 0) {
                sprintf(text, lb_kitchen_text_fmt, LbStr(0, 89));
            } else {
                lb_kitchen_skill_text(first, skill->kind_a, (skill->value_a < 0) + 8, skill->value_a);
                if (skill->kind_b == 0) {
                    sprintf(text, lb_kitchen_skill1_fmt, LbStr(0, skill->kind_a + 89), first);
                } else {
                    lb_kitchen_skill_text(second, skill->kind_b, (skill->value_b < 0) + 8, skill->value_b);
                    str = LbStr(0, skill->kind_b + 89);
                    sprintf(text, lb_kitchen_skill2_fmt, LbStr(0, skill->kind_a + 89), first, str, second);
                }
            }
        } else {
            sprintf(text, lb_kitchen_text_fmt, LbStr(0, 101));
        }
        draw_font_idx(0x2131, (s8*)text, 5, &pos);
    }
}

/* 0x80391D54 (0x1A0): Draws the special pick panel: one slot per pick the course takes, the slot being filled
 * pulsing, and each pick's name. */
void lb_kitchen_special_pick_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s32 filling;
    s32 i;
    u16* label;
    u16* lsp;

    get_lsp_data(0x211A, &base);
    draw_sprite_ary(lb_kitchen_pick_ids, &base);
    draw_font_idx(0x212D, (s8*)LbStr(0, 0x58), 4, &base);
    get_lsp_data(0x212E, &base);
    uv_pair_copy(&pos, &base);
    if (work->special[0] == 0) {
        filling = 0;
    } else if (work->special[1] == 0) {
        filling = 1;
    } else if (work->special[2] == 0 && work->special_count == 3) {
        filling = 2;
    } else {
        filling = -1;
    }
    label = lb_kitchen_special_label_ids;
    lsp = lb_kitchen_slot_lsp;
    for (i = 0; i < work->special_count; i++) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        if (filling == i) {
            lb_kitchen_slot_pulse_draw(&pos);
        }
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(*label, &pos);
        if (work->special[i] != 0) {
            draw_font_idx(0x212F, (s8*)LbStr(0, work->special[i] + 0x59), 4, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        label++;
        lsp++;
    }
}

/* 0x803921A4 (0x184): Draws the payment box: one row per payment the menu offers, the cursor's row lit. */
void lb_kitchen_confirm_draw(LbKitchenWork* work) {
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 str;
    u32 i;
    u16* lsp;
    bool cursor;
    u32 flags;
    u32 color;

    get_lsp_data(0x2181, &base);
    draw_sprite_anim_ary(lb_kitchen_confirm_ids, work->confirm_count, &base);
    switch (work->pay_menu) {
    case 1:
        str = 0x54;
        break;
    case 2:
        str = 0x56;
        break;
    default:
        str = 0x51;
        break;
    }
    get_lsp_data(0x2189, &base);
    uv_pair_copy(&pos, &base);
    lsp = lb_kitchen_confirm_lsp;
    for (i = 0; i < (u32)work->confirm_count; i++) {
        if (i == work->confirm_cursor) {
            cursor = 1;
            flags = 5;
        } else {
            cursor = 0;
            flags = 1;
        }
        color = GetMenuFontColor(work->confirm_enabled[i], cursor, true, false);
        draw_sprite_ary(lb_kitchen_confirm_row_ids, &pos);
        spr_data_copy(&spr, get_lsp_data(0x218A, NULL));
        spr.color = color;
        draw_font(spr, (s8*)LbStr(0, str), flags, &pos);
        if (cursor != 0) {
            put_menu_cursor(lb_kitchen_confirm_cursor_ids, 0, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        str++;
        lsp++;
    }
}

/* 0x80392328 (0x6C): Draws the pick screen: the list, the picks and, past the list, the course and payment boxes. */
void lb_kitchen_pick_screen_draw(LbKitchenWork* work) {
    lb_kitchen_list_draw(work);
    lb_kitchen_pick_draw(work);
    if (work->step == 2) {
        lb_kitchen_confirm_draw(work);
        lb_kitchen_course_draw(work, 0);
    } else if (work->step != 0) {
        lb_kitchen_course_draw(work, 1);
    }
}

/* 0x80392394 (0x170): Draws the special list: each course's name, lit when picked, and the page arrows. */
void lb_kitchen_special_list_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    bool active = work->step == 1;
    s32 i;
    u16* lsp;
    bool cursor;
    bool picked;
    u32 color;

    get_lsp_data(0x20EF, &base);
    draw_sprite_ary(lb_kitchen_list_ids, &base);
    draw_sprite_idx(0x20F1, &base);
    lb_page_arrow_draw(0x20EE, work->page, work->page_count, work->page_moved, active);
    for (i = 0, lsp = lb_kitchen_row_lsp; i < work->row_count; i++) {
        cursor = i == work->list_cursor;
        if (work->special[0] == work->list[i] || work->special[1] == work->list[i] ||
            work->special[2] == work->list[i]) {
            picked = true;
        } else {
            picked = false;
        }
        color = GetMenuFontColor(true, cursor, active, picked);
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        if (!active) {
            cursor = false;
        }
        lb_list_row_draw((s8*)LbStr(0, work->list[i] + 0x59), cursor, &pos, color, 13);
        lsp++;
    }
}

/* 0x803925A0 (0x25C): Draws the help panel's message and its second line for the current step. */
void lb_kitchen_msg_draw(LbKitchenWork* work) {
    u16 msg = 0xFFFF;
    u16 sub = 0xFFFF;
    s32 color = 2;
    s32 line = 1;
    LbKitchenRow* row;
    u16 pick;

    switch (work->state) {
    case 4:
        switch (work->step) {
        case 0:
            pick = work->picks[0].id;
            msg = (pick != 0xFFFF) + 0x16F;
            row = &work->page_rows[work->row_cursor];
            if (row->enabled == 0) {
                if (pick == row->id) {
                    sub = 0x171;
                } else if (game_ready_ck() == 1) {
                    sub = 0x175;
                    line = 2;
                } else {
                    sub = 0x170;
                }
            }
            break;
        case 1:
            msg = 0x172;
            sub = 0x173;
            color = 5;
            break;
        case 2:
            switch (work->confirm_cursor) {
            case 0:
                msg = 0x174;
                if (work->confirm_enabled[work->confirm_cursor] == 0) {
                    sub = 0x175;
                    line = 2;
                }
                break;
            case 1:
                if (game_ready_ck() == 1) {
                    msg = 0x178;
                } else {
                    msg = 0x176;
                    if (work->confirm_enabled[work->confirm_cursor] == 0) {
                        sub = 0x177;
                        line = 2;
                    }
                }
                break;
            case 2:
                msg = 0x178;
                break;
            }
            break;
        }
        break;
    case 3:
        switch (work->step) {
        case 1:
            if (work->special[0] == 0) {
                msg = 0x179;
            } else {
                msg = 0x17B - (work->special[1] == 0);
            }
            if (work->special[0] == work->list[work->list_cursor] ||
                work->special[1] == work->list[work->list_cursor]) {
                sub = 0x17C;
            }
            break;
        case 2:
            msg = 0x172;
            break;
        case 3:
            switch (work->confirm_cursor) {
            case 0:
                msg = 0x17D;
                break;
            case 1:
                msg = 0x178;
                break;
            }
            break;
        }
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (sub != 0xFFFF) {
            lb_panel_line_draw(0x1877, sub, color, line);
        }
    }
}

/* 0x803927FC (0xCC): The kitchen's draw callback: the frame, then the opening choice, the pick screen or the special
 * screen, then the help panel. */
void lb_kitchen_draw_task(void) {
    LbKitchenWork* work = (LbKitchenWork*)lobby_w.menu_0xAC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_kitchen_frame_draw();
    switch (work->state) {
    case 2:
        lb_choice_draw(&work->choice, 0x20C4, lb_kitchen_choice_sprites, lb_kitchen_choice_ids, 0x1877, 0);
        get_lsp_data(0x20C4, &pos);
        draw_sprite_idx(0x20C5, &pos);
        break;
    case 4:
        lb_kitchen_pick_screen_draw(work);
        break;
    case 3:
        lb_kitchen_special_screen_draw(work);
        break;
    }
    lb_kitchen_msg_draw(work);
}

/* 0x803928C8 (0x28): Whether the kitchen is outside its menu states (2..4). */
u32 lb_kitchen_idle_ck(void) {
    return (u32)(((LbKitchenWork*)lobby_w.menu_0xAC)->state - 2) > 2;
}

/* --------------------------------------------------------------------------------------------- *
 * The trade screen.
 * --------------------------------------------------------------------------------------------- */

/* 0x803928F0 (0x70): Opens the trade screen: clears the whole screen block, files the opener's records and puts the
 * lobby into screen 0x12. */
void lb_trade_open(s32 arg, LbTradeSource* source) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;

    memset(work, 0, 0x2000);
    work->arg_0x058 = arg;
    work->source = source;
    lobby_w.state_0x000 = 0x12;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* 0x80392960 (0x4): Closes the trade screen. */
s32 lb_trade_close(void) {
    return lb_panel_close();
}

/* 0x80392A4C (0x17C): Fills trade page `page` (five offers of the ten, or fifteen during the event) with each offer's
 * state and the prices the player can pay. */
void lb_trade_page_set(LbTradeWork* work, s16 page) {
    s32 first = page * 5;
    s32 last = first + 4;
    s32 count;
    s32 i;
    s32 j;
    s16 index;
    LbTradeOffer* offer;

    work->row_count = 0;
    memset(work->rows, 0, sizeof(work->rows));
    index = 0;
    offer = (LbTradeOffer*)NetCtrlWk::getServerNotice();
    count = 10;
    if (lobby_w.field_0x15E == 1) {
        count = 15;
    }
    for (i = 0; i < count; i++) {
        if (i >= first && i <= last) {
            work->rows[work->row_count].index = index;
            work->rows[work->row_count].state = lb_trade_offer_state(offer, work->choice.cursor);
            for (j = 0; j < 3; j++) {
                if (userdata_item_count_total(offer->costs[j].item, lobby_world_block) < offer->costs[j].count) {
                    work->rows[work->row_count].affordable[j] = 0;
                } else {
                    work->rows[work->row_count].affordable[j] = 1;
                }
            }
            work->row_count++;
        }
        index++;
        offer++;
    }
    if (work->row_cursor >= work->row_count) {
        work->row_cursor = work->row_count - 1;
    }
    work->page = page;
    work->page_count = menu_page_count(index, 5);
}

/* 0x80392BC8 (0x100): Makes the picked exchange: takes the price, gives the item to the pouch or the box and
 * refreshes the page; 1 while the same exchange can be made again. */
s32 lb_trade_exchange(LbTradeWork* work) {
    LbTradeOffer* offer;
    LbTradeCost item;
    LbTradeCost cost;
    u8 stored[4];
    LbTradeRow* row;

    offer = &((LbTradeOffer*)NetCtrlWk::getServerNotice())[work->rows[work->row_cursor].index];
    item_pair_copy(&item, &offer->item);
    item_pair_copy(&cost, &offer->costs[work->cost_cursor]);
    work->hold = 4;
    eft052_page_count_add(cost.item, cost.count);
    if (work->choice.cursor == 0) {
        userdata_item_give(lobby_world_block, item.item, item.count, 1);
    } else {
        item_box_store(item.item, item.count, stored);
    }
    lb_trade_page_set(work, work->page);
    row = &work->rows[work->row_cursor];
    if (row->state == 0 && row->affordable[work->cost_cursor] == 1) {
        return 1;
    }
    return 0;
}

/* 0x80392CC8 (0x478): The trade screen's frame step: the opening choice, the offer list, the price list, the yes/no
 * and the held repeat; then the draw callback. */
void lb_trade_step(void) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;
    LbTradeOffer* offer;
    s32 again;

    if (work->hold != 0) {
        work->hold--;
    }
    work->page_moved = 0;
    switch (work->state) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->state++;
            lb_choice_init(&work->choice, &lb_trade_choice_def, 0, 0);
            camera_talk_lock_set(1);
            sysSE_stop(41);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->choice)) {
        case 1:
            work->state = 2;
            work->row_cursor = 0;
            lb_trade_page_set(work, 0);
            break;
        case 2:
            work->state = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->rows[work->row_cursor].state == 0) {
                offer = &((LbTradeOffer*)NetCtrlWk::getServerNotice())[work->rows[work->row_cursor].index];
                if (offer->costs[2].item != 0) {
                    work->cost_count = 3;
                } else if (offer->costs[1].item != 0) {
                    work->cost_count = 2;
                } else {
                    work->cost_count = 1;
                }
                work->cost_cursor = 0;
                work->state = 3;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->state = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->row_cursor = menu_cursor_step(work->row_cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            work->page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                                     &work->page_moved);
            lb_trade_page_set(work, work->page);
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->rows[work->row_cursor].affordable[work->cost_cursor] == 1) {
                work->state = 4;
                work->repeat = 0;
                work->yes_no = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->state = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cost_cursor = menu_cursor_step(work->cost_cursor, work->cost_count, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 4:
        ainpc_page_hold_set();
        switch (toggle_word_step(&work->yes_no, lb_cmd_repeat_get(), 4, 8, 0xFFFF)) {
        case 1:
            sysSE_stop(27);
            work->repeat = 16;
            if (lb_trade_exchange(work) == 0) {
                work->state = 2;
            } else {
                work->state = 5;
            }
            break;
        case 2:
            work->state = 3;
            break;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (lb_cmd_held_ck(16) != 0) {
            if (--work->repeat <= 0) {
                work->repeat = 5;
                again = lb_trade_exchange(work);
                sysSE_stop(27);
                if (again == 0) {
                    work->state = 2;
                    sysSE_req(1);
                }
            }
        } else {
            work->state = 3;
        }
        break;
    case 6:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_trade_close();
        }
        break;
    }
    if (work->state != 0 && work->state != 6) {
        subTransSet((u32)lb_trade_draw_task, 0, NULL);
    }
}

/* 0x80393140 (0x80): Draws the trade screen's frame (its wide-screen variant) and its title. */
void lb_trade_frame_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x21A9, &pos);
        draw_sprite_ary(lb_trade_frame_wide_ids, &pos);
    } else {
        get_lsp_data(0x21A4, &pos);
        draw_sprite_ary(lb_trade_frame_ids, &pos);
    }
    get_lsp_data(0x2197, &pos);
    draw_sprite_ary(lb_trade_title_ids, &pos);
}

/* 0x803931C0 (0x228): Draws the offer list, the picked offer's item panel and text, the box/pouch tabs and the page
 * arrows. */
void lb_trade_list_draw(LbTradeWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 item;
    s32 box_mark = 0;
    s32 pouch_mark = 0;
    bool active = work->state == 2;
    LbTradeOffer* offers;
    s32 i;
    u16* lsp;
    LbTradeOffer* offer;
    s32 cursor;
    s32 enabled;

    get_lsp_data(0x21B1, &base);
    draw_sprite_anim_ary(lb_trade_list_ids, 5, &base);
    draw_sprite_idx(lb_trade_label_ids[1], &base);
    offers = (LbTradeOffer*)NetCtrlWk::getServerNotice();
    for (i = 0, lsp = lb_trade_row_lsp; i < work->row_count; i++) {
        offer = &offers[work->rows[i].index];
        if (i == work->row_cursor) {
            cursor = 1;
            item = offer->item.item;
        } else {
            cursor = 0;
        }
        enabled = work->rows[i].state == 0;
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_item_cell_draw(offer->item.item, offer->item.count, &pos, enabled, cursor, active, 0);
        lsp++;
    }
    if (work->hold != 0) {
        if (work->choice.cursor == 0) {
            box_mark = 2;
        }
        if (work->choice.cursor != 0) {
            pouch_mark = 2;
        }
    }
    get_lsp_data(0x21C9, &base);
    draw_sprite_ary(lb_trade_detail_ids, &base);
    lb_item_detail_draw(0x21C9, item, lb_trade_detail_lsp, box_mark, pouch_mark);
    draw_sprite_anim_ary(lb_trade_box_ids, work->choice.cursor == 1, &base);
    draw_sprite_anim_ary(lb_trade_pouch_ids, work->choice.cursor == 0, &base);
    draw_font_idx(0x21E4, (s8*)ItemExp(item), 0, &base);
    get_lsp_data(0x21B1, &base);
    draw_sprite_anim_idx(0x21BB, 5, &base);
    lb_page_arrow_draw(0x21B0, work->page, work->page_count, work->page_moved, active);
}

/* 0x803933E8 (0x368): Draws the picked offer's prices: each price's item cell, its help record, the count it asks and
 * the count held, dimmed when the player holds too few. */
void lb_trade_cost_draw(LbTradeWork* work) {
    _SPR_DATA_ spr;
    char text[0x10];
    PlaceRec place;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    bool active = work->state == 3;
    s32 i;
    s32 offset;
    u16* lsp;
    LbTradeOffer* offer;
    LbTradeCost* cost;
    s32 cursor;
    s32 lit;
    _SPR_DATA_* rect;

    get_lsp_data(0x21EC, &base);
    draw_sprite_ary(lb_trade_cost_ids, &base);
    get_lsp_data(0x2213, &base);
    uv_pair_copy(&pos, &base);
    offset = 0;
    lsp = lb_trade_cost_lsp;
    for (i = 0; i < 3; i++) {
        offer = &((LbTradeOffer*)NetCtrlWk::getServerNotice())[work->rows[work->row_cursor].index];
        cost = (LbTradeCost*)((u8*)offer->costs + offset);
        if (cost->item != 0) {
            if (active) {
                draw_sprite_anim_ary(lb_trade_cost_row_ids, 0, &pos);
            } else {
                draw_sprite_anim_ary(lb_trade_cost_row_ids, 1, &pos);
            }
            if (i == work->cost_cursor && work->state != 2) {
                cursor = 1;
            } else {
                cursor = 0;
            }
            if (!active && work->rows[work->row_cursor].affordable[i] == 1) {
                lit = -1;
            } else {
                lit = 0;
            }
            if (work->state != 2) {
                lit = 0;
            }
            lb_item_cell_draw_wide(cost->item, lit, &pos, work->rows[work->row_cursor].affordable[i], cursor, active,
                                   0x280);
            place_rec_trade_set(&place, work->source->id, offer->item.item, offer->item.count, cost->item,
                                cost->count);
            rect = get_lsp_data(0x2213, NULL);
            place_rec_alloc(&place, pos.x, pos.y, rect->width, rect->height, 4, 14);
            sprintf(text, lb_trade_count_fmt, cost->count);
            spr_data_copy(&spr, get_lsp_data(0x222E, NULL));
            if (work->rows[work->row_cursor].affordable[i] == 0) {
                spr.color = 0x646464FF;
            }
            draw_font(spr, (s8*)text, 1, &pos);
            sprintf(text, lb_trade_count_fmt, userdata_item_count_total(cost->item, lobby_world_block));
            if (cursor != 0 && work->hold != 0) {
                draw_font_anim_idx(0x222F, 2, (s8*)text, 2, &pos);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x222F, NULL));
                if (work->rows[work->row_cursor].affordable[i] == 0) {
                    spr.color = 0x646464FF;
                }
                draw_font(spr, (s8*)text, 2, &pos);
            }
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        offset += 4;
        lsp++;
    }
    font_flush();
    get_lsp_data(0x21EC, &base);
    if (work->state == 2) {
        draw_sprite_ary(lb_trade_cost_cursor_ids, &base);
    }
    draw_sprite_idx(0x2211, &base);
    if (work->state == 2) {
        draw_sprite_idx(0x2212, &base);
    }
    ainpc_page_mark_b_draw(0x2232, NULL);
}

/* 0x80393750 (0x16C): Draws the help panel's message and its second line for the current step (the yes/no while an
 * exchange runs). */
void lb_trade_msg_draw(LbTradeWork* work) {
    u16 msg = 0xFFFF;
    u16 sub = 0xFFFF;
    s32 color = 2;
    s32 line = 1;
    u8 state;
    s16 dest;

    switch (work->state) {
    case 2:
        dest = work->choice.cursor;
        if (dest == 0) {
            msg = 0x22;
        } else {
            msg = 0x2A;
            line = 2;
        }
        state = work->rows[work->row_cursor].state;
        if ((state & 2) != 0) {
            sub = 0x25;
        } else if ((state & 1) != 0) {
            if (dest == 0) {
                sub = 0x23;
            } else {
                sub = 0x24;
            }
        } else if ((state & 4) != 0) {
            sub = 0x31;
        }
        break;
    case 3:
        msg = 0x26;
        if (work->rows[work->row_cursor].affordable[work->cost_cursor] == 0) {
            sub = 0x27;
        }
        break;
    case 4:
    case 5:
        msg = 0x28;
        sub = 0x29;
        color = 5;
        lb_panel_yes_no_draw(0x1879, work->yes_no);
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (sub != 0xFFFF) {
            lb_panel_line_draw(0x1877, sub, color, line);
        }
    }
}

/* 0x803938BC (0xD8): The trade screen's draw callback: the frame, the opening choice or the list and prices, then
 * the help panel. */
void lb_trade_draw_task(void) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_trade_frame_draw();
    if ((u32)(work->state - 2) > 3) {
        if (work->state == 1) {
            lb_choice_draw(&work->choice, 0x21B1, lb_trade_list_ids, lb_trade_choice_ids, 0x1877, 0);
            get_lsp_data(0x21B1, &pos);
            draw_sprite_idx(0x21BC, &pos);
            draw_sprite_idx(0x21BD, &pos);
            draw_sprite_idx(lb_trade_label_ids[0], &pos);
        }
    } else {
        lb_trade_list_draw(work);
        lb_trade_cost_draw(work);
    }
    lb_trade_msg_draw(work);
}

/* --------------------------------------------------------------------------------------------- *
 * The scene effect.
 * --------------------------------------------------------------------------------------------- */

/* 0x80393994 (0xDC): Spawns the scene effect: five pooled models, released again when the pool runs short. */
void lb_scene_eft_spawn(void) {
    _EFT* self = (_EFT*)eft_res_slot_get(0x4C);
    LbSceneEftWork* work;
    s32 i;
    MHchar** model;

    if (self == NULL) {
        return;
    }
    self->release_0x40 = lb_scene_eft_release;
    work = (LbSceneEftWork*)self->work_0x38;
    work->count = 5;
    self->area_0x44 = get_now_areano();
    model = work->models;
    for (i = 0; i < work->count; i++) {
        *model = (MHchar*)eft_res_model_get();
        if (*model == NULL) {
            eft_res_slot_release(self);
            return;
        }
        model++;
    }
    self->source_0x30 = NULL;
    self->dispatch_0x34 = lb_scene_eft_step;
    self->field_0x03 = 0x36;
    eft_state_flags_set(self, 8, 0);
}

/* 0x80393A70 (0x7C): Releases the scene effect's models. */
void lb_scene_eft_release(_EFT* self) {
    LbSceneEftWork* work = (LbSceneEftWork*)self->work_0x38;
    for (s32 i = 0; i < work->count; i++) {
        fn_800E26C4(work->models[i]);
        fn_800F8A44((void*)&work->models[i], 1);
    }
}

/* 0x80393AEC (0x3C): Runs the arm the effect's state byte selects. */
void lb_scene_eft_step(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        lb_scene_eft_init(self);
        break;
    case 1:
        lb_scene_eft_move(self);
        break;
    case 2:
        lb_quest_board_state_next(self);
        break;
    case 3:
        lb_quest_board_effect_retire(self);
        break;
    }
}

/* 0x80394038 (0x10C): Steps one of the scene effect's models: it waits for the lobby's camera mode 2, slides back when
 * the camera work stops, and for model 3 follows the key table by the camera's frame; 1 for model 4. */
u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index) {
    switch (model->step) {
    case 0:
        if (lobby_w.field_0x003 == 2) {
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

/* 0x80394144 (0x10): Advances the work's step byte.  GUESS: the step is what `lb_scene_eft_step`'s arm 2 runs. */
void lb_quest_board_state_next(_EFT* self) {
    self->state_0x05++;
}

/* 0x80394154 (0x4): Retires one pooled effect runtime record. */
void lb_quest_board_effect_retire(_EFT* self) {
    eft_res_slot_release(self);
}

}  /* extern "C" */
