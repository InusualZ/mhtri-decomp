/* lobby/lb_quest_screen.cpp - the quest/multiplayer screen band after `menu/multi_result.cpp`, headed by the note-pane
 *   helpers and ended by the quest work's tail (item-work accessors, quest clock, random source, element flags, spawn
 *   lists and kill bookkeeping).
 * RANGE. .text 0x803A3A50-0x803AA4A4 (95 functions); .data 0x805F2038-0x805F2940 (from the two private switch tables
 *   `jumptable_805F2038`/`jumptable_805F2068`), .sdata 0x80793530-0x807935D0, .sdata2 0x8079C448-0x8079C4C0, extab,
 *   extabindex.  Not one TU: a `--max-bytes` slice of five bands (note pane, lobby item/quest screen, resource loading,
 *   model/quest bookkeeping, enemy area logic; docs/lobby.md); the `.sdata2` run carries the int-to-double magic twice
 *   (0x8079C468, 0x8079C480), one pool per TU.  The unwind runs tile with `menu/multi_result.cpp`'s.
 * FLAGS. `#pragma peephole off` (the target keeps `clrlwi`+`slwi` unfused in both bands: `note_timer_ready_ck` 82.5 ->
 *   100, `quest_clock_byte_get` 92 -> 100) and `#pragma fp_contract off` (no fused multiply-add anywhere in the target;
 *   `quest_enemy_kill_record` keeps `fmuls`+`fadds`).
 * NAMES. Module `lobby` and `lb_quest_screen` are a GUESS from the biggest band (`lobby_w`, `lb_param_w`, `Screen_w`,
 *   `lobby_world_block`, `LbStr`, `lb_item_get_data`, `subTransSet`, `menu_cursor_step`); no `__FILE__` string or dump
 *   name covers the range.  Every function name is a GUESS from its body; the note-pane sub-states from the pane's
 *   state order; the quest tail's from the item-work fields they touch and their callers (`quest_em_*_get`: the four
 *   spawn-argument rows `enemy/enemy_control.cpp` turns into a level and a size).
 *   GUESS (from each body and its callers): quest_arena_item_count_get, quest_area_spawn_setup, quest_area_entry_collect,
 *   GUESS: quest_area_spawn_apply, quest_screen_enemy_start, quest_area_res_load, move_work_item_work_get,
 *   GUESS: quest_time_elapsed_get, quest_time_base_get, quest_time_limit_get, quest_id_set, quest_record_copy,
 *   GUESS: quest_id_get, quest_rand_seed_set, quest_rand_next, quest_clock_byte_get, quest_clock_step,
 *   GUESS: quest_time_limit_set, quest_clock_reset, quest_enemy_kill_dispatch, quest_arena_need_add, quest_arena_key_set,
 *   GUESS: quest_em_stat_tbl_get, quest_em_stat_var_get, quest_em_size_get, quest_em_size_var_get, quest_work_state_get,
 *   GUESS: quest_enemy_kill_record, quest_net_kill_apply, quest_element_set, quest_element_finish, quest_element_done_mark,
 *   GUESS: quest_element_pick_ck, quest_element_failed_ck, quest_element_flag20_ck, quest_element_live_ck,
 *   GUESS: quest_failed_ck, quest_sub_state_end_ck
 *   GUESS (from each body and its callers): lb_interior_slot_free_get, lb_interior_slot_find, lb_interior_load_done
 *   GUESS: lb_interior_load, lb_interior_release, lb_interior_path_get, lb_interior_kind_get, lb_interior_scale_get
 *   GUESS: lb_interior_fx_spawn, lb_interior_path_table, lb_interior_kind_table, lb_interior_scale_table
 *   GUESS (from each body and its callers): note_pane_angle_step, note_pane_init, note_pane_step
 *   GUESS: note_timer_lobby_ready_ck, note_timer_tick, npc_talk_step
 *   GUESS (from each body and its callers): note_trade_open, note_trade_close, note_trade_step
 *   GUESS: note_trade_points_init, note_trade_points_step, note_trade_goods_count, note_trade_offers_roll
 *   GUESS: note_trade_accept, note_trade_select_step, note_trade_frame_draw, note_trade_help_draw
 *   GUESS: note_trade_full_draw, note_trade_slot_draw, note_trade_slots_draw, note_trade_bonus_draw
 *   GUESS: note_trade_offer_draw, note_trade_select_draw, note_itembox_cell_pos_get, note_itembox_row_draw
 *   GUESS: note_itembox_draw, note_trade_route_draw, note_trade_reward_draw, note_trade_voyage_draw, note_trade_draw
 *   GUESS: note_trade_menu_tmpl, note_kind_dice_tbl, note_reward_items_1, note_reward_items_2, note_reward_items_3
 *   GUESS: note_reward_items_4, note_bonus_kind_tbl, note_voyage_tbl, note_dice_0, note_dice_1, note_dice_2
 *   GUESS: note_dice_3, note_dice_4, note_dice_5
 *   GUESS (from each body and its callers): note_title_tbl, note_frame_tbl, note_frame_wide_tbl, note_full_text_tbl
 *   GUESS: note_slot_back_tbl, note_slot_tbl, note_slot_cursor_tbl, note_slots_back_tbl, note_bonus_tbl
 *   GUESS: note_offer_back_tbl, note_offer_arrow_tbl, note_offer_goods_tbl, note_offer_voyage_tbl
 *   GUESS: note_itembox_cell_tbl, note_itembox_back_tbl, note_trade_menu_sprite_tbl, note_itembox_frame_tbl
 *   GUESS: note_route_tbl, note_reward_back_tbl, note_reward_row_tbl, note_voyage_route_tbl, note_full_back_tbl
 *   GUESS: note_full_tbl, note_full_glow_tbl, note_points_tbl, note_arrow_tbl, note_slot_row_tbl, note_full_row_tbl
 *   GUESS: note_offer_kind_tbl, note_offer_points_tbl, note_trade_menu_row_tbl
 *   GUESS (from each body and its callers): lobby_res_slots_init, note_pane_state_0, note_pane_state_1
 *   GUESS: note_pane_state_2, note_pane_state_3, note_pane_state_4, note_pane_state_5, note_pane_state_6
 *   GUESS: note_pane_state_9
 * RESIDUALS. Every row is written (95).  Partial: `note_pane_init` (retail keeps `&vec_0x1B4` in a register),
 *   `note_pane_angle_step`, `note_pane_state_1` (`calcVecAng2`'s second argument: retail passes r4 unset),
 *   `note_pane_pos_step`, `note_timer_tick`, `note_value_to_slot`/`note_slot_to_value` (retail reaches
 *   `note_slot_flat_table` through `r13`, ours through `lis`/`addi`), `note_trade_open` (the loop counters' `li` pair
 *   sits before the table base), `note_trade_offers_roll`/`note_trade_slots_draw`/`note_trade_route_draw`/
 *   `note_trade_select_step`/`note_trade_bonus_draw`/`note_itembox_row_draw`/`note_trade_voyage_draw`/
 *   `note_trade_offer_draw` (register choice); in the tail `quest_area_spawn_apply` (the six-kind membership test:
 *   retail compares without an index register), the kill bookkeeping and spawn-list rows (`quest_enemy_kill_record`
 *   saves one register more, `_savegpr_20`/`_restgpr_20` against `_savegpr_21`/`_restgpr_21`), `quest_element_finish` (`lb_sub16_send`'s owner
 *   spells its flag `s8`, retail's caller narrows with `clrlwi`), `quest_net_kill_apply` (retail tests the element
 *   against 3 with two branches), `quest_arena_key_set`, `quest_arena_need_add`, `quest_area_res_load`,
 *   `quest_failed_ck`, `quest_area_entry_collect`, `lobby_res_slots_init`.
 *   Data: the note trade's tables are defined after the bodies (the target relocates each by name, which MWCC only
 *   does for an object not yet defined); `.data`/`.sdata` emission order is not checked against the target.
 *   flipcheck: `.text`/extab/extabindex short of the claim (partial rows); the `.sdata`/`.sdata2` pool is shared with
 *   `menu/multi_result.cpp` (fold candidate).
 * SHAPES. The note band's `NoteWork` (`enemy/note_work.h` -> `sound/mhchar.h`) and the quest tail's
 *   `quest/quest_entry.h` (-> `pl.h`) define `MHchar` and `_GXChannelID` twice, so the tail reaches its `quest_entry`
 *   callees through the leaf `quest/quest_record_find.h`.  `QuestBossSpawn` (`em_large_spawn`) and `EmAreaEntry`
 *   (`em_area_entry_tbl`) are one 0x44-byte record (`quest_area_spawn_apply` hands the first to
 *   `em_area_entry_release`).  `quest_element_pick_ck`'s `use_alt` test is a conditional expression (an `if` drops a
 *   branch).  `quest_record_copy` is a plain struct assignment: MWCC copies `QuestRecord` member by member, which is
 *   what fixes that record's layout (`quest/quest_types.h`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/note_work.h"
#include "enemy/fn_80382310.h"
#include "ef/fn_800CDB2C.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/quest_element_failed_ck.h"
#include "mh3_pad/vec3.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/Screen_w.h"
#include "quest/quest_record_find.h"
#include "quest/quest_item_slot.h"
#include "ef/get_move_work_adrs.h"
#include "ef/system_core.h"
#include "ef/load_file_req.h"
#include "ef/eft052.h"
#include "NAND/nand.h"
#include "unsplit/OS.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "menu/quest_str_tbl_35_get.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/em_pop.h"
#include "enemy/em_ground_rec_clear.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/em_quest_element_set.h"
#include "enemy/em_prog_support.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "draw_shape/qnpc_res_table.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800F2A94.h"
#include "lobby/lb_event_request.h"
#include "lobby/lb_sub16_send.h"
#include "Pl/pl_act.h"
#include "Pl/quest_spawn_rec_find.h"
#include "pad_connect.h"
#include "camera/camera.h"
#include "camera/camera_kill_cut_start_split.h"
#include "get_FqResult_work.h"
#include "lobby/lobby_w.h"
#include "nw_resource.h"
#include "menu/placeinfo_model_create.h"
#include "ef/eft050_interior_fx_spawn.h"
#include "quest/lb_interior_fx_tbl.h"
#include "menu/note_pane_player_near_ck.h"
#include "MSL_C/alloc.h"
#include "ef.h"
#include "fn_8004CAD8.h"
#include "sound/fn_800D7F54.h"
#include "fn_80047398/lobby_world_block.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/em_area_entry_tbl.h"
#include "enemy/EmAreaEntry.h"
#include "enemy/fn_8013F764.h"
#include "nw_resource.h"
#include "ef/work_mem_free.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "Pl/plw.h"
#include "Pl/pl_item_add.h"
#include "Pl/fn_80262940.h"
#include "lobby/lb_server_sel_trans.h"
#include "ef/eft004_scaled_spawn.h"
#include "menu/npc_trade_pick.h"
#include "hud/layout_types.h"
#include "hud/draw_sprite_ary.h"
#include "hud/draw_font_idx.h"
#include "hud/get_lsp_data.h"
#include "hud/cockpit.h"
#include "menu/menu_message.h"
#include "fn_80047398.h"
#include "lobby/lb_npc.h"
#include "menu/put_menu_cursor.h"
#include "menu/get_pop_dat_ptr.h"
#include "lobby/spr_data_copy.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_menu_open.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/LbStr.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_unlock_cond_ck.h"
#include "lobby/lb_item_box_count_draw.h"
#include "ef/eft052_item_get_open.h"
#include "userdata_item.h"
#include "main.h"
#include "sound/set_zmode__FbUcb.h"
#include "sound/fn_800E3CBC.h"

/* The quest tail's own rows other units do not call (address order below). */
extern "C" {
void quest_record_copy(QuestRecord* dst, QuestRecord* src);
void quest_enemy_kill_record(Q_MoveWork* work, _ENEMY_WORK* enemy, u8 mini);
}

#pragma peephole off
#pragma fp_contract off

/* -------------------------------------------------------------------------------------------------
 * The note-pane band's own still-unwritten members: the nine tail-called sub-states
 * `note_pane_state_dispatch` selects, in the pane's state order.
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
void note_pane_state_dispatch(NoteWork* self);
s32 npc_talk_step(NoteWork* self);
void note_pane_state_0(NoteWork* self);
void note_pane_state_1(NoteWork* self);
void note_pane_state_2(NoteWork* self);
void note_pane_state_3(NoteWork* self);
void note_pane_state_4(NoteWork* self);
void note_pane_state_5(NoteWork* self);
void note_pane_state_6(NoteWork* self, u32 variant);
void note_pane_state_9(NoteWork* self);
}

/* -------------------------------------------------------------------------------------------------
 * The note-pane band (0x803A3A50..0x803A52A4).
 * ------------------------------------------------------------------------------------------------- */

/* 0x803A4170 - steps the pane's eased position towards its target (4004 units while its animation pair is (0,4), else
 * 1820), snapping on within one step. */
extern "C" void note_pane_pos_step(NoteWork* self) {
    u32 step = (note_pane_anim_pair_ck(self, 0, 4) != 0) ? 4004 : 1820;
    u32 target = self->field_0x1A8;
    u32 cur = (u16)self->field_0x18C;
    u16 avail = (u16)(target - cur);
    if ((u16)(avail + step) < (u32)(step * 2)) {
        self->field_0x18C = target;
    } else if (avail < 0x8000) {
        self->field_0x18C = (u16)(cur + step);
    } else {
        self->field_0x18C = (u16)(cur - step);
    }
}

/* 0x803A4208 - puts the pane on its animation pair (0,1). */
extern "C" void note_pane_anim_pair_0_1(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 1);
}

/* 0x803A4E30 - the pane's null sub-state. */
extern "C" void note_pane_idle(NoteWork* self) {
}

/* 0x803A4E34 - runs the pane sub-state `+0x19D` selects: 0 dispatches on the pane's state byte,
 * 1 is the null one. */
extern "C" void note_pane_dispatch(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0: note_pane_state_dispatch(self); break;
    case 1: note_pane_idle(self); break;
    }
}

/* 0x803A4DD4 - the note pane's per-state dispatcher: `NoteWork::field_0x19F` selects one of ten
 * tail-called sub-state bodies. */
extern "C" void note_pane_state_dispatch(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0: note_pane_state_0(self); break;
    case 1: note_pane_state_1(self); break;
    case 2: note_pane_state_2(self); break;
    case 3: note_pane_state_3(self); break;
    case 4: note_pane_state_4(self); break;
    case 5: note_pane_state_5(self); break;
    case 6: note_pane_state_6(self, 0); break;
    case 7: note_pane_state_6(self, 1); break;
    case 8: note_pane_state_6(self, 2); break;
    case 9: note_pane_state_9(self); break;
    }
}

/* 0x803A4E58 - reads the quest NPC's motion number through `qn_get_motion_no` (the record is the pane's 0x1F8-byte
 * `NoteWork`; the vector local is retail's `VEC3 v; VEC3_ctor(&v);` idiom). */
extern "C" void note_pane_get_motion(NoteWork* self) {
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    qn_get_motion_no((_QNPC_W*)self);
}

/* 0x803A4E90 - true while the lifetime record still has a live countdown and a set flag. */
extern "C" s32 note_timer_ready_ck(NoteTimer* timer) {
    if ((s8)timer->voyage_0x06 <= 0) {
        if ((s8)timer->gain_0x01 != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803A4F7C - the inverse of `note_slot_to_value`: the flat slot index holding `value` in `note_slot_table`'s four
 * 6-entry tables, then `note_slot_flat_table`; 255 when absent. */
extern "C" u32 note_value_to_slot(u16 value) {
    u32 index = 0;
    s8 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (value == p[j]) {
                return index;
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (value == *q) {
                return index;
            }
            index++;
            q++;
        }
    }
    return 255;
}

/* 0x803A5070 - maps a flat slot index to its table value over the same four 6-entry tables and the
 * flat run at `note_slot_flat_table`; 0 past the end of both. */
extern "C" u16 note_slot_to_value(u8 slot) {
    u32 index = 0;
    u16 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (slot == (u8)index) {
                return p[j];
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (slot == (u8)index) {
                return *q;
            }
            index++;
            q++;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------------------------------
 * The interior-model resources (0x803A75D8..0x803A7E1C): eight 0x14000-byte buffers in `lobby_w`'s resource view,
 * loaded from `lb_interior_path_table` and released by reference count.
 * ------------------------------------------------------------------------------------------------- */

/* The note trade's top menu (send goods / routes / rewards), the voyages each route takes, the dice each offer kind
 * rolls its points with, the six rewards of each route and the route table `note_value_to_slot`/`note_slot_to_value`
 * also search (continued by `note_slot_flat_table`). */
LbListTmpl note_trade_menu_tmpl = {3, 0, 0, 0x1D3, -1, NULL, 5, 3};
s8 note_voyage_tbl[4] = {3, 3, 4, 5};
s8 note_dice_0[8] = {30, 20, 50, -5, 20, -10, 0, 0};
s8 note_dice_1[8] = {25, 30, 45, -5, 30, -15, 0, 0};
s8 note_dice_2[8] = {25, 10, 30, 5, 45, -5, 0, 0};
s8 note_dice_3[8] = {25, 20, 30, 10, 45, -10, 0, 0};
s8 note_dice_4[8] = {100, 0, 100, 0, 100, 0, 0, 0};
s8 note_dice_5[8] = {20, 5, 60, 0, 20, -5, 0, 0};
u16 note_slot_flat_table[4] = {0x219, 0x25C, 0x25D, 0};
s8* note_kind_dice_tbl[6] = {note_dice_0, note_dice_1, note_dice_2, note_dice_3, note_dice_4, note_dice_5};
u16 note_reward_items_1[6] = {0x201, 0x202, 0x203, 0x204, 0x205, 0x206};
u16 note_reward_items_2[6] = {0x207, 0x208, 0x209, 0x20A, 0x20B, 0x20C};
u16 note_reward_items_3[6] = {0x20D, 0x20E, 0x20F, 0x210, 0x211, 0x212};
u16 note_reward_items_4[6] = {0x213, 0x214, 0x215, 0x216, 0x217, 0x218};
u16* note_slot_table[4] = {note_reward_items_1, note_reward_items_2, note_reward_items_3, note_reward_items_4};

/* The 27 interior model files, by interior id. */
char* lb_interior_path_table[27] = {
    "15/machi/interia/interia-00/interia-00.brres", "15/machi/interia/interia-01/interia-01.brres",
    "15/machi/interia/interia-02/interia-02.brres", "15/machi/interia/interia-03/interia-03.brres",
    "15/machi/interia/interia-04/interia-04.brres", "15/machi/interia/interia-05/interia-05.brres",
    "15/machi/interia/interia-06/interia-06.brres", "15/machi/interia/interia-07/interia-07.brres",
    "15/machi/interia/interia-08/interia-08.brres", "15/machi/interia/interia-09/interia-09.brres",
    "15/machi/interia/interia-10/interia-10.brres", "15/machi/interia/interia-11/interia-11.brres",
    "15/machi/interia/interia-12/interia-12.brres", "15/machi/interia/interia-13/interia-13.brres",
    "15/machi/interia/interia-14/interia-14.brres", "15/machi/interia/interia-15/interia-15.brres",
    "15/machi/interia/interia-16/interia-16.brres", "15/machi/interia/interia-17/interia-17.brres",
    "15/machi/interia/interia-18/interia-18.brres", "15/machi/interia/interia-19/interia-19.brres",
    "15/machi/interia/interia-20/interia-20.brres", "15/machi/interia/interia-21/interia-21.brres",
    "15/machi/interia/interia-22/interia-22.brres", "15/machi/interia/interia-23/interia-23.brres",
    "15/machi/interia/interia-24/interia-24.brres", "15/machi/interia/interia-25/interia-25.brres",
    "15/machi/interia/interia-26/interia-26.brres",
};

/* The interior id of each furniture kind, and each interior's shadow scale. */
u16 lb_interior_kind_table[28] = {
    3, 4, 5, 0, 1, 2, 10, 9, 11, 6, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 20, 23, 21, 22, 24, 25, 26, 0,
};

f32 lb_interior_scale_table[27] = {
    80.0f, 90.0f, 85.0f, 70.0f, 100.0f, 180.0f, 100.0f, 120.0f, 90.0f, 150.0f, 200.0f, 190.0f, 180.0f, 150.0f,
    150.0f, 160.0f, 150.0f, 150.0f, 100.0f, 210.0f, 210.0f, 100.0f, 100.0f, 100.0f, 80.0f, 120.0f, 150.0f,
};

/* 0x803A75D8 (0x140): clears the eight resource slots and splits one 0xA0000-byte resource-memory block into their
 * buffers. */
extern "C" void lobby_res_slots_init(void) {
    LbLobbyWork* w = &lobby_w;
    s32 i;
    LbResSlot* slot;
    u8* base;

    for (i = 0, slot = w->res_slots_0x0CC; i < 8; slot++, i++) {
        memset(slot, 0, sizeof(LbResSlot));
    }
    w->res_handle_0x0C4 = pull_res_mem(NULL, 0xA0000, 1);
    base = (u8*)getResMemAdrs(lobby_w.res_handle_0x0C4);
    w->res_base_0x0C8 = base;
    w->res_slots_0x0CC[0].id_0x2 = 0xFF;
    w->res_slots_0x0CC[0].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[0].buf_0x4 = base;
    w->res_slots_0x0CC[0].size_0x8 = 0x14000;
    w->res_slots_0x0CC[1].id_0x2 = 0xFF;
    w->res_slots_0x0CC[1].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[1].buf_0x4 = base + 0x14000;
    w->res_slots_0x0CC[1].size_0x8 = 0x14000;
    w->res_slots_0x0CC[2].id_0x2 = 0xFF;
    w->res_slots_0x0CC[2].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[2].buf_0x4 = base + 0x28000;
    w->res_slots_0x0CC[2].size_0x8 = 0x14000;
    w->res_slots_0x0CC[3].id_0x2 = 0xFF;
    w->res_slots_0x0CC[3].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[3].buf_0x4 = base + 0x3C000;
    w->res_slots_0x0CC[3].size_0x8 = 0x14000;
    w->res_slots_0x0CC[4].id_0x2 = 0xFF;
    w->res_slots_0x0CC[4].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[4].buf_0x4 = base + 0x50000;
    w->res_slots_0x0CC[4].size_0x8 = 0x14000;
    w->res_slots_0x0CC[5].id_0x2 = 0xFF;
    w->res_slots_0x0CC[5].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[5].buf_0x4 = base + 0x64000;
    w->res_slots_0x0CC[5].size_0x8 = 0x14000;
    w->res_slots_0x0CC[6].id_0x2 = 0xFF;
    w->res_slots_0x0CC[6].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[6].buf_0x4 = base + 0x78000;
    w->res_slots_0x0CC[6].size_0x8 = 0x14000;
    w->res_slots_0x0CC[7].id_0x2 = 0xFF;
    w->res_slots_0x0CC[7].sub_0x1 = 0xFF;
    w->res_slots_0x0CC[7].buf_0x4 = base + 0x8C000;
    w->res_slots_0x0CC[7].size_0x8 = 0x14000;
}

/* 0x803A7718 (0x90): the first free resource slot, NULL when all eight are in use. */
extern "C" LbResSlot* lb_interior_slot_free_get(void) {
    LbResSlot* slot = lobby_w.res_slots_0x0CC;
    s32 i;

    for (i = 0; i < 8; i++, slot++) {
        if (slot->used_0x0 == 0) {
            return slot;
        }
    }
    return NULL;
}

/* 0x803A77A8 (0x134): the slot holding interior `id`, NULL when none does. */
extern "C" LbResSlot* lb_interior_slot_find(u8 id) {
    LbResSlot* slot = lobby_w.res_slots_0x0CC;
    s32 i;

    for (i = 0; i < 8; i++, slot++) {
        if (slot->used_0x0 != 0 && slot->id_0x2 == id) {
            return slot;
        }
    }
    return NULL;
}

/* 0x803A78DC (0xE4): the load completion: registers the read file under its path and builds its model (`mode` 1)
 * or every model its mask names (`mode` 2). */
extern "C" void lb_interior_load_done(u32 data, s32 size, s32 flag, u32* ctx) {
    LbResSlot* slot = lb_interior_slot_find(ctx[2]);
    s32 i;

    if (slot != NULL) {
        nwAddResource(lb_interior_path_table[ctx[2]], slot->buf_0x4);
        switch (ctx[0]) {
        case 1:
            placeinfo_model_create(ctx[1], lb_interior_path_table[ctx[2]]);
            break;
        case 2:
            for (i = 0; i < 8; i++) {
                if ((ctx[1] & (1 << i)) != 0) {
                    placeinfo_model_create(i, lb_interior_path_table[ctx[2]]);
                }
            }
            break;
        }
    }
}

/* 0x803A79C0 (0x318): loads interior `id` for model `sub` (`mode` 1) or for every model of the mask `sub` (`mode`
 * 2): an already-loaded interior gains a user and builds its model(s) at once; otherwise a free slot takes the file
 * and the models are built when it arrives.  Returns the slot, NULL when none is free. */
extern "C" LbResSlot* lb_interior_load(u8 mode, u8 sub, u8 id) {
    LbResSlot* slot;
    u32 ctx[3];
    s32 i;

    if ((s32)mode != 2) {
        slot = lb_interior_slot_find(id);
        if (slot != NULL) {
            slot->refs_0x3++;
            placeinfo_model_create(sub, lb_interior_path_table[id]);
            return slot;
        }
        slot = lb_interior_slot_free_get();
        if (slot == NULL) {
            return NULL;
        }
        slot->used_0x0 = 1;
        slot->sub_0x1 = sub;
        slot->id_0x2 = id;
        slot->refs_0x3 = 1;
        ctx[0] = mode;
        ctx[1] = sub;
        ctx[2] = id;
        load_file_req(lb_interior_path_table[id], (u32)slot->buf_0x4, slot->size_0x8, (u32)lb_interior_load_done, 3,
                      ctx);
    } else {
        slot = lb_interior_slot_find(id);
        if (slot != NULL) {
            for (i = 0; i < 8; i++) {
                if ((sub & (1 << i)) != 0) {
                    slot->refs_0x3++;
                    placeinfo_model_create(i, lb_interior_path_table[id]);
                }
            }
            return slot;
        }
        slot = lb_interior_slot_free_get();
        if (slot == NULL) {
            return NULL;
        }
        slot->used_0x0 = 1;
        for (i = 0; i < 8; i++) {
            if ((sub & (1 << i)) != 0) {
                slot->sub_0x1 = i;
                break;
            }
        }
        slot->id_0x2 = id;
        slot->refs_0x3 = 0;
        for (i = 0; i < 8; i++) {
            if ((sub & (1 << i)) != 0) {
                slot->refs_0x3++;
            }
        }
        ctx[0] = mode;
        ctx[1] = sub;
        ctx[2] = id;
        load_file_req(lb_interior_path_table[id], (u32)slot->buf_0x4, slot->size_0x8, (u32)lb_interior_load_done, 3,
                      ctx);
    }
    return slot;
}

/* 0x803A7CD8 (0x58): drops one user of interior `id`, freeing its slot at zero. */
extern "C" void lb_interior_release(u8 id) {
    LbResSlot* slot = lb_interior_slot_find(id);
    if (slot != NULL) {
        slot->refs_0x3--;
        if (slot->refs_0x3 == 0) {
            slot->used_0x0 = 0;
            slot->sub_0x1 = 0xFF;
            slot->id_0x2 = 0xFF;
        }
    }
}

/* 0x803A7D30 (0x18): the model file path of interior `id`. */
extern "C" char* lb_interior_path_get(u8 id) {
    return lb_interior_path_table[id];
}

/* 0x803A7D48 (0x1C): the interior id of furniture kind `kind`. */
extern "C" u8 lb_interior_kind_get(u8 kind) {
    return lb_interior_kind_table[kind];
}

/* 0x803A7D64 (0x38): the shadow scale of furniture kind `kind` (0 for 0xFF). */
extern "C" f32 lb_interior_scale_get(u8 kind) {
    if (kind == 0xFF) {
        return 0.0f;
    }
    return lb_interior_scale_table[lb_interior_kind_table[kind]];
}

/* 0x803A7D9C (0x80): spawns the effects interior `id` places for model `key`. */
extern "C" void lb_interior_fx_spawn(u8 id, u8 key) {
    LbInteriorFx* fx = lb_interior_fx_tbl[id];

    if (fx == NULL) {
        return;
    }
    for (; fx->key_0x00 != 0xFF; fx++) {
        if (fx->key_0x00 == key) {
            eft050_interior_fx_spawn(key, &fx->pos_0x04, fx->scale_0x10, 0);
        }
    }
}

/* -------------------------------------------------------------------------------------------------
 * The quest tail (0x803A7E1C..0x803AA4A4): the item-work accessors, the quest clock and random source,
 * the element flags and the kill/capture bookkeeping.
 * ------------------------------------------------------------------------------------------------- */

/* 0x803A7E1C (0x4C): the arena item table's entry count (0 in the entry state or without an item work). */
extern "C" u32 quest_arena_item_count_get(void) {
    Q_ItemWork* item;

    if (move_work_state_ck() != 0) {
        return 0;
    }
    item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->count_0x6A2A;
}

/* 0x803A7E68 (0x210): files the monster slots of `count` area lists into spawn list `index` of the move work
 * (continuing the previous list's spawn order), spawns each, and keeps the area's main monster in `main_0x2258`. */
extern "C" void quest_area_spawn_setup(Q_MoveWork* work, u32* area, u8 count, u8 index) {
    EmGroundRec ground;
    QuestSpawnRec* main;
    u32* num;
    QuestSpawnRec* rec;
    s32 i;
    QuestEntrySlot* slot;
    u32 order;
    QuestBossSpawn* boss;
    s32 element;

    em_ground_rec_clear(&ground);
    if (work->main_0x2258 == NULL) {
        work->main_0x2258 = (QuestSpawnRec*)work_mem_alloc(16);
        memset(work->main_0x2258, 0, 16);
    }
    main = work->main_0x2258;
    if (index == 0) {
        num = &work->area_counts_0x2154[index];
        *num = 0;
    } else {
        num = &work->area_counts_0x2154[index];
        *num = work->area_counts_0x2154[index - 1];
    }
    rec = work->area_recs_0x154[index];
    for (i = 0; i < 6; i++) {
        work->res_a_0x2194[index][i] = -1;
        work->res_b_0x21F4[index][i] = -1;
    }
    for (i = 0; i < count; i++) {
        slot = (QuestEntrySlot*)*area;
        if (slot != NULL) {
            for (; slot->monster_0x00 != 0; slot++) {
                if (slot->element_index_0x28 == -1) {
                    order = *num + 32;
                    em_ground_rec_set(&ground, &slot->element_0x08);
                    boss = em_large_spawn(slot->monster_0x00, &ground, order);
                    if (boss == NULL) {
                        return;
                    }
                    memset(rec, 0, 16);
                    rec->order_0x0 = order;
                    rec->slot_0x8 = slot;
                    rec->count_0x4 = slot->count_0x04;
                    rec->monster_0x6 = slot->monster_0x00;
                    rec->boss_0xC = boss;
                    if (slot->element_0x08.byte_0x02 == 0) {
                        main->order_0x0 = order;
                        main->slot_0x8 = slot;
                        main->count_0x4 = slot->count_0x04;
                        main->monster_0x6 = slot->monster_0x00;
                        main->boss_0xC = boss;
                    }
                    element = quest_element_find(slot->monster_0x00);
                    if (element >= 0) {
                        em_quest_element_set_large((_ENEMY_WORK*)boss, element);
                    }
                    rec++;
                    (*num)++;
                }
            }
        }
        area++;
    }
}

/* 0x803A8078 (0xB0): respawns every large monster of the first spawn list whose count has run out and collects
 * the new spawns into `out`; returns how many it collected. */
extern "C" s32 quest_area_entry_collect(Q_MoveWork* work, QuestBossSpawn** out) {
    EmGroundRec ground;
    s32 count;
    QuestSpawnRec* rec;
    s32 n;
    s32 i;
    QuestBossSpawn* boss;

    em_ground_rec_clear(&ground);
    count = work->area_counts_0x2154[0];
    rec = work->area_recs_0x154[0];
    n = 0;
    for (i = 0; i < count; i++, rec++) {
        if (rec->count_0x4 == 0) {
            em_ground_rec_set(&ground, &rec->slot_0x8->element_0x08);
            boss = em_large_spawn(rec->slot_0x8->monster_0x00, &ground, i + 32);
            if (boss != NULL) {
                *out = boss;
                out++;
                n++;
            }
        }
    }
    return n;
}

/* 0x803A8128 (0x5C4): reworks the move work's six resident monster kinds for area list `index`/area `sub`: keeps
 * the kinds the live area entries near the area and the live small monsters still need, releases the others (their
 * resources, control slot and SE slot), loads the new ones, and respawns the large monsters whose count ran out. */
extern "C" void quest_area_spawn_apply(Q_MoveWork* work, u8 index, u8 sub) {
    s8 keep[6];
    s32 need[6];
    s32 add[6];
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    s32 i;
    s32 j;
    s32 n;
    QuestBossSpawn** list;
    QuestSpawnRec* rec;
    QuestEntrySlot* slot;
    _ENEMY_WORK* enemy;
    u16 max;
    u8 slot_index;

    for (i = 0; i < 6; i++) {
        add[i] = 0;
        need[i] = 0;
        work->kinds_prev_0x2170[i] = work->kinds_0x2164[i];
        if (work->kinds_prev_0x2170[i] != 0) {
            keep[i] = 0;
        } else {
            keep[i] = -1;
        }
    }
    n = 0;
    list = (QuestBossSpawn**)work_mem_alloc(0x200);
    if (list != NULL) {
        n = quest_area_entry_collect(work, list);
    }
    for (i = 0; i < 128; entry++, i++) {
        if (entry->active == 0 || entry->order_0x16 < 32) {
            continue;
        }
        rec = quest_spawn_rec_find(entry->order_0x16);
        if (rec == NULL) {
            continue;
        }
        slot = rec->slot_0x8;
        if (slot->element_0x08.byte_0x02 == 0) {
            if (work->main_0x2258 == NULL) {
                work->main_0x2258 = (QuestSpawnRec*)work_mem_alloc(16);
                memset(work->main_0x2258, 0, 16);
            }
        } else if (em_area_entry_near_ck(entry, index, sub) == 0) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (slot->monster_0x00 == need[j]) {
                break;
            }
        }
        if (j != 6) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (need[j] == 0) {
                break;
            }
        }
        if (j >= 6) {
            break;
        }
        need[j] = slot->monster_0x00;
    }
    enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (max = get_move_work_max(3); max > 0; max--, enemy++) {
        if (enemy->active == 0 || (enemy->field_0x1C8 & 1) != 0 || enemy->field_0x00C != 0) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (enemy->team == need[j]) {
                break;
            }
        }
        if (j != 6) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (need[j] == 0) {
                break;
            }
        }
        if (j >= 6) {
            break;
        }
        need[j] = enemy->team;
    }
    for (i = 0; i < 6; i++) {
        if (need[i] != 0) {
            for (j = 0; j < 6; j++) {
                if (work->kinds_prev_0x2170[j] == need[i]) {
                    keep[j] = 1;
                    break;
                }
            }
            if (j >= 6) {
                add[i] = need[i];
            }
        }
    }
    for (i = 0; i < 6; i++) {
        if (keep[i] == 0) {
            if (work->res_a_0x2194[0][i] != -1) {
                nw_res_entry_clear(work->res_a_0x2194[0][i]);
                work->res_a_0x2194[0][i] = -1;
            }
            if (work->res_b_0x21F4[0][i] != -1) {
                nw_res_entry_clear(work->res_b_0x21F4[0][i]);
                work->res_b_0x21F4[0][i] = -1;
            }
            slot_index = em_kind_slot_find((u8)work->kinds_0x2164[i]);
            if (slot_index != 0xFF) {
                em_kind_slot_release(slot_index);
            }
            snd_em_se_slot_release(work->kinds_0x2164[i]);
            work->kinds_0x2164[i] = 0;
        }
    }
    for (i = 0; i < 6; i++) {
        if (add[i] != 0) {
            for (j = 0; j < 6; j++) {
                if (work->kinds_0x2164[j] == 0) {
                    break;
                }
            }
            if (j < 6) {
                work->kinds_0x2164[j] = add[i];
                em_kind_release(work->kinds_0x2164[j]);
            }
        }
    }
    if (list != NULL) {
        for (i = 0; i < n; i++) {
            em_area_entry_release((EmAreaEntry*)list[i]);
        }
        work_mem_free(list);
    }
}

/* 0x803A86EC (0x4): releases the enemy area entries of the current area. */
extern "C" void quest_screen_enemy_start(Q_MoveWork* work) {
    em_area_entries_release();
}

/* 0x803A86F0 (0xB8): when the quest NPC of (`kind`, `sub`) needs its models, reads every `qnpc_res_table` file
 * back to back into a free enemy resource buffer and loads the NPC's voice banks. */
extern "C" void quest_area_res_load(u8 map, u8 area) {
    s32 i = 0;
    u8* buf;
    QnpcResEntry* entry;
    u32 ctx;

    if (qnpc_load_ck(map, area) != 0 && (buf = em_res_buffer_get()) != NULL) {
        for (entry = qnpc_res_table; entry->size != 0; entry++, i++) {
            ctx = (u8)i;
            load_file_req(entry->path, (u32)buf, entry->size, (u32)qnpc_res_load_done, 1, &ctx);
            buf += entry->size;
        }
        snd_npc_voice_bank_load(map, area);
    }
}

/* 0x803A87A8 (0x38): the item work slot 0's move work points at, NULL without a move work. */
extern "C" Q_ItemWork* move_work_item_work_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return NULL;
    }
    return work->item_work;
}

/* 0x803A87E0 (0x3C): the time left (+0x24), clamped up to 0. */
extern "C" s32 quest_time_elapsed_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_limit_0x24 < 0 ? 0 : item->time_limit_0x24;
}

/* 0x803A881C (0x3C): the time the screen counts down from (+0x20), clamped up to 0. */
extern "C" s32 quest_time_base_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_base_0x20 < 0 ? 0 : item->time_base_0x20;
}

/* 0x803A8858 (0x34): the quest's whole time limit (+0x1C). */
extern "C" s32 quest_time_limit_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_total_0x1C;
}

/* 0x803A888C (0x108): accepts quest `id`: copies its record into the item work's row and takes the quest id, the
 * slot byte, the time limit (minutes, scaled to frames) and the row's three words from it. */
extern "C" void quest_id_set(s32 id) {
    Q_ItemWork* item = move_work_item_work_get();
    QuestRecord* rec;

    if (item != NULL && item->record_0x3C != NULL) {
        rec = quest_record_find((u16)id);
        quest_record_copy((QuestRecord*)item->record_0x3C, rec);
        item->quest_id_0x10 = rec->field_0x02C;
        item->field_0x14 = rec->slot_bytes_0x314[0].value;
        item->time_total_0x1C = rec->field_0x13A * (60.0f * Screen_w.frame_scale);
        item->time_limit_0x24 = rec->field_0x13A * (60.0f * Screen_w.frame_scale);
        item->rec_value_0x30 = rec->field_0x34C;
        item->rec_value_0x34 = rec->field_0x358;
        item->rec_flags_0x38 = rec->flags_0x310;
    }
}

/* 0x803A8994 (0x3B8): copies a whole quest result record. */
extern "C" void quest_record_copy(QuestRecord* dst, QuestRecord* src) {
    *dst = *src;
}

/* 0x803A8D4C (0x58): the current quest id: the lobby's selected id while the slot is in its entry state, else
 * the item work's own. */
extern "C" u32 quest_id_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    if (move_work_state_ck() != 0) {
        return lb_param_w.field_0x00;
    }
    return item->quest_id_0x10;
}

/* 0x803A8DA4 (0x140): seeds the random source with the sum of the clock snapshot's seven (low, high << 8)
 * pairs, 451 when that sum is 0. */
extern "C" void quest_rand_seed_set(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->rand_state_0x5A = 0;
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[0] + (u16)(item->rand_words_0x6C[1] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[2] + (u16)(item->rand_words_0x6C[3] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[4] + (u16)(item->rand_words_0x6C[5] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[6] + (u16)(item->rand_words_0x6C[7] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[8] + (u16)(item->rand_words_0x6C[9] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[10] + (u16)(item->rand_words_0x6C[11] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[12] + (u16)(item->rand_words_0x6C[13] << 8));
        if (item->rand_state_0x5A == 0) {
            item->rand_state_0x5A = 451;
        }
    }
}

/* 0x803A8EE4 (0x7C): steps the random source (x176 mod 65363, a 0 state taken as 1) and returns it. */
extern "C" u32 quest_rand_next(void) {
    Q_ItemWork* item = move_work_item_work_get();
    u32 state = item->rand_state_0x5A;
    if (item == NULL) {
        return 0;
    }
    if (state == 0) {
        state = 1;
    }
    return item->rand_state_0x5A = (state * 176) % 65363;
}

/* 0x803A8F60 (0x50): the low byte of clock snapshot word `index`. */
extern "C" u8 quest_clock_byte_get(u8 index) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->rand_words_0x6C[index];
}

/* 0x803A8FB0 (0x180): counts the time left down by the real time since the last step (at least one frame, none
 * with `mode` 0) once less than 99 minutes are left; with no previous reading `mode` 1 takes one frame. */
extern "C" void quest_clock_step(s32 mode) {
    Q_ItemWork* item = move_work_item_work_get();
    f64 frames;

    if (item == NULL) {
        return;
    }
    if (item->time_limit_0x24 >= 99.0f * (60.0f * Screen_w.frame_scale)) {
        return;
    }
    item->timer_0x6A50 = item->timer_0x6A48;
    item->timer_0x6A48 = OSGetTime();
    if (item->timer_0x6A50 > 0.0) {
        frames = 8000.0 * (item->timer_0x6A48 - item->timer_0x6A50) / (OS_BUS_CLOCK / 4 / 125000) / 1000.0 /
                 item->timer_rate_0x6A58;
        if (frames < 1.0) {
            frames = 1.0;
        }
        if (mode == 0) {
            frames = 0.0;
        }
        item->time_left_0x6A60 -= frames;
    } else if (mode == 1) {
        item->time_left_0x6A60 -= 1.0;
    }
    if (item->time_limit_0x24 > 0) {
        item->time_limit_0x24 = item->time_left_0x6A60;
        if (item->time_limit_0x24 < 0) {
            item->time_limit_0x24 = 0;
        }
    }
}

/* 0x803A9130 (0x5C): sets the time left (in frames) and its floating copy. */
extern "C" void quest_time_limit_set(s32 frames) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->time_limit_0x24 = frames;
        item->time_left_0x6A60 = item->time_limit_0x24;
    }
}

/* 0x803A918C (0x48): restarts the real-time clock: both readings take the current time. */
extern "C" void quest_clock_reset(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->timer_0x6A48 = OSGetTime();
        item->timer_0x6A50 = item->timer_0x6A48;
    }
}

/* 0x803A91D4 (0x6C): books a kill: the entry-state slot's record or the quest's. */
extern "C" void quest_enemy_kill_dispatch(_ENEMY_WORK* enemy, u8 from_net) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        if (work->state_0x113 == 1) {
            em_set_kill_record(enemy, from_net);
        } else {
            quest_enemy_kill_record(work, enemy, from_net);
        }
    }
}

/* 0x803A9240 (0x84): counts one more arena item `value` of element `id` (values 0..7) and lets the arena gate
 * re-check it; nothing in the entry state. */
extern "C" void quest_arena_need_add(u8 id, u32 unused, u8 value) {
    QuestWork* work;

    if (move_work_state_ck() != 1 && (work = (QuestWork*)move_work_item_work_get()) != NULL && value <= 7) {
        work->part_counts_0x6803[id][value]++;
        quest_arena_value_clear_ck(id, value);
    }
}

/* 0x803A92C4 (0x78): sets key bit `bit` of arena element `id` and lets the arena gate re-check it. */
extern "C" void quest_arena_key_set(u8 id, u8 bit) {
    QuestWork* work;

    if (move_work_state_ck() != 1 && (work = (QuestWork*)move_work_item_work_get()) != NULL) {
        work->key_bits_0x694C[id] |= 1 << bit;
        quest_arena_key_clear(id);
    }
}

/* 0x803A933C (0xE0): monster `id`'s first parameter: the accepted row's for an id of 32 or more, else the item
 * work's row entry for the id (0 when the id has no row). */
extern "C" u8 quest_em_stat_tbl_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        if (id >= 32) {
            return item->record_0x3C->em_stat_0x380;
        }
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[0][i];
        }
    }
    return 0;
}

/* 0x803A941C (0xDC): monster `id`'s second parameter row entry (0 for an id of 32 or more or without a row). */
extern "C" u8 quest_em_stat_var_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        if (id >= 32) {
            return 0;
        }
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[1][i];
        }
    }
    return 0;
}

/* 0x803A94F8 (0xCC): monster `id`'s third parameter row entry (0 without a row). */
extern "C" u8 quest_em_size_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[2][i];
        }
    }
    return 0;
}

/* 0x803A95C4 (0xCC): monster `id`'s fourth parameter row entry (0 without a row). */
extern "C" u8 quest_em_size_var_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[3][i];
        }
    }
    return 0;
}

/* 0x803A9690 (0x34): the item work's state word. */
extern "C" s32 quest_work_state_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->state_0x6AA0;
}

/* Runs the kill cut-in on `enemy` (the split-screen form while the screen is split, else only inside its area). */
static inline void quest_kill_cut_start(_ENEMY_WORK* enemy) {
    if (screen_split_mode_ck() != 0) {
        camera_kill_cut_start_split(1, enemy);
    } else if (em_area_ck(enemy) == 1) {
        camera_kill_cut_start(1, enemy);
    }
}

/* 0x803A96C4 (0x4B8): books one slain or captured monster (a small-monster record when `from_net` is set): the
 * per-kind counts and sizes, its spawn entry's count, the tracked-kind kills or the quest element's progress, and
 * the kill cut-in unless that finished the quest. */
extern "C" void quest_enemy_kill_record(Q_MoveWork* work, _ENEMY_WORK* enemy, u8 from_net) {
    _ENEMY_WORK* target = NULL;
    Q_ItemWork* item = move_work_item_work_get();
    s32 done;
    s32 size;
    u32 big;
    u16 key;
    u8 element;
    u8 kind;
    u8 how;
    u8 area;
    QuestSpawnRec* entry;
    s32 i;
    s32 need;
    s8 first;

    if (item == NULL) {
        return;
    }
    done = 0;
    size = 0;
    big = 0;
    if (from_net == 0) {
        target = enemy;
        key = enemy->field_0x01A;
        element = enemy->field_0x00D;
        kind = enemy->team;
        how = em_captured_ck(enemy);
        em_record_hit_ck(enemy);
        if ((enemy->field_0x1C8 & 1) != 0) {
            big = 1;
            size = 100.0f * get_em_chg_scale(enemy) + 0.5f;
        }
        area = enemy->area_slot_0x013;
    } else {
        _ENEMY_MINI_WORK* mini = (_ENEMY_MINI_WORK*)enemy;
        key = mini->order_0x16;
        element = mini->element_0x0D;
        kind = mini->monster_0x02;
        how = em_mini_kill_kind_get(enemy);
        em_mini_hit_ck(enemy);
        area = mini->area_slot_0x1D;
    }
    if (how == 0) {
        item->set_c.count[kind]++;
        quest_element_item_apply(1, kind, 1);
    }
    if (how == 1) {
        item->set_d.count[kind]++;
    }
    if (big == 1) {
        if (item->size_0x534[kind].min == 0) {
            item->size_0x534[kind].min = size;
        } else if (item->size_0x534[kind].min > (u16)size) {
            item->size_0x534[kind].min = size;
        }
        if (item->size_0x534[kind].max == 0) {
            item->size_0x534[kind].max = size;
        } else if (item->size_0x534[kind].max < (u16)size) {
            item->size_0x534[kind].max = size;
        }
    }
    entry = quest_spawn_rec_find_in(key, area);
    if (entry != NULL && entry->count_0x4 > 0) {
        entry->count_0x4--;
    }
    if (element > 2) {
        if (big == 1 && Pl_motion_input_ck(0) == 0) {
            for (i = 0; i < 3; i++) {
                if (kind == work->spawn_0x2274[i + 3].monster_0x6) {
                    ((QuestWork*)item)->slot_kills_0x6980[i]++;
                    break;
                }
            }
        }
        return;
    }
    if (quest_flag_4000000_ck(NULL) == 1) {
        item->hunt_end_0x6977 = 0;
        element = (s8)(2 - element);
    }
    if (quest_flag_80000000_ck(NULL) == 1) {
        item->hunt_end_0x6977 = 0;
    }
    if (quest_element_progress_step(kind, how, &first) != 1) {
        return;
    }
    if (quest_flag_8_ck(NULL) == 1) {
        return;
    }
    if (quest_flag_80000_ck(NULL) == 0) {
        if (quest_flag_4000000_ck(NULL) == 1) {
            if ((u8)(element - 1) <= 1) {
                done = 1;
            }
        } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
            if ((item->elements_0x94[0].flags & 9) == 9 && (item->elements_0x94[1].flags & 9) == 9) {
                if ((s8)element == 0) {
                    if (quest_element_pick_ck((QuestWork*)item, 1, 0) == 0) {
                        done = 1;
                    }
                } else if ((s8)element == 1) {
                    if (quest_element_pick_ck((QuestWork*)item, 0, 0) == 0) {
                        done = 1;
                    }
                }
            } else if ((item->elements_0x94[0].flags & 9) == 9 && item->elements_0x94[0].value != 0) {
                done = 1;
            }
        } else if (quest_flag_40000_ck(NULL) == 1) {
            for (i = 0; i < 3; i++) {
                if (quest_element_live_ck((QuestWork*)item, i) != 0 &&
                    quest_element_pick_ck((QuestWork*)item, i, 0) == 0) {
                    done = 1;
                    break;
                }
            }
        } else if ((s8)element != 0 || first != 0) {
            done = 1;
        }
        if (target != NULL && done == 0) {
            quest_kill_cut_start(target);
        }
    } else {
        done = 0;
        for (i = 0; i < 3; i++) {
            quest_arena_count_get(i);
            need = quest_arena_need_get(i);
            if (need >= 0) {
                done += need;
            }
        }
        if (done == 0) {
            if (target != NULL) {
                quest_kill_cut_start(target);
            }
        } else {
            snd_hunt_stream_start(0);
        }
    }
}

/* 0x803A9B7C (0x270): applies a kill another player reported: takes `count` off the spawn entry under (`key`,
 * `area`), books the taken ones as kills (`how` 0) or captures (1) of its kind, and steps the quest element. */
extern "C" void quest_net_kill_apply(u8 area, u16 key, u16 count, s32 how, u8 element) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;
    s32 taken;
    u8 kind;
    QuestSpawnRec* entry;
    s32 i;
    s8 first;

    if (work != NULL && work->state_0x113 != 1 && (item = move_work_item_work_get()) != NULL) {
        taken = 0;
        kind = 0;
        entry = quest_spawn_rec_find_in(key, area);
        if (entry != NULL) {
            kind = entry->monster_0x6;
            if (entry->count_0x4 >= count) {
                taken = entry->count_0x4 - count + 1;
                entry->count_0x4 -= (s16)taken;
            }
        }
        if (kind != 0 && taken != 0) {
            if ((u8)how == 0) {
                item->set_c.count[kind] += (u16)taken;
                quest_element_item_apply(1, kind, taken);
                userdata_hunt_count_add(kind, (u16)taken);
            }
            if ((u8)how == 1) {
                item->set_d.count[kind] += (u16)taken;
                userdata_capture_count_add(kind, (u16)taken);
            }
            if (element <= 3 && element != 3) {
                if (quest_flag_4000000_ck(NULL) == 1) {
                    item->hunt_end_0x6977 = 0;
                    element = 2 - element;
                }
                if (quest_flag_80000000_ck(NULL) == 1) {
                    item->hunt_end_0x6977 = 0;
                }
                if (quest_element_progress_step(kind, how, &first) == 1 && quest_flag_8_ck(NULL) != 1 &&
                    quest_flag_80000_ck(NULL) == 0) {
                    if (quest_flag_4000000_ck(NULL) == 1) {
                    } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
                        if (element == 0) {
                            quest_element_pick_ck((QuestWork*)item, 1, 0);
                        } else if (element == 1) {
                            quest_element_pick_ck((QuestWork*)item, 0, 0);
                        }
                    } else if (quest_flag_40000_ck(NULL) == 1) {
                        for (i = 0; i < 3; i++) {
                            if (quest_element_live_ck((QuestWork*)item, i) != 0 &&
                                quest_element_pick_ck((QuestWork*)item, i, 0) == 0) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* 0x803A9DEC (0x7C): finishes element `index` with flag `value` unless the slot's sub-state has ended. */
extern "C" void quest_element_set(QuestWork* work, u16 index, u32 value) {
    QuestElement* element = &work->elements_0x0094[index];
    if (quest_sub_state_end_ck(1) == 0) {
        quest_element_finish(work, element, index, value);
    }
}

/* 0x803A9E68 (0xC0): marks element `index` finished (flag 0x20) unless it already counts as picked, announces
 * the first element's special clear, and records it locally or sends it to the server. */
extern "C" void quest_element_finish(QuestWork* work, QuestElement* element, u16 index, u32 value) {
    if (quest_element_pick_ck(work, index, 0) != 1) {
        element->flags |= 0x20;
        if (index == 0 && quest_flag_1000000_ck(NULL) == 1) {
            lb_event_request(13);
        }
        if (isServerSelectState() == 0) {
            quest_element_done_mark(work, element, index, value);
        } else {
            lb_sub16_send(32, index);
        }
    }
}

/* 0x803A9F28 (0x138): files `index` in the first free finish-order slot, marks its flag word and, for an element
 * past the first that the quest's rules do not rule out, announces it and copies its reward cell. */
extern "C" void quest_element_done_mark(QuestWork* work, QuestElement* element, u16 index, u32 flag) {
    s32 i;
    u8 result;

    for (i = 0; i < 3; i++) {
        if (work->slot_ids_0x309[i] == 0xFF) {
            work->slot_ids_0x309[i] = index;
            work->counters_0x2D8[index] |= 0x20;
            if (index != 0 && (element->flags & 0x10) == 0 && quest_flag_4000000_ck(NULL) == 0) {
                result = quest_objective_result_get(NULL);
                if (result != 7 && result != 9 && quest_flag_80000_ck(NULL) == 0 && quest_flag_2000000_ck(NULL) == 0) {
                    hud_msg_push(0, quest_str_tbl_35_get(index + 23));
                    snd_hunt_stream_start(0);
                    hud_msg_push(1, quest_str_tbl_35_get(index + 33));
                    quest_item_pair_copy_cell(work->supply_0x5E2, quest_field372_get(NULL), index - 1);
                }
            }
            break;
        }
    }
}

/* 0x803AA060 (0x158): whether element `index` counts as picked: active (bit 3), not failed (bit 6) and finished
 * (bit 5) in its flag word or, with `use_alt`, in its +0x2D8 word; `index` 4 asks whether all three are (an empty
 * element counts). */
extern "C" u32 quest_element_pick_ck(QuestWork* work, s32 index, u8 use_alt) {
    s32 count;
    s32 i;
    u32 flags;
    u32 alt;

    if (index == 4) {
        count = 0;
        for (i = 0; i < 3; i++) {
            flags = work->elements_0x0094[i].flags;
            if (use_alt == 0) {
                alt = flags;
            } else {
                alt = work->counters_0x2D8[i];
            }
            if (flags == 0) {
                count++;
            } else if ((flags & 8) != 0 && (alt & 0x20) != 0) {
                count++;
            }
        }
        return count >= 3;
    }
    flags = work->elements_0x0094[index].flags;
    if ((flags & 8) != 0) {
        if ((flags & 0x40) == 0x40) {
            return 0;
        }
        flags = (use_alt == 0) ? flags : work->counters_0x2D8[index];
        if ((flags & 0x20) != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803AA1B8 (0xE8): whether element `index` has failed (active with bit 6); `index` 4 asks whether all three have
 * (an empty element counts). */
extern "C" u32 quest_element_failed_ck(QuestWork* work, s32 index) {
    s32 count;
    s32 i;
    u32 flags;

    if (index == 4) {
        count = 0;
        for (i = 0; i < 3; i++) {
            flags = work->elements_0x0094[i].flags;
            if (flags == 0) {
                count++;
            } else if ((flags & 8) != 0 && (flags & 0x40) != 0) {
                count++;
            }
        }
        return count >= 3;
    }
    flags = work->elements_0x0094[index].flags;
    if ((flags & 8) != 0) {
        return (flags & 0x40) == 0x40;
    }
    return 0;
}

/* 0x803AA2A0 (0x20): whether element `index` is finished (flag bit 5). */
extern "C" u32 quest_element_flag20_ck(QuestWork* work, s32 index) {
    return (work->elements_0x0094[index].flags & 0x20) != 0;
}

/* 0x803AA2C0 (0x30): whether element `index` is set up and active (flag bit 3). */
extern "C" s32 quest_element_live_ck(QuestWork* work, s32 index) {
    u32 flags = work->elements_0x0094[index].flags;
    if (flags == 0) {
        return 0;
    }
    return (flags & 8) != 0;
}

/* 0x803AA2F0 (0x12C): whether one of the elements the quest grade selects has failed, for a row whose +0x310 flags
 * carry exactly one of bits 0 and 2. */
extern "C" u32 quest_failed_ck(QuestWork* work) {
    QuestRecord* record = work->record_0x03C;
    u32 kind;
    u32 mask;
    s32 i;
    u32 flags;

    if (record == NULL) {
        return 0;
    }
    kind = record->flags_0x310 & 5;
    if (kind == 0) {
        return 0;
    }
    if (kind == 5) {
        return 0;
    }
    switch (work->grade_0x93) {
    case 3:
        mask = 3;
        break;
    case 4:
        mask = 7;
        break;
    case 6:
        mask = 5;
        break;
    default:
        mask = 1;
        break;
    }
    for (i = 0; i < 3; i++) {
        if ((mask & (1 << i)) != 0) {
            flags = work->elements_0x0094[i].flags;
            if ((flags & 8) != 0 && (flags & 0x40) == 0x40) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x803AA41C (0x88): 1 when the slot's sub-state is 3, -1 at 8 (and at 5/7 for a non-zero `flag`), else 0;
 * -1 without a move work. */
extern "C" s32 quest_sub_state_end_ck(s32 flag) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return -1;
    }
    if (work->sub_0xFA == 8) {
        return -1;
    }
    if (work->sub_0xFA == 3) {
        return 1;
    }
    if (work->sub_0xFA == 5 || work->sub_0xFA == 7) {
        if (flag != 0) {
            return -1;
        }
    }
    return 0;
}

/* 0x803A3A50 (0x694): one step of the player's talk with the quest NPC: opens the greeting, then walks the swap/gift
 * dialogue `+0x1C0` tracks; returns 0 once the talk is over. */
extern "C" s32 npc_talk_step(NoteWork* self) {
    _PLW* me = my_player_work_get();

    if (Pl_motion_input_ck(1) == 1) {
        me->talk_wait_0x668 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C1 = 0;
        npc_talk_end();
        return 0;
    }
    if (me->talk_wait_0x668 == 0) {
        if (self->field_0x1C1 == 0 && (u16)pl_carry_item_get(me) == 0xFFFF && me->talk_left_0x669 <= 6) {
            me->talk_left_0x669--;
        }
        self->field_0x1C0 = 0;
        self->field_0x1C1 = 0;
        npc_talk_end();
        return 0;
    }
    me->talk_wait_0x668 = 5;
    switch (self->field_0x1C0) {
    case 0:
        self->field_0x1C1 = 0;
        if (me->talk_left_0x669 <= 6 || (u16)pl_carry_item_get(me) != 0xFFFF) {
            self->field_0x1C0 = 2;
            switch (ran_suu(0) % 3) {
            case 0:
                npc_talk_start(1, 0, 0, 1);
                break;
            case 1:
                npc_talk_start(1, 1, 0, 1);
                break;
            case 2:
                npc_talk_start(1, 2, 0, 1);
                break;
            }
            if ((u16)pl_carry_item_get(me) != 0xFFFF) {
                self->field_0x1C1 = 1;
            }
        } else {
            self->field_0x1C0 = 1;
            if (note_pane_anim_pair_ck(self, 0, 6) != 0) {
                npc_talk_start(1, 9, 0, 1);
            } else if (note_pane_anim_pair_ck(self, 0, 7) != 0) {
                npc_talk_start(1, 10, 0, 1);
            } else {
                npc_talk_start(1, 8, 0, 1);
            }
        }
        break;
    case 1:
        if (npc_talk_active_ck() == 0) {
            me->talk_wait_0x668 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            return 0;
        }
        break;
    case 2:
        if (npc_talk_active_ck() == 0) {
            switch ((u8)npc_trade_pick(me)) {
            case 0:
                self->field_0x1C0 = 3;
                npc_talk_start(1, 3, me->trade_want_0x66C, 1);
                break;
            case 1:
                self->field_0x1C0 = 5;
                npc_talk_start(1, 3, me->trade_want_0x66C, 1);
                break;
            case 2:
                if (npc_gift_roll(me) != 0) {
                    self->field_0x1C0 = 4;
                } else {
                    self->field_0x1C0 = 6;
                }
                npc_talk_start(1, 7, 0, 1);
                break;
            case 3:
                self->field_0x1C0 = 8;
                npc_talk_start(1, 13, me->trade_want_0x66C, 1);
                break;
            case 4:
                self->field_0x1C0 = 10;
                npc_talk_start(1, 13, me->trade_want_0x66C, 1);
                break;
            default:
                me->talk_wait_0x668 = 0;
                self->field_0x1C0 = 0;
                self->field_0x1C1 = 0;
                return 0;
            }
        }
        break;
    case 3:
        if (npc_talk_active_ck() == 0) {
            if (talk_msg_choice_ck() == 0) {
                pl_item_add(me, me->trade_want_0x66C, me->trade_count_0x66E);
                pl_item_add(me, me->trade_give_0x670, 1);
                pl_model_state_set(me, 2, 39, me->trade_want_0x66C);
                pl_model_state_set(me, 2, 8, me->trade_give_0x670);
            }
            me->talk_wait_0x668 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            return 0;
        }
        break;
    case 4:
        if (npc_talk_active_ck() == 0) {
            pl_item_add(me, me->trade_give_0x670, 1);
            pl_model_state_set(me, 2, 8, me->trade_give_0x670);
            me->talk_wait_0x668 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            return 0;
        }
        break;
    case 5:
        if (npc_talk_active_ck() == 0) {
            if (talk_msg_choice_ck() == 0) {
                self->field_0x1C0 = 7;
                if (Pl_item_timer_get(me, me->trade_give_0x670) > 0) {
                    npc_talk_start(1, 12, me->trade_give_0x670, 1);
                } else {
                    npc_talk_start(1, 11, 0, 1);
                }
                break;
            }
            me->talk_wait_0x668 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            return 0;
        }
        break;
    case 6:
        if (npc_talk_active_ck() == 0) {
            self->field_0x1C0 = 7;
            if (Pl_item_timer_get(me, me->trade_give_0x670) > 0) {
                npc_talk_start(1, 12, me->trade_give_0x670, 1);
            } else {
                npc_talk_start(1, 11, 0, 1);
            }
        }
        break;
    case 7:
        if (npc_talk_active_ck() == 0) {
            me->talk_left_0x669--;
            me->talk_wait_0x668 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            return 0;
        }
        break;
    case 8:
        if (npc_talk_active_ck() == 0) {
            self->field_0x1C0++;
            pl_item_add(me, me->trade_want_0x66C, me->trade_count_0x66E);
            npc_talk_start(1, 14, me->trade_want_0x66C, 1);
        }
        break;
    case 9:
        if (npc_talk_active_ck() == 0) {
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            pl_item_add(me, me->trade_give_0x670, 1);
            pl_model_state_set(me, 2, 8, me->trade_give_0x670);
            me->talk_wait_0x668 = 0;
            return 0;
        }
        break;
    case 10:
        if (npc_talk_active_ck() == 0) {
            self->field_0x1C0++;
            pl_item_add(me, me->trade_want_0x66C, me->trade_count_0x66E);
            if (Pl_item_timer_get(me, me->trade_give_0x670) > 0) {
                npc_talk_start(1, 12, me->trade_give_0x670, 1);
            } else {
                npc_talk_start(1, 11, 0, 1);
            }
        }
        break;
    case 11:
        if (npc_talk_active_ck() == 0) {
            self->field_0x1C0 = 0;
            self->field_0x1C1 = 0;
            me->talk_wait_0x668 = 0;
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x803A40E4 (0x8C): steps angle `cur` towards `target` by `step`, snapping on within one step. */
extern "C" u16 note_pane_angle_step(u16 target, u16 cur, u16 step) {
    u16 diff = target - cur;
    u16 result;

    if (abs((s16)diff) < step) {
        result = target;
    } else if (diff < 0x8000) {
        result = cur + step;
    } else {
        result = cur - step;
    }
    return result;
}

/* 0x803A4214 (0xC4): puts the pane at its map's home spot, facing its current heading, idle, and starts its voice
 * entry. */
extern "C" void note_pane_init(NoteWork* self) {
    nw4r::math::VEC3 home;
    nw4r::math::VEC3* rest = &self->vec_0x1B4;

    VEC3_ctor(&home);
    self->field_0x001 = 1;
    self->flag_0x1AC = 1;
    vec_to_mh_vec3(&home, &note_pane_home_table[stage_map_kind_get(self->map_0x194)]);
    copyVec3(&self->vec_0x170, copyVec3(&self->vec_0x17C, &home));
    self->field_0x1A8 = self->field_0x18C;
    copyVec3(rest, &self->vec_0x170);
    self->field_0x1C0 = 0;
    self->field_0x1C1 = 0;
    note_pane_anim_pair_0_1(self);
    self->field_0x1F4 = (s32)se_entry_request(10, (struct _ENEMY_WORK*)self, qnpc_snd_func_get());
}

/* 0x803A42D8 (0x50): one frame of the pane: its sub-state (twice when a re-run is pending), then its heading. */
extern "C" void note_pane_step(NoteWork* self) {
    note_pane_dispatch(self);
    if (self->field_0x1A1 == 1) {
        note_pane_dispatch(self);
        self->field_0x1A1 = 0;
    }
    note_pane_pos_step(self);
}

/* 0x803A4328 (0x120): pane state 0, idle: waits 187 frames, then picks one of the three idle pairs; a player close
 * by switches it to pair (0,4). */
extern "C" void note_pane_state_0(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        self->field_0x16C = 187;
        note_pane_motion_set(self, 1, 4, 0);
        break;
    case 1:
        if (note_pane_player_near_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 4);
        } else if (--self->field_0x16C < 0) {
            switch (ran_suu(1) % 3) {
            case 0:
                note_pane_set_anim_pair(self, 0, 1);
                break;
            case 1:
                note_pane_set_anim_pair(self, 0, 2);
                break;
            case 2:
                note_pane_set_anim_pair(self, 0, 3);
                break;
            }
        }
        break;
    }
}

/* 0x803A4448 (0xF0): pane state 1, looking around: turns towards its home for 100..163 frames. */
extern "C" void note_pane_state_1(NoteWork* self) {
    u16 angle;

    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        self->field_0x16C = (ran_suu(1) & 0x3F) + 100;
        note_pane_motion_set(self, 10, 4, 0);
        break;
    case 1:
        if (--self->field_0x16C <= 0) {
            note_pane_set_anim_pair(self, 0, 3);
        } else {
            angle = note_pane_angle_step(calcVecAng2(&self->vec_0x170, &self->vec_0x17C), self->field_0x18C, 0x100);
            self->field_0x18C = angle;
            self->field_0x1A8 = angle;
        }
        if (note_pane_player_near_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 4);
        }
        break;
    }
}

/* 0x803A4538 (0x160): pane state 2, fidgeting: motion 2 for a random time, then motion 1 for another or the
 * one-shot motion 3. */
extern "C" void note_pane_state_2(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        self->field_0x16C = ran_suu(1) & 0xBF;
        note_pane_motion_set(self, 2, 2, 0);
        break;
    case 1:
        if (--self->field_0x16C <= 0) {
            if ((ran_suu(1) & 1) != 0) {
                self->field_0x169 = 2;
                self->field_0x16C = ran_suu(1) & 0xBF;
                note_pane_motion_set(self, 1, 4, 0);
            } else {
                self->field_0x169 = 3;
                note_pane_motion_set(self, 3, 4, 0);
            }
        }
        break;
    case 2:
        if (--self->field_0x16C <= 0) {
            self->field_0x169 = 0;
        }
        break;
    case 3:
        if (note_pane_motion_end_ck(self) == 1) {
            self->field_0x169 = 0;
        }
        break;
    }
    if (note_pane_player_near_ck(self) == 1) {
        note_pane_set_anim_pair(self, 0, 4);
    }
}

/* 0x803A4698 (0xA4): pane state 3: plays motion 11, then goes back to pair (0,1). */
extern "C" void note_pane_state_3(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        self->field_0x16C = 0;
        note_pane_motion_start(self, 11, 6, 0);
        break;
    case 1:
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_anim_pair_0_1(self);
        }
        if (note_pane_player_near_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 4);
        }
        break;
    }
}

/* 0x803A473C (0x1D0): pane state 4, talking: turns to the player, picks an idle pair when nothing can be swapped, and
 * runs the talk until it ends. */
extern "C" void note_pane_state_4(NoteWork* self) {
    _PLW* me = my_player_work_get();

    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_start(self, 4, 8, 0);
        self->field_0x1A8 = calcVecAng2(&self->vec_0x170, &me->vec_0x03C);
        if (me->talk_left_0x669 >= 7 && (u16)pl_carry_item_get(me) == 0xFFFF) {
            switch (ran_suu(0) % 3) {
            case 0:
                note_pane_set_anim_pair(self, 0, 6);
                break;
            case 1:
                note_pane_set_anim_pair(self, 0, 7);
                break;
            case 2:
                note_pane_set_anim_pair(self, 0, 8);
                break;
            }
        }
        if (npc_talk_step(self) == 0) {
            npc_talk_flag_set(30);
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 1:
        if (npc_talk_step(self) == 0) {
            npc_talk_flag_set(30);
            note_pane_anim_pair_0_1(self);
        } else if (note_pane_motion_end_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 5);
        } else if ((self->field_0x1C0 == 3 && talk_msg_index_get() == 5) || self->field_0x1C0 == 4 ||
                   self->field_0x1C0 == 9) {
            note_pane_set_anim_pair(self, 0, 9);
        }
        break;
    }
}

/* 0x803A490C (0xE8): pane state 5, talking on: motion 5 while the talk runs. */
extern "C" void note_pane_state_5(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_start(self, 5, 4, 0);
        if (npc_talk_step(self) == 0) {
            npc_talk_flag_set(30);
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 1:
        if (npc_talk_step(self) == 0) {
            npc_talk_flag_set(30);
            note_pane_anim_pair_0_1(self);
        } else if ((self->field_0x1C0 == 3 && talk_msg_index_get() == 5) || self->field_0x1C0 == 4 ||
                   self->field_0x1C0 == 9) {
            note_pane_set_anim_pair(self, 0, 9);
        }
        break;
    }
}

/* 0x803A49F4 (0x1B0): pane states 6..8, one of three reaction motions while the talk runs, then back to motion 1. */
extern "C" void note_pane_state_6(NoteWork* self, u32 variant) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        switch ((u8)variant) {
        case 0:
            note_pane_motion_start(self, 6, 4, 0);
            break;
        case 1:
            note_pane_motion_start(self, 7, 4, 0);
            break;
        case 2:
            note_pane_motion_start(self, 8, 6, 0);
            break;
        }
        if (npc_talk_step(self) == 0) {
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 1:
        npc_talk_flag_set(45);
        if (npc_talk_step(self) == 0) {
            if (note_pane_motion_end_ck(self) == 1) {
                note_pane_anim_pair_0_1(self);
            } else {
                self->field_0x169 = 3;
            }
        } else if (note_pane_motion_end_ck(self) == 1) {
            self->field_0x169++;
            note_pane_motion_start(self, 1, 4, 0);
        }
        break;
    case 2:
        if (npc_talk_step(self) == 0) {
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 3:
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_anim_pair_0_1(self);
        } else if (note_pane_player_near_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 4);
        }
        break;
    }
}

/* Spawns the pane's hand effect: the point (10, -10, 0) in joint 6's frame. */
static inline void note_pane_hand_fx(NoteWork* self, nw4r::math::MTX34* mtx, nw4r::math::VEC3* pos,
                                     nw4r::math::VEC3* ofs) {
    note_pane_joint_mtx_get(self, 6, mtx);
    copyVec3(pos, ofs);
    mulVecMatAddTrans(pos, mtx);
    eft004_scaled_spawn(pos, self->field_0x195, 1.0f);
}

/* 0x803A4BA4 (0x230): pane state 9, handing over: motion 9 with the hand effect at frame 140, then back to motion 1. */
extern "C" void note_pane_state_9(NoteWork* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    setVec3(&ofs, 10.0f, -10.0f, 0.0f);
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_start(self, 9, 4, 0);
        npc_talk_flag_set(90);
        if (npc_talk_step(self) == 0) {
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 1:
        if (npc_talk_step(self) == 0) {
            if (note_pane_motion_end_ck(self) == 1) {
                note_pane_anim_pair_0_1(self);
            } else {
                self->field_0x169 = 3;
            }
        } else if (note_pane_motion_end_ck(self) == 1) {
            self->field_0x169++;
            note_pane_motion_start(self, 1, 4, 0);
        } else if (note_pane_motion_frame_ck(self, 0, 140.0f, 0.0f) != 0) {
            note_pane_hand_fx(self, &mtx, &pos, &ofs);
        }
        break;
    case 2:
        if (npc_talk_step(self) == 0) {
            note_pane_anim_pair_0_1(self);
        }
        break;
    case 3:
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_anim_pair_0_1(self);
        } else {
            if (note_pane_motion_frame_ck(self, 0, 140.0f, 0.0f) != 0) {
                note_pane_hand_fx(self, &mtx, &pos, &ofs);
            }
            if (note_pane_player_near_ck(self) == 1) {
                note_pane_set_anim_pair(self, 0, 4);
            }
        }
        break;
    }
}

/* 0x803A4EC0 (0xC): whether the save block's lobby timer has run out with its flag set. */
extern "C" s32 note_timer_lobby_ready_ck(void) {
    return note_timer_ready_ck(&((Q_UserData*)lobby_world_block)->note_timer_0x5270);
}

/* 0x803A4ECC (0xB0): one tick of the save block's lobby timer: counts its countdown down and steps its two random
 * words (twice, and fifty times each). */
extern "C" void note_timer_tick(void) {
    NoteTimer* timer = &((Q_UserData*)lobby_world_block)->note_timer_0x5270;
    u32 i;
    u16 j;
    u32 k;

    if (timer->voyage_0x06 > 0) {
        timer->voyage_0x06--;
    }
    for (i = 0; i < 2; i++) {
        timer->seed_0x04 = rand_lcg_step(timer->seed_0x04);
    }
    for (j = 0; j < 4; j++) {
        for (k = 0; k < 50; k++) {
            timer->seed_0x10[j] = rand_lcg_step(timer->seed_0x10[j]);
        }
    }
}

/* The note trade's tables, defined at the foot of the file (after every body, as the target lays its
 * relocations out). */
extern u8 note_bonus_kind_tbl[2][5][2];
extern u16 note_title_tbl[16];
extern u16 note_frame_tbl[5];
extern u16 note_frame_wide_tbl[5];
extern u16 note_full_text_tbl[7];
extern u16 note_slot_back_tbl[17];
extern u16 note_slot_tbl[10];
extern u16 note_slot_cursor_tbl[10];
extern u16 note_slots_back_tbl[11];
extern u16 note_bonus_tbl[6];
extern u16 note_offer_back_tbl[12];
extern u16 note_offer_arrow_tbl[6];
extern u16 note_offer_goods_tbl[10];
extern u16 note_offer_voyage_tbl[13];
extern u16 note_itembox_cell_tbl[11];
extern u16 note_itembox_back_tbl[7];
extern u16 note_trade_menu_sprite_tbl[12];
extern u16 note_itembox_frame_tbl[26];
extern u16 note_route_tbl[9];
extern u16 note_reward_back_tbl[24];
extern u16 note_reward_row_tbl[6];
extern u16 note_voyage_route_tbl[25];
extern u16 note_full_back_tbl[3];
extern u16 note_full_tbl[3];
extern u16 note_full_glow_tbl[3];
extern u16 note_points_tbl[3];
extern u16 note_arrow_tbl[4];
extern u16 note_slot_row_tbl[4];
extern u16 note_full_row_tbl[4];
extern u16 note_offer_kind_tbl[3];
extern u16 note_offer_points_tbl[4];
extern u16 note_trade_menu_row_tbl[3];

/* -------------------------------------------------------------------------------------------------
 * Screen 0x17, the note trade (0x803A5100..0x803A75D8): the captain's four trade routes, their points towards a
 * reward, the six goods offers of a route and their draw.
 * ------------------------------------------------------------------------------------------------- */


/* 0x803A5100 (0x1A4): opens the note trade for `player` at `npc`: the routes the story has opened, and the bonus kind
 * and route of this visit. */
extern "C" void note_trade_open(struct _PLW* player, struct _LB_NPC* npc) {
    LbNoteTradeWork* w = lobby_w.note_trade_0x0AC;
    u16 seed;
    u16 roll;
    u16 sum;
    s32 i;
    u8* p;

    memset(w, 0, 0x2000);
    w->player_0xD4 = player;
    w->npc_0xD8 = npc;
    lobby_w.state_0x000 = 0x17;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
    w->timer_0xD0 = &((Q_UserData*)lobby_world_block)->note_timer_0x5270;
    if (lb_unlock_cond_ck(35) == 1) {
        w->slot_count_0x36 = 4;
    } else if (lb_unlock_cond_ck(33) == 1) {
        w->slot_count_0x36 = 3;
    } else if (lb_unlock_cond_ck(31) == 1) {
        w->slot_count_0x36 = 2;
    } else {
        w->slot_count_0x36 = 1;
    }
    seed = rand_lcg_step(w->timer_0xD0->seed_0x04);
    roll = seed % 100;
    sum = 0;
    for (i = 0, p = note_bonus_kind_tbl[w->slot_count_0x36 != 1][0]; i < 5; p += 2, i++) {
        sum += *p;
        if (roll < sum) {
            w->bonus_kind_0x02 = note_bonus_kind_tbl[w->slot_count_0x36 != 1][i][1];
            break;
        }
    }
    seed = rand_lcg_step(seed);
    w->bonus_slot_0x03 = seed % w->slot_count_0x36;
    ainpc_page_mode2_set();
}

/* 0x803A52A4 (0x4): closes the note trade. */
extern "C" void note_trade_close(void) {
    lb_panel_close();
}

/* 0x803A52A8 (0x3A8): one step of the note trade: the greeting (or the points hand-over when the ship is back), the
 * top menu, the goods selection, the route view and the reward view. */
extern "C" void note_trade_step(void) {
    LbNoteTradeWork* w = lobby_w.note_trade_0x0AC;
    s16 i;
    u16 off;

    w->moved_0x3C = 0;
    switch (w->state_0x00) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            if ((u32)note_timer_ready_ck(w->timer_0xD0) == 1) {
                w->state_0x00 = 4;
                w->unused_0x2A = 0;
                note_trade_points_init(w);
            } else {
                w->state_0x00 = 1;
                off = 0;
                if (w->timer_0xD0->voyage_0x06 != 0) {
                    off = 1;
                }
                lb_choice_init(&w->menu_0x04, &note_trade_menu_tmpl, off, 0);
            }
            camera_talk_lock_set(1);
            sysSE_stop(41);
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(note_trade_points_step(w) - 1) <= 1) {
            w->state_0x00 = 1;
            lb_choice_init(&w->menu_0x04, &note_trade_menu_tmpl, 0, 0);
            if (w->timer_0xD0->voyage_0x06 != 0) {
                w->menu_0x04.off_mask_0x02 = 1;
            }
        }
        if (++w->blink_0x28 >= 40) {
            w->blink_0x28 = 0;
            if (w->sub_0x01 == 1) {
                for (i = 0; i < w->slot_count_0x36; i++) {
                    if (w->timer_0xD0->points_0x0C[i] >= 100) {
                        break;
                    }
                }
            }
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&w->menu_0x04)) {
        case 1:
            switch (w->menu_0x04.cursor_0x00) {
            case 0:
                w->state_0x00 = 2;
                note_trade_goods_count(w);
                break;
            case 1:
                w->state_0x00 = 3;
                break;
            case 2:
                w->state_0x00 = 5;
                w->offer_cursor_0x3A = 0;
                w->reward_cursor_0x2C = 0;
                break;
            }
            break;
        case 2:
            w->state_0x00 = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20) != 0) {
            w->state_0x00 = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            w->reward_cursor_0x2C = menu_cursor_step(w->reward_cursor_0x2C, 6, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            w->offer_cursor_0x3A = menu_cursor_step_fixed_tail(w->offer_cursor_0x3A, w->slot_count_0x36,
                                                               lb_cmd_repeat_get(), 4, 8, &w->moved_0x3C);
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if ((u32)(note_trade_select_step(w) - 1) <= 1) {
            w->state_0x00 = 1;
            if (w->timer_0xD0->voyage_0x06 != 0) {
                w->menu_0x04.off_mask_0x02 = 1;
            }
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (++w->blink_0x28 >= 40) {
            w->blink_0x28 = 0;
        }
        if (lb_cmd_pressed_ck(0x20) != 0) {
            w->state_0x00 = 1;
            sysSE_req(1);
        }
        break;
    case 6:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            note_trade_close();
        }
        break;
    }
    if (w->state_0x00 != 0 && w->state_0x00 != 6) {
        subTransSet((u32)note_trade_draw, 0, NULL);
    }
}

/* 0x803A5650 (0x30): starts the points hand-over: 20 frames' wait, and the bonus route defaults to the trade's own. */
extern "C" void note_trade_points_init(LbNoteTradeWork* w) {
    NoteTimer* timer = w->timer_0xD0;

    w->sub_0x01 = 0;
    w->wait_0x30 = 20;
    if ((s8)timer->bonus_gain_0x03 == 0) {
        timer->bonus_slot_0x02 = timer->slot_0x00;
    }
}

/* 0x803A5680 (0x4A8): one step of the points hand-over: counts the trade's points onto its routes (all at once on
 * decide), marks the routes that reach 100, then hands out their rewards; 1 once done. */
extern "C" s32 note_trade_points_step(LbNoteTradeWork* w) {
    NoteTimer* timer = w->timer_0xD0;
    s32 done = 0;
    s32 counted;
    s32 i;
    s32 bit;
    u16 slot;
    u8 bonus;

    switch (w->sub_0x01) {
    case 0:
        if (w->wait_0x30 > 0) {
            w->wait_0x30--;
            break;
        }
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if ((s8)timer->gain_0x01 > 0) {
                timer->points_0x0C[timer->slot_0x00] += timer->gain_0x01;
                timer->gain_0x01 = 0;
            }
            if ((s8)timer->bonus_gain_0x03 > 0) {
                timer->points_0x0C[timer->bonus_slot_0x02] += timer->bonus_gain_0x03;
                timer->bonus_gain_0x03 = 0;
            }
            sysSE_req(0);
        } else {
            counted = 0;
            if ((s8)timer->gain_0x01 > 0) {
                timer->gain_0x01--;
                timer->points_0x0C[timer->slot_0x00]++;
                counted = 1;
            }
            if ((s8)timer->bonus_gain_0x03 > 0) {
                timer->bonus_gain_0x03--;
                timer->points_0x0C[timer->bonus_slot_0x02]++;
                counted = 1;
            }
            if (counted != 0) {
                sysSE_stop(47);
            }
        }
        for (i = 0; i < w->slot_count_0x36; i++) {
            bit = 1 << i;
            if ((w->full_mask_0x26 & bit) != 0 && w->full_blink_0xDE[i] < 40) {
                w->full_blink_0xDE[i]++;
            }
            if (w->timer_0xD0->points_0x0C[i] >= 100 && (w->full_mask_0x26 & bit) == 0) {
                sysSE_stop(48);
                w->full_mask_0x26 |= (u16)bit;
                w->full_blink_0xDE[i] = 1;
            }
        }
        if ((s8)timer->gain_0x01 == 0 && (s8)timer->bonus_gain_0x03 == 0) {
            w->sub_0x01 = 1;
        }
        break;
    case 1:
        for (i = 0; i < w->slot_count_0x36; i++) {
            if ((w->full_mask_0x26 & (1 << i)) != 0 && w->full_blink_0xDE[i] < 40) {
                w->full_blink_0xDE[i]++;
            }
        }
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (timer->points_0x0C[timer->slot_0x00] >= 100 || timer->points_0x0C[timer->bonus_slot_0x02] >= 100) {
                w->sub_0x01 = 2;
                memset(&w->get_0x44, 0, sizeof(w->get_0x44));
                if (timer->points_0x0C[timer->slot_0x00] >= 100) {
                    slot = timer->slot_0x00;
                    w->get_0x44.item_0x00 = note_slot_table[slot][timer->reward_0x08[slot]];
                    w->get_0x44.count_0x02 = 1;
                    ((Q_UserData*)lobby_world_block)->note_rewards_0x5288 |= 1 << (slot * 6 + timer->reward_0x08[slot]);
                }
                bonus = timer->bonus_slot_0x02;
                if (timer->points_0x0C[bonus] >= 100 && bonus != timer->slot_0x00) {
                    w->get_0x44.item_0x04 = note_slot_table[bonus][timer->reward_0x08[bonus]];
                    w->get_0x44.count_0x06 = 1;
                    ((Q_UserData*)lobby_world_block)->note_rewards_0x5288 |= 1 << (bonus * 6 + timer->reward_0x08[bonus]);
                }
                eft052_item_get_open(&w->get_0x44, lb_item_get_data + 0x80);
                sysSE_stop(12);
            } else {
                done = 1;
                sysSE_req(0);
            }
            w->full_mask_0x26 = 0;
        }
        break;
    case 2:
        if ((u32)(eft052_item_get_step() - 1) <= 1) {
            done = 1;
            if (timer->points_0x0C[timer->slot_0x00] >= 100) {
                timer->points_0x0C[timer->slot_0x00] -= 100;
                timer->reward_0x08[timer->slot_0x00] = ran_suu(1) % 6;
            }
            bonus = timer->bonus_slot_0x02;
            if (timer->points_0x0C[bonus] >= 100 && bonus != timer->slot_0x00) {
                timer->points_0x0C[bonus] -= 100;
                timer->reward_0x08[timer->bonus_slot_0x02] = ran_suu(1) % 6;
            }
        }
        break;
    }
    return done;
}

/* 0x803A5B28 (0x38): starts the goods selection and counts the goods table. */
extern "C" void note_trade_goods_count(LbNoteTradeWork* w) {
    s16 count;
    NoteGoods* goods;

    w->sub_0x01 = 0;
    w->slot_0x24 = 0;
    for (count = 0, goods = note_goods_tbl[0]; goods->items_0x00[0].item_0x00 != 0; goods++) {
        count++;
    }
    w->goods_count_0x32 = count;
}

/* 0x803A5B60 (0x4FC): rolls the six offers of route `slot`: each a random goods record (never twice) with its kind,
 * its points and the bonus the kind rolls, five points more for the visit's bonus kind or route. */
extern "C" void note_trade_offers_roll(LbNoteTradeWork* w, s16 slot) {
    s8 order[45];
    u16 seed;
    s16 i;
    NoteGoods* table;
    NoteOffer* offer;
    u16 pick;
    s16 left;
    s32 j;
    s8* dice;
    s8 delta;
    u16 roll;
    s8 sum;
    s16 k;
    s32 boost;

    seed = w->timer_0xD0->seed_0x10[slot];
    for (i = 0; i < 45; i++) {
        if (i < w->goods_count_0x32) {
            order[i] = i;
        } else {
            order[i] = -1;
        }
    }
    table = note_goods_tbl[0];
    w->offer_count_0x38 = 6;
    w->offer_cursor_0x3A = 0;
    for (i = 0, offer = w->offers_0x70; i < w->offer_count_0x38; i++, offer++) {
        memset(offer, 0, sizeof(NoteOffer));
        seed = rand_lcg_step(seed);
        if (w->slot_count_0x36 == 1) {
            switch (seed % 4) {
            case 0:
                offer->kind_0x04 = 0;
                break;
            case 1:
                offer->kind_0x04 = 1;
                break;
            case 2:
                offer->kind_0x04 = 4;
                break;
            case 3:
                offer->kind_0x04 = 5;
                break;
            }
        } else {
            offer->kind_0x04 = seed % 6;
        }
        seed = rand_lcg_step(seed);
        left = w->goods_count_0x32 - i;
        if (left > 0) {
            pick = seed % left;
        } else {
            pick = 0;
        }
        offer->goods_0x00 = &table[order[pick]];
        memcpy(&order[pick], &order[pick + 1], 44 - pick);
        offer->short_0x05 = 0;
        for (j = 0; j < 2; j++) {
            if (offer->goods_0x00->items_0x00[j].item_0x00 != 0 &&
                (s32)userdata_item_count_total(offer->goods_0x00->items_0x00[j].item_0x00, lobby_world_block) <
                    offer->goods_0x00->items_0x00[j].count_0x02) {
                offer->short_0x05 = 1;
            }
        }
        offer->points_0x06 = (s8)offer->goods_0x00->points_0x08;
        offer->points_max_0x0C = offer->goods_0x00->points_0x08;
        offer->points_min_0x0A = offer->goods_0x00->points_0x08;
        dice = note_kind_dice_tbl[offer->kind_0x04];
        offer->points_max_0x0C += dice[1];
        offer->points_min_0x0A += dice[5];
        seed = rand_lcg_step(seed);
        roll = seed % 100;
        for (k = 0, sum = 0; dice[k * 2] != 0; k++) {
            sum += dice[k * 2];
            if (roll < sum) {
                delta = dice[k * 2 + 1];
                if ((u32)(offer->kind_0x04 - 2) <= 1) {
                    if (delta <= 0) {
                        offer->points_0x06 += delta;
                    } else if (w->slot_count_0x36 > 1) {
                        offer->bonus_0x07 += delta;
                        seed = rand_lcg_step(seed);
                        offer->bonus_slot_0x08 =
                            (w->slot_0x24 + seed % (w->slot_count_0x36 - 1) + 1) % w->slot_count_0x36;
                    }
                } else {
                    offer->points_0x06 += delta;
                }
                break;
            }
        }
        boost = 0;
        switch (w->bonus_kind_0x02) {
        case 1:
            if (w->bonus_slot_0x03 == slot) {
                boost = 1;
            }
            break;
        case 2:
            if (offer->kind_0x04 <= 1) {
                boost = 1;
            }
            break;
        case 3:
            if ((u32)(offer->kind_0x04 - 2) <= 1) {
                boost = 1;
            }
            break;
        case 4:
            if ((u32)(offer->kind_0x04 - 4) <= 1) {
                boost = 1;
            }
            break;
        }
        if (boost == 1) {
            offer->points_0x06 += 5;
            offer->points_max_0x0C += 5;
            offer->points_min_0x0A += 5;
        }
    }
}

/* 0x803A605C (0xD8): sends the chosen offer: takes its goods out of the box and starts the voyage with its points
 * (and its bonus route). */
extern "C" void note_trade_accept(LbNoteTradeWork* w) {
    NoteOffer* offer = &w->offers_0x70[w->offer_cursor_0x3A];
    s32 i;

    for (i = 0; i < 2; i++) {
        if (offer->goods_0x00->items_0x00[i].item_0x00 != 0) {
            eft052_page_count_add(offer->goods_0x00->items_0x00[i].item_0x00, offer->goods_0x00->items_0x00[i].count_0x02);
        }
    }
    w->timer_0xD0->voyage_0x06 = note_voyage_tbl[w->slot_0x24];
    w->timer_0xD0->slot_0x00 = w->slot_0x24;
    w->timer_0xD0->gain_0x01 = offer->points_0x06;
    if (offer->bonus_0x07 > 0) {
        w->timer_0xD0->bonus_slot_0x02 = offer->bonus_slot_0x08;
        w->timer_0xD0->bonus_gain_0x03 = offer->bonus_0x07;
    }
}

/* 0x803A6134 (0x1FC): one step of the goods selection: the route, then the offer, then the yes/no; 1 once an offer
 * is sent, 2 on cancel. */
extern "C" s32 note_trade_select_step(LbNoteTradeWork* w) {
    s32 result = 0;

    switch (w->sub_0x01) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            w->sub_0x01 = 1;
            note_trade_offers_roll(w, w->slot_0x24);
            sysSE_req(27);
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            w->slot_0x24 = menu_cursor_step(w->slot_0x24, w->slot_count_0x36, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (w->offers_0x70[w->offer_cursor_0x3A].short_0x05 == 0) {
                w->sub_0x01 = 2;
                w->confirm_0x40 = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            w->sub_0x01 = 0;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            w->offer_cursor_0x3A = menu_cursor_step_forward(w->offer_cursor_0x3A, w->offer_count_0x38,
                                                            lb_cmd_repeat_get(), 4, 8, 6, &w->moved_0x3C);
        }
        break;
    case 2:
        switch (toggle_word_step(&w->confirm_0x40, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            result = 1;
            note_trade_accept(w);
            sysSE_stop(45);
            break;
        case 2:
            w->sub_0x01 = 1;
            break;
        }
        break;
    }
    return result;
}

/* 0x803A6330 (0x80): draws the screen's frame (the wide or the narrow one) and its title. */
extern "C" void note_trade_frame_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(9810, &pos);
        draw_sprite_ary(note_frame_wide_tbl, &pos);
    } else {
        get_lsp_data(9805, &pos);
        draw_sprite_ary(note_frame_tbl, &pos);
    }
    get_lsp_data(9789, &pos);
    draw_sprite_ary(note_title_tbl, &pos);
}

/* 0x803A63B0 (0x118): puts the help line of the current state (and its note), and the yes/no of the confirmation. */
extern "C" void note_trade_help_draw(LbNoteTradeWork* w) {
    u16 help = 0xFFFF;
    u16 note = 0xFFFF;

    switch (w->state_0x00) {
    case 1:
        help = w->menu_0x04.cursor_0x00 + 437;
        if (w->timer_0xD0->voyage_0x06 != 0 && w->menu_0x04.cursor_0x00 == 0) {
            note = 440;
        }
        break;
    case 2:
        switch (w->sub_0x01) {
        case 0:
            help = 441;
            break;
        case 1:
            help = 442;
            if (w->offers_0x70[w->offer_cursor_0x3A].short_0x05 == 1) {
                note = 443;
            }
            break;
        case 2:
            help = 444;
            lb_panel_yes_no_draw(6265, w->confirm_0x40);
            break;
        }
        break;
    }
    if (help != 0xFFFF) {
        lb_panel_msg_draw(6263, help);
        if (note != 0xFFFF) {
            lb_panel_str_print(6263, note, 2);
        }
    }
}

/* 0x803A64C8 (0x9C): draws a full route's blinking "complete" mark at `ofs`, at frame `frame`. */
extern "C" void note_trade_full_draw(u16 frame, _mh_ivec2_* ofs) {
    _mh_ivec2_ pos;

    draw_sprite_ary(note_full_back_tbl, ofs);
    get_lsp_data(9873, &pos);
    pos.x += ofs->x;
    pos.y += ofs->y;
    draw_sprite_anim_ary(note_full_tbl, frame, &pos);
    draw_sprite_anim_ary(note_full_glow_tbl, 40, &pos);
    draw_sprite_anim_ary(note_full_text_tbl, frame, &pos);
}

/* 0x803A6564 (0x1EC): draws route `slot`'s row: its frame, the cursor, its gauge filled to its points, the points,
 * the closed mark, and the arrow when it is the hand-over's route. */
extern "C" void note_trade_slot_draw(LbNoteTradeWork* w, s16 slot, s32 open, s32 active, u32 arrow,
                                     _mh_ivec2_* pos) {
    s16 points = w->timer_0xD0->points_0x0C[slot] % 100;
    _SPR_DATA_ spr;
    s16 fill;
    _mh_ivec2_ apos;
    _mh_ivec2_ base;

    draw_sprite_anim_ary(note_slot_back_tbl, slot, pos);
    if (open != 0 && active != 0) {
        put_menu_cursor(note_slot_cursor_tbl, 0, pos);
    }
    draw_sprite_anim_ary(note_slot_tbl, slot, pos);
    spr_data_copy(&spr, get_lsp_data(9893, NULL));
    fill = (s16)(0.01f * (f32)(points * spr.width));
    if (spr.width >= fill) {
        spr.width = fill;
    }
    spr_anim_draw(&spr, slot, pos);
    draw_number_anim_ary(note_points_tbl, slot, points, 0, pos);
    draw_font_anim_idx(9885, (u16)slot, (s8*)LbStr(1, 10), 4, pos);
    if (open == 0) {
        draw_sprite_idx(9880, pos);
        draw_sprite_anim_idx(9879, active == 0, pos);
    }
    if (arrow == 1) {
        sprite_frame_apply(&spr, 9874, w->blink_0x28, &apos);
        get_lsp_data(9878, &base);
        apos.y = pos->y + (apos.y - base.y);
        draw_sprite_ary(note_arrow_tbl, &apos);
    }
}

/* 0x803A6750 (0x1D4): draws the routes' panel: every open route's row and the full marks. */
extern "C" void note_trade_slots_draw(LbNoteTradeWork* w, s32 active) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s16 i;
    const u16* row;
    const u16* full_row;
    s32 open;
    u32 arrow;

    get_lsp_data(9929, &base);
    draw_sprite_anim_ary(note_slots_back_tbl, w->slot_count_0x36, &base);
    for (i = 0, row = note_slot_row_tbl, full_row = note_full_row_tbl; i < w->slot_count_0x36;
         row++, full_row++, i++) {
        get_lsp_data(*row, &pos);
        pos.x += base.x;
        pos.y += base.y;
        arrow = 0;
        if (w->state_0x00 == 3) {
            open = 1;
            if (i == w->timer_0xD0->slot_0x00 && w->timer_0xD0->voyage_0x06 != 0) {
                arrow = 1;
            }
        } else if (w->state_0x00 == 4) {
            open = 0;
            if (i == w->timer_0xD0->slot_0x00) {
                open = 1;
                arrow = 1;
            } else if (i == w->timer_0xD0->bonus_slot_0x02) {
                open = 1;
            }
            if ((w->full_mask_0x26 & (1 << i)) != 0) {
                arrow = 0;
            }
        } else {
            open = i == w->slot_0x24;
        }
        note_trade_slot_draw(w, i, open, active, arrow, &pos);
        if ((w->full_mask_0x26 & (1 << i)) != 0) {
            get_lsp_data(*full_row, &pos);
            pos.x += base.x;
            pos.y += base.y;
            note_trade_full_draw(w->full_blink_0xDE[i], &pos);
        }
    }
}

/* 0x803A6924 (0x124): draws the captain's box at panel `panel`: its window, its title and the visit's bonus line. */
extern "C" void note_trade_bonus_draw(u16 panel, LbNoteTradeWork* w) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s8 text[128];
    s8* route;

    get_lsp_data(panel, &pos);
    spr_data_copy(&spr, get_lsp_data(9958, NULL));
    draw_window_frame_style(pos.x, pos.y, spr.width, spr.height, 0);
    draw_sprite_ary(note_bonus_tbl, &pos);
    draw_font_idx(9950, (s8*)LbStr(0, 474), 1, &pos);
    if (w->bonus_kind_0x02 == 1) {
        route = (s8*)LbStr(0, w->bonus_slot_0x03 + 470);
        sprintf((char*)text, (char*)LbStr(0, w->bonus_kind_0x02 + 475), route);
        draw_font_idx(9951, text, 0, &pos);
    } else {
        draw_font_idx(9951, (s8*)LbStr(0, w->bonus_kind_0x02 + 475), 0, &pos);
    }
}

/* 0x803A6A48 (0x394): draws the chosen offer's sheet: its kind, the page arrows, the goods it asks for, its points
 * (a range when the kind rolls them) and the route's voyage. */
extern "C" void note_trade_offer_draw(LbNoteTradeWork* w, s32 active) {
    NoteOffer* offer = &w->offers_0x70[w->offer_cursor_0x3A];
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s8 text[16];

    get_lsp_data(10013, &base);
    draw_sprite_ary(note_offer_back_tbl, &base);
    get_lsp_data(9959, &base);
    draw_sprite_ary(note_offer_kind_tbl, &base);
    draw_font_idx(9960, (s8*)LbStr(0, 480), 4, &base);
    draw_font_idx(9961, (s8*)LbStr(0, ((u32)offer->kind_0x04 >> 1) + 481), 5, &base);
    get_lsp_data(9964, &base);
    PutPageArrow(note_offer_arrow_tbl, w->offer_cursor_0x3A, w->offer_count_0x38, w->moved_0x3C, &base,
                 active ? 0 : 0x80);
    get_lsp_data(9971, &base);
    draw_sprite_ary(note_offer_goods_tbl, &base);
    draw_font_idx(9972, (s8*)LbStr(0, 1), 0, &base);
    get_lsp_data(9982, &pos);
    pos.x += base.x;
    pos.y += base.y;
    lb_item_need_list_draw(offer->goods_0x00->items_0x00, 2, 1, &pos, 3, w->npc_0xD8->field_0x002, 0);
    get_lsp_data(9983, &base);
    draw_sprite_ary(note_offer_points_tbl, &base);
    draw_font_idx(9984, (s8*)LbStr(0, 484), 0, &base);
    sprintf((char*)text, "%d", offer->points_min_0x0A);
    if (offer->points_min_0x0A != offer->points_max_0x0C) {
        draw_font_idx(9985, text, 1, &base);
        draw_font_idx(9986, (s8*)LbStr(1, 10), 0, &base);
        draw_font_idx(9987, (s8*)LbStr(1, 11), 0, &base);
        sprintf((char*)text, "%d", offer->points_max_0x0C);
        draw_font_idx(9988, text, 1, &base);
        draw_font_idx(9989, (s8*)LbStr(1, 10), 0, &base);
    } else {
        draw_font_idx(9990, text, 1, &base);
        draw_font_idx(9991, (s8*)LbStr(1, 10), 0, &base);
    }
    get_lsp_data(9995, &base);
    draw_sprite_ary(note_offer_voyage_tbl, &base);
    draw_font_idx(9996, (s8*)LbStr(0, 485), 0, &base);
    draw_font_idx(9997, (s8*)LbStr(0, offer->kind_0x04 + 486), 0, &base);
    draw_font_idx(9998, (s8*)LbStr(0, 492), 0, &base);
    draw_font_idx(9999, (s8*)LbStr(0, 638), 0, &base);
    draw_number_idx(10000, note_voyage_tbl[w->slot_0x24], 0, &base);
    ainpc_page_mark_b_draw(10025, NULL);
}

/* 0x803A6DDC (0xA4): draws the goods selection: the routes, the offer sheet once a route is picked, the captain's
 * box. */
extern "C" void note_trade_select_draw(LbNoteTradeWork* w) {
    switch (w->sub_0x01) {
    case 0:
        note_trade_slots_draw(w, 1);
        note_trade_bonus_draw(9949, w);
        break;
    case 1:
        note_trade_slots_draw(w, 0);
        note_trade_offer_draw(w, 1);
        note_trade_bonus_draw(9949, w);
        break;
    case 2:
        note_trade_slots_draw(w, 0);
        note_trade_offer_draw(w, 0);
        note_trade_bonus_draw(9949, w);
        break;
    }
}

/* 0x803A6E80 (0x70): the position of item-box cell `index` (ten a row). */
extern "C" void note_itembox_cell_pos_get(s32 index, _mh_ivec2_* out) {
    _mh_ivec2_ pos;

    get_lsp_data(note_itembox_cell_tbl[index % 10], &pos);
    out->x = pos.x;
    out->y = pos.y;
}

/* 0x803A6EF0 (0x2A0): draws the item box's row of ten cells, the cursor and the selection marks. */
extern "C" void note_itembox_row_draw(Eft052ItemBox* box) {
    _mh_ivec2_ pos;
    _mh_ivec2_ sel;
    _mh_ivec2_ base;
    _mh_ivec2_ cell;
    _SPR_DATA_ mark;
    _SPR_DATA_ icon;
    _SPR_DATA_ frame;
    BOOL found;
    s32 i;

    get_lsp_data(10099, &pos);
    draw_sprite_anim_ary(note_itembox_back_tbl, 0, &pos);
    found = FALSE;
    get_lsp_data(10088, &pos);
    spr_data_copy(&icon, get_lsp_data(4871, NULL));
    uv_pair_copy(&base, &icon);
    for (i = 0; i < 10; i++) {
        note_itembox_cell_pos_get(i, &cell);
        spr_data_copy(&frame, get_lsp_data(10098, NULL));
        uv_pair_copy(&frame, &cell);
        icon.pos.x = cell.x + base.x;
        icon.pos.y = cell.y + base.y;
        draw_sprite(frame, &pos);
        if (box->cursor_0x0E == i) {
            found = TRUE;
            uv_pair_copy(&sel, &cell);
        }
        if ((u8)(box->state_0x00 - 1) <= 1 && chk_pointer() == 0 && box->hover_0x10 == i) {
            spr_data_copy(&mark, get_lsp_data(4868, NULL));
            mark.pos.x += cell.x;
            mark.pos.y += cell.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw(&mark, box->frame_0x08, &pos);
            set_blendmode(4, 5, 1);
        }
        if (box->cells_0x24[i].item_0x00 != 0) {
            draw_itemicon_item_id(icon, box->cells_0x24[i].item_0x00, &pos);
        }
    }
    if (box->picked_0x0C != 0 && found) {
        set_blendmode(4, 1, 1);
        spr_data_copy(&mark, get_lsp_data(4869, NULL));
        mark.pos.x += sel.x;
        mark.pos.y += sel.y;
        spr_anim_draw(&mark, box->frame_0x0A, &pos);
        set_blendmode(4, 5, 1);
        spr_data_copy(&mark, get_lsp_data(4870, NULL));
        mark.pos.x += sel.x;
        mark.pos.y += sel.y;
        spr_anim_draw(&mark, box->frame_0x0A, &pos);
    }
}

/* 0x803A7190 (0x78): draws the item box: its back, the cell row, the count and the box list. */
extern "C" void note_itembox_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(10062, &pos);
    draw_sprite_ary(note_itembox_frame_tbl, &pos);
    note_itembox_row_draw(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 10061, 0);
    eft052_box_list_draw(10107, note_trade_menu_sprite_tbl, note_trade_menu_row_tbl, 9319, 0);
}

/* 0x803A7208 (0xA4): draws the route view (or, when the box is up, the money and the box). */
extern "C" void note_trade_route_draw(LbNoteTradeWork* w) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;

    switch ((u32)w->sub_0x01) {
    case 0:
    case 1:
        get_lsp_data(9929, &base);
        get_lsp_data(9948, &pos);
        pos.x += base.x;
        pos.y += base.y;
        draw_sprite_ary(note_route_tbl, &pos);
        note_trade_slots_draw(w, 0);
        break;
    case 2:
        menu_money_draw(5372);
        note_itembox_draw();
        break;
    }
}

/* 0x803A72AC (0x150): draws route page `offer_cursor`'s six rewards (the ones won shown, the rest hidden) and the
 * cursor's item row. */
extern "C" void note_trade_reward_draw(LbNoteTradeWork* w) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 row[2];
    u16* items;
    u16 i;
    u16 item;
    s32 sel;

    get_lsp_data(10028, &base);
    draw_sprite_anim_ary(note_reward_back_tbl, w->offer_cursor_0x3A, &base);
    lb_page_arrow_draw(10026, w->offer_cursor_0x3A, w->slot_count_0x36, w->moved_0x3C, 1);
    items = note_slot_table[w->offer_cursor_0x3A];
    for (i = 0; i < 6; i++) {
        get_lsp_data(note_reward_row_tbl[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        if ((((Q_UserData*)lobby_world_block)->note_rewards_0x5288 & (1 << (i + w->offer_cursor_0x3A * 6))) != 0) {
            item = items[i];
        } else {
            item = 0;
        }
        if (i == w->reward_cursor_0x2C) {
            sel = 1;
            row[0] = item;
            row[1] = 1;
            menu_item_row_draw_by_lsp(10059, row);
        } else {
            sel = 0;
        }
        lb_window_draw(item, 0, &pos, 1, sel, 1, 0x80);
    }
}

/* 0x803A73FC (0xF4): draws the voyage line while the ship is out: the route and the voyages left. */
extern "C" void note_trade_voyage_draw(LbNoteTradeWork* w) {
    _mh_ivec2_ pos;
    s8 text[32];
    s8* tail;

    if (w->timer_0xD0->voyage_0x06 != 0) {
        get_lsp_data(9831, &pos);
        draw_sprite_anim_ary(note_voyage_route_tbl, w->timer_0xD0->slot_0x00, &pos);
        draw_font_idx(9852, (s8*)LbStr(0, 493), 1, &pos);
        tail = (s8*)LbStr(0, 14);
        sprintf((char*)text, "%s%s", (s8*)LbStr(0, 13), tail);
        sprintf((char*)text, (char*)text, w->timer_0xD0->voyage_0x06);
        draw_font_idx(9853, text, 0, &pos);
    }
}

/* 0x803A74F0 (0xE8): draws the note trade: the frame, the state's view and the help line. */
extern "C" void note_trade_draw(void) {
    LbNoteTradeWork* w = lobby_w.note_trade_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    note_trade_frame_draw();
    switch (w->state_0x00) {
    case 1:
        lb_choice_draw(&w->menu_0x04, 9816, note_trade_menu_sprite_tbl, note_trade_menu_row_tbl, 6263, 0);
        note_trade_voyage_draw(w);
        break;
    case 2:
        note_trade_select_draw(w);
        break;
    case 3:
        note_trade_slots_draw(w, 0);
        break;
    case 4:
        note_trade_route_draw(w);
        break;
    case 5:
        note_trade_reward_draw(w);
        break;
    }
    note_trade_help_draw(w);
}

/* The per-route roll of the trade's bonus kind: {weight, kind} pairs, the first row for one open route, the second
 * for more. */
u8 note_bonus_kind_tbl[2][5][2] = {
    {{60, 0}, {20, 1}, {10, 2}, {0, 3}, {10, 4}},
    {{50, 0}, {20, 1}, {10, 2}, {10, 3}, {10, 4}},
};

/* The note trade's sprite tables: frame and title, the routes' rows, gauges, cursor and full marks, the captain's box,
 * the offer sheet, the item box and the reward page (each 0xFFFF-terminated where the draw walks it). */
u16 note_title_tbl[16] = {0x2640, 0x2641, 0x2642, 0x2643, 0x2644, 0x2645, 0x2646, 0x2647,
                          0x2648, 0x2649, 0x264A, 0x264B, 0x264C, 0x263E, 0x263F, 0xFFFF};
u16 note_frame_tbl[5] = {0x2650, 0x2651, 0x264E, 0x264F, 0xFFFF};
u16 note_frame_wide_tbl[5] = {0x2655, 0x2656, 0x2653, 0x2654, 0xFFFF};
u16 note_full_text_tbl[7] = {0x2688, 0x2689, 0x268A, 0x268B, 0x268C, 0x268D, 0xFFFF};
u16 note_slot_back_tbl[17] = {0x26AE, 0x26AF, 0x26B0, 0x26B1, 0x26B2, 0x26B3, 0x26B4, 0x26B5, 0x26B6,
                              0x26AB, 0x26AC, 0x26AD, 0x26A1, 0x26A2, 0x26A3, 0x26A4, 0xFFFF};
u16 note_slot_tbl[10] = {0x26A7, 0x26A8, 0x26A9, 0x26AA, 0x26A6, 0x2699, 0x269A, 0x269B, 0x269C, 0xFFFF};
u16 note_slot_cursor_tbl[10] = {0x26BF, 0x26B7, 0x26B8, 0x26B9, 0x26BA, 0x26BB, 0x26BC, 0x26BD, 0x26BE, 0xFFFF};
u16 note_slots_back_tbl[11] = {0x26D1, 0x26D2, 0x26D3, 0x26CF, 0x26D0, 0x26CA, 0x26CB, 0x26CC, 0x26CD, 0x26CE, 0xFFFF};
u16 note_bonus_tbl[6] = {0x26E1, 0x26E2, 0x26E3, 0x26E4, 0x26E5, 0xFFFF};
u16 note_offer_back_tbl[12] = {0x271E, 0x271F, 0x2720, 0x2721, 0x2722, 0x2723,
                               0x2724, 0x2725, 0x2726, 0x2727, 0x2728, 0xFFFF};
u16 note_offer_arrow_tbl[6] = {0x26F1, 0x26EF, 0x26F0, 0x26ED, 0x26EE, 0xFFFF};
u16 note_offer_goods_tbl[10] = {0x26F5, 0x26F6, 0x26F7, 0x26F8, 0x26F9, 0x26FA, 0x26FB, 0x26FC, 0x26FD, 0xFFFF};
u16 note_offer_voyage_tbl[13] = {0x2714, 0x2715, 0x2716, 0x2717, 0x2718, 0x2719, 0x271A,
                                 0x271B, 0x271C, 0x2712, 0x2713, 0x2711, 0xFFFF};
u16 note_itembox_cell_tbl[11] = {0x2769, 0x276A, 0x276B, 0x276C, 0x276D, 0x276E, 0x276F, 0x2770, 0x2771, 0x2772, 0xFFFF};
u16 note_itembox_back_tbl[7] = {0x2774, 0x2775, 0x2776, 0x2777, 0x2778, 0x2779, 0xFFFF};
u16 note_trade_menu_sprite_tbl[12] = {0x2661, 0x2662, 0x2663, 0x265D, 0x265E, 0x2659,
                                      0x265A, 0x265B, 0x265C, 0x265F, 0x2660, 0xFFFF};
u16 note_itembox_frame_tbl[26] = {0x2766, 0x2767, 0x2763, 0x2764, 0x2765, 0x2752, 0x2753, 0x2754, 0x2755,
                                  0x2756, 0x2757, 0x2758, 0x2759, 0x275A, 0x275B, 0x275C, 0x275D, 0x275E,
                                  0x275F, 0x2760, 0x2761, 0x2762, 0x274F, 0x2750, 0x2751, 0xFFFF};
u16 note_route_tbl[9] = {0x26C7, 0x26C8, 0x26C2, 0x26C3, 0x26C4, 0x26C5, 0x26C6, 0x26C1, 0xFFFF};
u16 note_reward_back_tbl[24] = {0x2743, 0x2741, 0x2742, 0x2740, 0x273E, 0x273F, 0x273A, 0x273B,
                                0x273C, 0x273D, 0x2736, 0x2737, 0x2738, 0x2739, 0x2732, 0x2733,
                                0x2734, 0x2735, 0x272E, 0x272F, 0x2730, 0x2731, 0x272D, 0xFFFF};
u16 note_reward_row_tbl[6] = {0x2745, 0x2746, 0x2747, 0x2748, 0x2749, 0x274A};
u16 note_voyage_route_tbl[25] = {0x2670, 0x2671, 0x2672, 0x2673, 0x2668, 0x2669, 0x266A, 0x266B, 0x266C,
                                 0x266D, 0x266E, 0x266F, 0x2674, 0x2675, 0x2676, 0x2677, 0x267F, 0x2680,
                                 0x2681, 0x2682, 0x2678, 0x2679, 0x267A, 0x267B, 0xFFFF};
u16 note_full_back_tbl[3] = {0x268F, 0x2690, 0xFFFF};
u16 note_full_tbl[3] = {0x2686, 0x2687, 0xFFFF};
u16 note_full_glow_tbl[3] = {0x2684, 0x2685, 0xFFFF};
u16 note_points_tbl[3] = {0x26A0, 0x269F, 0x269E};
u16 note_arrow_tbl[4] = {0x2694, 0x2695, 0x2693, 0xFFFF};
u16 note_slot_row_tbl[4] = {0x26D4, 0x26D5, 0x26D6, 0x26D7};
u16 note_full_row_tbl[4] = {0x26D8, 0x26D9, 0x26DA, 0x26DB};
u16 note_offer_kind_tbl[3] = {0x26EA, 0x26EB, 0xFFFF};
u16 note_offer_points_tbl[4] = {0x2708, 0x2709, 0x270A, 0xFFFF};
u16 note_trade_menu_row_tbl[3] = {0x2664, 0x2665, 0x2666};
