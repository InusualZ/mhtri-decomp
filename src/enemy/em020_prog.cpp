/*
 * enemy/em020_prog.cpp - enemy 020's program: the quest board's info-page draws at the head, the `_ENEMY_WORK`
 *   reset/init/action/motion handlers with their program table, and the lobby network tail (packet senders and
 *   receivers, the friend/recent/mail lists and the per-frame lobby update).
 * RANGE. .text 0x8036CF64-0x80378F9C (138 functions); extab 0x800178E4-0x80017C44, extabindex
 *   0x800373BC-0x800378CC, .rodata 0x80570A20-0x80570AE0, .data 0x805EE098-0x805EE518 (`em020_prog_tbl` first),
 *   .bss 0x806BF530-0x806C23E8, .sdata 0x807933B0-0x807933B8, .sbss 0x80794BF0-0x80794BF8, .sdata2
 *   0x8079B820-0x8079BC88.  Right edge: `em019_prog_tbl` opens the next `.data` block.  The range reads as three
 *   TUs: the head 0x8036CF64-0x8036E26C (quest info pages: calls only menu/hud code, owns no data), em020 proper
 *   0x8036E26C-0x8037583C (`.data` through the user-data table at 0x805EE3F8, `.sdata2` 0x8079B820-0x8079BC78)
 *   and the lobby tail 0x8037583C-0x80378F9C (a vtable-then-string data-order seam at 0x805EE428, `.bss`,
 *   `.sbss`, `.sdata`) - filed as a seam request, not acted on here.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi` + `cmpwi`, `slwi` +
 *   `clrlwi` and `srwi` + `clrlwi` pairs) and `#pragma fp_contract off` (separate `fmuls` + `fadds`); `#pragma
 *   dont_inline on/off` around the three packet constructors so MWCC emits them out of line after their first user.
 * NAMES. The file name follows the runtime dump's `em020_prog_tbl`, which opens the TU's `.data`; `getInstance_`
 *   is the dump's name.  GUESS (from each body, its callers and what it stores):
 *   em020_reset, em020_init, `Em020UserData`, em020_event_notify, em020_turn_back, em020_circle_charge,
 *   em020_blast, em020_walk_approach, em020_action5_dispatch, em020_slam, em020_leap, em020_dash, em020_tail_sweep,
 *   em020_spin, em020_burrow_attack_a, em020_burrow_attack_b, em020_pose_cb, em020_pose_cc, em020_pose_cd, em020_hop,
 *   em020_action7_dispatch, em020_action10_dispatch, em020_action11_play, em020_demo_intro, em020_demo_hold,
 *   em020_demo_burrow, em020_demo_emerge, em020_entry_area_a, em020_entry_area_b, em020_action13_dispatch,
 *   em020_action_dispatch, em020_glow_color_get, em020_color_step, em020_glow_update, em020_light_update,
 *   em020_drool_spawn, em020_motion_events, em020_status_flags_get, lobby_recent_has, lobby_friend_request_send,
 *   lobby_recent_add, lobby_friend_accept, lobby_recent_drop, assign__14LobbyRecentRowFPC14LobbyRecentRow,
 *   assign__12LobbyMailRowFPC12LobbyMailRow, lobby_friend_has, lobby_friend_list_full, lobby_friend_has_row,
 *   lobby_friend_remove, lobby_friend_drop, lobby_net_busy_ck, lobby_net_master_ck, lobby_net_profile_submit,
 *   lobby_net_position_send, __ct__19LbNetPositionPacketFv, lobby_net_action_send, lobby_net_action_update,
 *   lobby_net_action_recv, lobby_net_peer_load, lobby_net_peer_release, lobby_net_position_recv,
 *   __ct__16LbNetPlacePacketFv, __ct__16LbNetStatePacketFv, lobby_net_meal_recv, lobby_net_notice_recv,
 *   lobby_net_place_send, lobby_net_place_recv, lobby_net_profile_part1_send, lobby_net_card_apply,
 *   lobby_net_dress_recv, lobby_net_state_request_send, lobby_net_state_request_recv, lobby_net_state_send,
 *   lobby_net_state_recv, lobby_net_trade_send, lobby_net_trade_recv, lobby_net_item_request_send,
 *   lobby_net_item_request_recv, lobby_net_item_answer_send, lobby_net_item_answer_recv, lobby_net_join_notice_recv,
 *   lobby_net_flag_send, lobby_net_card_find, lobby_net_to_self_ck, lobby_net_from_self_ck, lobby_state_reset,
 *   lobby_net_leave, lobby_net_jump_msg_get, lobby_net_leave_select, lobby_net_update, quest_info_sheet_draw,
 *   quest_info_detail_draw, quest_info_summary_draw, quest_info_objective_draw, quest_info_page_open,
 *   quest_info_page_step, quest_info_page_draw, quest_info_name_draw, lobby_net_err_confirm, em020_glow_keys_demo,
 *   em020_se_rec_mot_a, em020_se_tbl_mot_a, em020_se_rec_mot_b, em020_se_tbl_mot_b, em020_se_rec_mot_c,
 *   em020_se_tbl_mot_c, em020_se_rec_act11_a, em020_se_rec_act11_b, em020_se_tbl_act11, em020_glow_keys_c8,
 *   em020_glow_keys_c9, em020_turn_seq_back, em020_turn_seq_charge_l, em020_turn_seq_charge_r
 *   Map rows already named (GUESS, from their bodies): the `em020_*` handlers the map carries, lobby_net_err_draw,
 *   em020_profile_send, releaseRemotePlayerParts, sendMemberJoinNotice, handleLobbyNetMessage, dropLobbyMail,
 *   addFriendNotice.  Foreign callees named by this unit are GUESS names too.
 * RESIDUALS.
 *  - `lobby_net_update`: 3240 B vs 3008 B; retail shares the error-window tails between cases (a tail merge no
 *    goto-free shape reproduced).
 *  - `em020_hp_ratio_ck`, `lobby_net_state_request_recv`, `em020_light_update`: retail exits through a `bne L; b end`
 *    pair the source shapes tried (inline helper, nested if, early return) do not produce.
 *  - `lobby_net_action_send`: retail lowers the kind switch as a compare chain in a different order.
 *  - `em020_burrow_attack_a/b`, `em020_demo_burrow`: the `shell_set_func_ptr` slot loads schedule one slot earlier.
 *  - `quest_info_detail_draw` (+8 B), `quest_info_sheet_draw` (case 5/6 compare order), `quest_info_name_draw`,
 *    `lobby_net_flag_send` (+4 B), `lobby_net_state_send` (-4 B), `assign__12LobbyMailRowFPC12LobbyMailRow`,
 *    `lobby_net_leave_select`, `em020_walk_approach`, `em020_arm_mot1_side_wait80` (`subfic` for retail's
 *    `cntlzw; extrwi; neg; addi`), `em020_motion_events`: register or scheduling residuals.
 *  - Data: `.sdata2` pool order follows first use and differs from retail at +0x454 (16 B short); `.sbss` ends at
 *    5 B where the target object carries a 3-byte tail.  `jump_err_msg_str` (`.sbss` 0x80794B10) is unclaimed and
 *    shared with `menu/menu_placeinfo.cpp` (rule 12 finding in `unsplit/unknown.h`, filed).
 *  - Signature casts: `em_get_part_limit`, `pl_parts_refresh`, `requestListRefreshAlias`, `userdata_item_slot_count`,
 *    `quest_grade_get` and `calcVecAngX` are called through a function-pointer cast where the owner's declaration
 *    differs from the retail call (decl requests filed).
 * SHAPES. `Em020UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/math.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "stage/stg_w.h"

#include "gx.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck (the C++ declaration, which mangles to the map's name) */
#include "enemy/fn_80138074.h" /* fn_8013A9F4, ResUserDataAc */
#include "MSL/new.h" /* the placement `operator new` */
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"     /* ScnMdl::CopiedMatAccess */
#include "g3d/g3d_resmat.h" /* ResTexSrt */
#include "fn_8004CAD8.h"         /* MTX34_ctor, fn_8005024C (their owner's header) */
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor (rule 2) */
#include "unsplit/sound.h"       /* fn_800E2994 */
#include "unsplit/unknown.h"     /* SystemWork / system_w */
#include "enemy/em020_ai.h"      /* em020_aim_target_ck (this unit's) */
#include "stage/stg_w.h"         /* fn_802B0A98, stage_map_kind_get (the owner is stage/stg_w.cpp) */
#include "ai/fn_802D44F4.h"      /* fn_802D94C4 (the owner is ai/ai_npc.cpp) */
#include "Pl/fn_8027D684.h"      /* fn_8027DC64 (the owner is Pl/pl_act.cpp) */

#include "enemy/em020_ai.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_80138074.h"
#include "Network/network_pat_control.h"
#include "sys_mem.h"
#include "enemy/em_common.h"
#include "enemy/em_pop.h"
#include "quest/quest_result_work.h"
#include "lobby/fn_8030121C.h"
#include "draw_shape.h"
#include "ef/eft007.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_hit_window_clear.h"
#include "enemy/em_camera_req.h"
#include "stage/shell_set_func_ptr.h"
#include "enemy/enemy_control.h"
#include "lobby/fn_801E0ADC.h"
#include "ef/fn_80105314.h"
#include "ef/eft026_pl_spawn.h"
#include "Pl/plw.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_npc.h"
#include "fn_80047398.h"
#include "ef/fn_800CDB2C.h"
#include "userdata_item.h"
#include "Network/net_session_close.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "ef/get_move_work_adrs.h"
#include "mh3_pad/lb_param_w.h"
#include "Pl/fn_802693C4.h"
#include "fn_80047398/lobby_world_block.h"
#include "main.h"
#include "font/flfnt.h"
#include "menu/menu_message.h"
#include "stage/get_now_mapno.h"
#include "MSL_C/printf.h"
#include "sound/set_zmode__FbUcb.h"
#include "Network/PatInterface.h"
#include "mh3_pad/Psw.h"
#include "menu/menu_item.h"
#include "ef/work_mem_free.h"
#include "ef/system_core.h"
#include "fn_80056F24.h"
#include "hud/get_lsp_data.h"
#include "hud/draw_sprite_ary.h"
#include "hud/draw_font_idx.h"
#include "lobby/spr_data_copy.h"
#include "quest/quest_item_slot.h"
#include "lobby/lb_quest_screen.h"
#include "quest/quest_record_find.h"
#include "light/light.h"
#include "lobby/lb_equip_page.h"
#include "menu/multi_result.h"
#include "ef/eft052.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "quest/quest_entry.h"
#include "hud/cockpit.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/draw_font.h"
#include "lobby/lb_quest_name_get.h"
#include "Network/submitUserProfile.h"
#include "Pl/parts_mdl_release_all.h"
#include "menu/get_pop_dat_ptr.h"

#include "enemy/em020_prog.h"
#include "enemy/lobby_state_block.h" /* the unit's `.bss` lobby state block (its leaf header) */
/* `lobby/lb_companion_ui.cpp`'s quest-page block (ten 0x130-byte records), from its leaf header. */
#include "lobby/lobby_hunter_cards.h"
#include "camera/camera_demo_event_set.h"
#include "Pl/pl_net_slot_bind.h"
#include "menu/pig_name_set.h"
#include "menu/quest_info_text_get.h"
#define pl_parts_refresh_c1 ((void (*)(_PLW*, s32))pl_parts_refresh)
#define eft026_pl_spawn_c1 ((void (*)(struct _EFT26_EM*, u8, nw4r::math::VEC3*, u32, f32))eft026_pl_spawn)
/* callees main declares with another signature: the calls go through these views (the same direct call). */
#define em_turn_to_target_3d_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32))em_turn_to_target_3d)
#define em_mot_layer_blend_set_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32, u32, u32, u32))em_mot_layer_blend_set)
#define em_combat_reset_c1 ((void (*)(struct _ENEMY_WORK*))em_combat_reset)
#define em_mat_blend_reset_c1 ((void (*)(struct _ENEMY_WORK*))em_mat_blend_reset)
#define em_mot_frame_get_c1 ((f32 (*)(struct _ENEMY_WORK*))em_mot_frame_get)
#define em_act_set_unless_act11_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32))em_act_set_unless_act11)
#define em_height_add_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_height_add)
#define em_height_set_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_height_set)
#define em_eft_fx_spawn_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32, f32, u32))em_eft_fx_spawn)
#define em_eft_fx_spawn_anchor_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32, f32))em_eft_fx_spawn_anchor)
#define mhchar_mat_blend_set_c1 ((void (*)(struct MHchar*, u32, f32))mhchar_mat_blend_set)

/* The user-data accessor `em020_init` installs: the base accessor with its own destructor and attach hook (its
 * table is `.data` 0x805EE3F8).  size: 0xC (`operator new(0xC)`) */
class Em020UserData : public ResUserDataAc {
public:
    Em020UserData();
    virtual ~Em020UserData();
    virtual void Attach();
};

/* This unit's own forward declarations. */
extern "C" {
void em020_reset(_ENEMY_WORK* self);
void em020_angle_step_to_zero(_ENEMY_WORK* self);
void em020_glow_color_get(_ENEMY_WORK* self, u8* r, u8* g, u8* b);
u8* em020_quest_page_ptr(void);
void lobby_notice_raise(LobbyTimerBlock* block, s32 mode);
void lobby_net_peer_load(_PLW* plw, const NetPeerRec* peer, u8 slot, u8 notify);
}
struct LbNetPacketHead;
struct LbNetItemPacket;
extern "C" {
void lobby_net_state_send(_PLW* plw, const LbNetPacketHead* request, u8 broadcast);
void lobby_net_item_answer_send(_PLW* plw, const LbNetItemPacket* request, u8 result);
}

#pragma peephole off
#pragma fp_contract off

extern "C" {

/* The quest board info page's work record as the page functions read it: the button words, the row the page sits on,
 * its mode and the page cursor.  size: 0x1B0 (the extent these bodies prove) */
struct QuestInfoPage {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ u16 pressed_0x004;   /* buttons that went down this frame */
    /* +0x006 */ u8 pad_0x006[0x2];
    /* +0x008 */ u16 held_0x008;      /* the held buttons */
    /* +0x00A */ u8 pad_0x00A[0x8];
    /* +0x012 */ u8 row_0x012;        /* the board row the page is anchored to */
    /* +0x013 */ u8 pad_0x013;
    /* +0x014 */ u8 mode_0x014;       /* the screen mode the page opens with */
    /* +0x015 */ u8 pad_0x015[0x1AE - 0x15];
    /* +0x1AE */ s8 page_0x1AE;       /* the page shown */
    /* +0x1AF */ s8 page_last_0x1AF;  /* the last page */
}; /* size: 0x1B0 */

/* 0x8036CF64 (0x744): draws the quest board's info sheet for board mode `mode`: the frame, the client and goal lines,
 * the grade, the two reward rows and the fee and time lines. */
void quest_info_sheet_draw(s32 mode)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ ofs;
    u8 grade = ((u8 (*)(void))quest_grade_get)();
    u8 done;
    u16* ids;

    get_lsp_data(0xA15, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xF9), &pos);
    if (mode == 5) {
        get_lsp_data(0xAA2, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xF9), &pos);
    }
    get_lsp_data(0xA27, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xFB), &pos);
    if ((u32)(mode - 4) > 2) {
        get_lsp_data(0xA1D, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xFA), &pos);
    } else {
        get_lsp_data(0xA1D, &pos);
        draw_sprite_idx(0xA1E, &pos);
    }
    if (quest_info_paid_ck() == 1) {
        spr_data_copy(&spr, get_lsp_data(0xA21, NULL));
        spr.color = 0x878787FF;
        draw_font(spr, quest_info_text_get(mode, 0x10), 5, &pos);
        if (grade == 0) {
            spr_data_copy(&spr, get_lsp_data(0xA22, NULL));
            spr.color = 0x878787FF;
            draw_font(spr, quest_info_text_get(mode, 0x11), 5, &pos);
        }
        get_lsp_data(0xA27, &pos);
        draw_sprite_idx(0xA2C, &pos);
    } else {
        draw_font_idx(0xA21, quest_info_text_get(mode, 0x10), 5, &pos);
        if (grade == 0)
            draw_font_idx(0xA22, quest_info_text_get(mode, 0x11), 5, &pos);
        get_lsp_data(0xA27, &pos);
        draw_sprite_idx(0xA2B, &pos);
        draw_font_idx(0xA2D, (s8*)quest_result_field_text_cur_get(0x17), 1, &pos);
    }
    if (grade != 0) {
        u8** grades = get_str_tbl(0x3D);

        get_lsp_data(0xA37, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xFD), &pos);
        draw_font_idx(0xA3B, quest_info_text_get(mode, 0x11), 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0xA3C, NULL));
        if (grade != 4)
            spr.color = 0xC3C3C3FF;
        draw_font(spr, (s8*)*(grades + grade - 1), 1, &pos);
    }
    switch (mode) {
    case 4:
        break;
    default:
        get_lsp_data(0xA27, &pos);
        spr_data_copy(&spr, get_lsp_data(0xA25, NULL));
        ofs.x = pos.x + spr.pos.x;
        ofs.y = pos.y + spr.pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xFC), &ofs);
        if (quest_item_work_notify(1) == 1 && quest_flag_800000_ck(0) == 0) {
            spr_data_copy(&spr, get_lsp_data(0xA23, NULL));
            spr.color = 0x878787FF;
            draw_font(spr, quest_info_text_get(mode, 2), 5, &pos);
            draw_sprite_idx(0xA34, &ofs);
        } else {
            draw_font_idx(0xA23, quest_info_text_get(mode, 2), 5, &pos);
            draw_sprite_idx(0xA33, &ofs);
            draw_font_idx(0xA35, (s8*)quest_result_field_text_cur_get(0x18), 1, &ofs);
        }
        spr_data_copy(&spr, get_lsp_data(0xA26, NULL));
        ofs.x = pos.x + spr.pos.x;
        ofs.y = pos.y + spr.pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xFC), &ofs);
        if (quest_item_work_notify(2) == 1 && quest_flag_800000_ck(0) == 0) {
            spr_data_copy(&spr, get_lsp_data(0xA24, NULL));
            spr.color = 0x878787FF;
            draw_font(spr, quest_info_text_get(mode, 3), 5, &pos);
            draw_sprite_idx(0xA34, &ofs);
        } else {
            draw_font_idx(0xA24, quest_info_text_get(mode, 3), 5, &pos);
            draw_sprite_idx(0xA33, &ofs);
            draw_font_idx(0xA35, (s8*)quest_result_field_text_cur_get(0x19), 1, &ofs);
        }
        break;
    case 6:
        get_lsp_data(0xA27, &pos);
        pos.x += 0x12;
        pos.y += 0x14;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x17B), &pos);
        draw_font_idx(0xF40, quest_info_text_get(mode, 0x1A), 1, &pos);
        break;
    case 5:
        get_lsp_data(0xAA3, &pos);
        draw_sprite_idx(0xAA5, &pos);
        get_lsp_data(0xAAD, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x108), &pos);
        draw_sprite_idx(0xAA6, &pos);
        draw_font_idx(0xAA4, (s8*)quest_result_field_text_cur_get(0x1A), 1, &pos);
        get_lsp_data(0xAAE, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x108), &pos);
        draw_sprite_idx(0xAA7, &pos);
        draw_font_idx(0xAA4, (s8*)quest_result_field_text_cur_get(0x1B), 1, &pos);
        get_lsp_data(0xAAF, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x108), &pos);
        draw_sprite_idx(0xAA8, &pos);
        draw_font_idx(0xAA4, (s8*)quest_result_field_text_cur_get(0x1C), 1, &pos);
        break;
    }
    done = quest_sub_state_end_ck(1);
    get_lsp_data(0xA3E, &pos);
    if (done == 0)
        ids = (u16*)get_menu_lsp_tbl(0xFE);
    else
        ids = (u16*)get_menu_lsp_tbl(0xFF);
    draw_sprite_ary(ids, &pos);
    draw_font_idx(0xA52, quest_info_text_get(mode, 0x14), 1, &pos);
    draw_font_idx(0xA53, quest_info_text_get(mode, 0x13), 1, &pos);
    if (done == 0) {
        draw_font_idx(0xA54, quest_info_text_get(mode, 0x12), 1, &pos);
        return;
    }
    spr_data_copy(&spr, get_lsp_data(0xA55, NULL));
    spr.color = 0x8C8C8CFF;
    draw_font(spr, quest_info_text_get(mode, 0x15), 1, &pos);
    draw_font_idx(0xA56, quest_info_text_get(mode, 0x12), 1, &pos);
}

/* 0x8036D6A8 (0x370): draws the quest board's detail block: the target and condition lines, with the reward row
 * hidden for the special modes and the arena quests. */
void quest_info_detail_draw(s32 mode)
{
    _mh_ivec2_ pos;
    s32 arena = 0;
    u16 id;
    u8 sel;

    if (mode == 2) {
        if (quest_flag_2000000_ck(quest_record_find(lb_param_w.field_0x00)) == 1)
            arena = 1;
    }
    if ((u32)(mode - 4) > 2 && arena == 0) {
        get_lsp_data(0xA15, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xF9), &pos);
    }
    if ((u32)(mode - 4) > 2 && arena == 0) {
        get_lsp_data(0xA57, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x100), &pos);
        draw_font_idx(0xA61, quest_info_text_get(mode, 0x10), 5, &pos);
        draw_font_idx(0xA62, quest_info_text_get(mode, 9), 0, &pos);
        draw_font_idx(0xA63, quest_info_text_get(mode, 0xA), 0, &pos);
        draw_font_idx(0xA64, quest_info_text_get(mode, 0xB), 0, &pos);
        draw_font_idx(0xA65, quest_info_text_get(mode, 0xC), 0, &pos);
        draw_font_idx(0xA66, quest_info_text_get(mode, 6), 0, &pos);
        sel = quest_info_reward_ck(mode);
        id = 0xA68;
        if (sel == 0)
            id = 0xA67;
        draw_sprite_idx(id, &pos);
    } else {
        get_lsp_data(0xA57, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x101), &pos);
        draw_font_idx(0xA61, quest_info_text_get(mode, 0x10), 5, &pos);
        draw_font_idx(0xA69, quest_info_text_get(mode, 0x11), 5, &pos);
        draw_font_idx(0xA70, quest_info_text_get(mode, 9), 0, &pos);
        draw_font_idx(0xA71, quest_info_text_get(mode, 0xA), 0, &pos);
        draw_font_idx(0xA72, quest_info_text_get(mode, 0xB), 0, &pos);
        draw_font_idx(0xA73, quest_info_text_get(mode, 0xC), 0, &pos);
        draw_font_idx(0xA74, quest_info_text_get(mode, 6), 0, &pos);
        sel = quest_info_reward_ck(mode);
        id = 0xA76;
        if (sel == 0)
            id = 0xA75;
        draw_sprite_idx(id, &pos);
    }
    if (arena == 0 && (u32)(mode - 4) > 2) {
        get_lsp_data(0xA77, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x102), &pos);
        draw_font_idx(0xA80, quest_info_text_get(mode, 2), 0, &pos);
        draw_font_idx(0xA81, quest_info_text_get(mode, 3), 0, &pos);
        draw_font_idx(0xA82, quest_info_text_get(mode, 0xD), 2, &pos);
        draw_font_idx(0xA83, quest_info_text_get(mode, 0xE), 2, &pos);
    }
}

/* 0x8036DA18 (0x168): draws the quest board's summary block: the client lines, the two map lines and the two
 * monster lines. */
void quest_info_summary_draw(s32 mode)
{
    _mh_ivec2_ pos;

    get_lsp_data(0xA84, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x103), &pos);
    draw_font_idx(0xA88, quest_info_text_get(mode, 0x10), 5, &pos);
    if ((u8)quest_grade_get() == 0)
        draw_font_idx(0xA89, quest_info_text_get(mode, 0x11), 5, &pos);
    get_lsp_data(0xA8A, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x104), &pos);
    draw_font_idx(0xA8D, quest_info_text_get(mode, 0x1E), 0, &pos);
    draw_font_idx(0xA8E, quest_info_text_get(mode, 0x1F), 0, &pos);
    get_lsp_data(0xA8F, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x105), &pos);
    draw_font_idx(0xA96, quest_info_text_get(mode, 4), 0, &pos);
    draw_font_idx(0xA97, quest_info_text_get(mode, 5), 0, &pos);
    get_lsp_data(0xAA0, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xF9), &pos);
}

/* 0x8036DB80 (0x150): draws the quest board's objective block, animating the client lines for objective kind 3. */
void quest_info_objective_draw(s32 mode)
{
    _mh_ivec2_ pos;
    u16 anim = quest_objective_get(0) == 3;

    get_lsp_data(0xA84, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x103), &pos);
    draw_font_anim_idx(0xA88, anim, quest_info_text_get(mode, 0x10), 5, &pos);
    if ((u8)quest_grade_get() == 0)
        draw_font_anim_idx(0xA89, anim, quest_info_text_get(mode, 0x11), 5, &pos);
    get_lsp_data(0xA98, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x106), &pos);
    draw_font_idx(0xA9B, quest_info_text_get(mode, 7), 0, &pos);
    get_lsp_data(0xA9C, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x107), &pos);
    draw_font_idx(0xA9F, quest_info_text_get(mode, 8), 0, &pos);
    get_lsp_data(0xAA1, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xF9), &pos);
}

/* 0x8036DCD0 (0x64): opens the quest info page: mode 10, the first page, and two pages for the board modes 2..6. */
void quest_info_page_open(QuestInfoPage* page)
{
    s32 mode;

    page->mode_0x014 = 0xA;
    page->page_0x1AE = 0;
    mode = quest_info_mode_get();
    if ((u32)(mode - 2) > 4) {
        if (mode == 1)
            page->page_last_0x1AF = 1;
    } else {
        page->page_last_0x1AF = 2;
    }
}

/* 0x8036DD34 (0x98): steps the quest info page: 2 when the page is left, else the page cursor follows the arrows. */
s32 quest_info_page_step(QuestInfoPage* page)
{
    s32 result = 0;
    u16 buttons = page->pressed_0x004 | (page->held_0x008 & 0xF);

    if ((buttons & 0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if ((buttons & 0xC) != 0) {
        page->page_0x1AE = menu_cursor_step(page->page_0x1AE, page->page_last_0x1AF, buttons, 4, 8);
    }
    return result;
}

/* 0x8036DDCC (0x408): draws the quest info page anchored to the board row: the frame, the page arrows and the page's
 * blocks (the client, fee and time lines on page 0, the map lines on page 1). */
void quest_info_page_draw(QuestInfoPage* page)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ base;
    s32 mode = quest_info_mode_get();
    u8 done;
    u16* ids;

    get_lsp_data(0xF13, &base);
    get_lsp_data(((u16*)get_menu_lsp_tbl(0x175))[page->row_0x012], &pos);
    base.x = pos.x - base.x;
    base.y = pos.y - base.y;
    put_lsp_anchor_offset(0xF13, &pos, base);
    draw_sprite_idx(0xF1E, &pos);
    if (page->page_0x1AE == 0 && mode != 1)
        draw_sprite_idx(0xF28, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x176), &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(0x177), page->page_0x1AE, page->page_last_0x1AF, page->held_0x008, &pos, 0);
    if (mode == 1) {
        put_lsp_anchor_offset(0xF31, &pos, base);
        draw_font_idx(0xF36, (s8*)*get_str_tbl(0x2B), 5, &pos);
        return;
    }
    put_lsp_anchor_offset(0xF2D, &pos, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x179), &pos);
    draw_font_idx(0xF30, quest_info_text_get(mode, 0), 5, &pos);
    switch (page->page_0x1AE) {
    case 0:
        put_lsp_anchor_offset(0xF31, &pos, base);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x17A), &pos);
        draw_font_idx(0xF35, quest_info_text_get(mode, 0x10), 1, &pos);
        draw_font_idx(0xF36, quest_info_text_get(mode, 0x11), 1, &pos);
        if (quest_info_paid_ck() == 1) {
            draw_sprite_idx(0xF34, &pos);
        } else {
            draw_sprite_idx(0xF33, &pos);
            draw_font_idx(0xF37, (s8*)quest_result_field_text_cur_get(0x17), 1, &pos);
        }
        put_lsp_anchor_offset(0xF3C, &pos, base);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x17B), &pos);
        draw_font_idx(0xF40, quest_info_text_get(mode, 0x1A), 1, &pos);
        done = quest_sub_state_end_ck(1);
        put_lsp_anchor_offset(0xF43, &pos, base);
        if (done == 0)
            ids = (u16*)get_menu_lsp_tbl(0x17C);
        else
            ids = (u16*)get_menu_lsp_tbl(0x17D);
        draw_sprite_ary(ids, &pos);
        draw_font_idx(0xF52, quest_info_text_get(mode, 0x14), 1, &pos);
        if (done == 0) {
            draw_font_idx(0xF53, quest_info_text_get(mode, 0x12), 1, &pos);
            return;
        }
        spr_data_copy(&spr, get_lsp_data(0xF54, NULL));
        spr.color = 0x8C8C8CFF;
        draw_font(spr, quest_info_text_get(mode, 0x15), 1, &pos);
        draw_font_idx(0xF55, quest_info_text_get(mode, 0x12), 1, &pos);
        return;
    case 1:
        put_lsp_anchor_offset(0xF56, &pos, base);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x17E), &pos);
        draw_font_idx(0xF5D, quest_info_text_get(mode, 4), 0, &pos);
        draw_font_idx(0xF5E, quest_info_text_get(mode, 5), 0, &pos);
        return;
    }
}

/* 0x8036E1D4 (0x98): prints the posted quest's name and message `index` at the window `pos`. */
void quest_info_name_draw(u8 index, const _mh_ivec2_* pos)
{
    s16 y;
    s16 x;

    x = pos->x;
    y = pos->y;
    font_set_size(0x16, 0x16);
    flfntSetColor(-1);
    font_locate(x + 0x22, y + 0x2A);
    font_print((s8*)lb_quest_name_get());
    font_locate(x + 0x18, y + 0x62);
    font_print((s8*)lb_quest_msg_get(index));
}

/* Steps the +0x1BC angle (an `s16` in the low halfword) 0x40 toward zero, snapping inside +/-0x40, and
 * hands the record on. */
void em020_angle_step_to_zero(_ENEMY_WORK* self)
{
    u32 ang = self->field_0x1BC;
    u16 half = (u16)ang;

    if (half < 0x8000) {
        if (half < 0x40)
            self->field_0x1BC = 0;
        else
            self->field_0x1BC = ang - 0x40;
    } else if (half > 0xFFC0) {
        self->field_0x1BC = 0;
    } else {
        self->field_0x1BC = ang + 0x40;
    }

    em_hit_arm_set(self);
}

void em020_init(_ENEMY_WORK* self);
void em020_timers_tick(_ENEMY_WORK* self);
void em020_action_dispatch(_ENEMY_WORK* self);
void em020_nop(_ENEMY_WORK* self);
void em020_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub);
void em020_motion_events(_ENEMY_WORK* self);
void em020_model_refresh(_ENEMY_WORK* self);
u8 em020_condition_ck(_ENEMY_WORK* self, u32 mode);
void em020_area_model_set(_ENEMY_WORK* self);
u32 em020_hp_ratio_ck(_ENEMY_WORK* self);
void em020_map_area_action_set(_ENEMY_WORK* self);

/* em020's program table (`.data` 0x805EE098), the slots the enemy driver calls: reset, init, the frame tick, the
 * action dispatch, the event notify, the motion events and the model, condition, area and HP hooks.
 * size: 0x70 */
struct Em020ProgTbl {
    /* +0x00 */ void (*reset_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*idle_0x10)(_ENEMY_WORK* self);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u8 unused_0x1C[0x20 - 0x1C];
    /* +0x20 */ void (*model_refresh_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u8 (*condition_ck_0x24)(_ENEMY_WORK* self, u32 mode);
    /* +0x28 */ u8 unused_0x28[0x34 - 0x28];
    /* +0x34 */ void (*area_model_set_0x34)(_ENEMY_WORK* self);
    /* +0x38 */ u8 unused_0x38[0x3C - 0x38];
    /* +0x3C */ u32 (*hp_ratio_ck_0x3C)(_ENEMY_WORK* self);
    /* +0x40 */ u8 unused_0x40[0x58 - 0x40];
    /* +0x58 */ void (*map_area_action_set_0x58)(_ENEMY_WORK* self);
    /* +0x5C */ u8 unused_0x5C[0x70 - 0x5C];
}; /* size: 0x70 */

Em020ProgTbl em020_prog_tbl = {
    em020_reset,
    em020_init,
    em020_timers_tick,
    em020_action_dispatch,
    em020_nop,
    em020_event_notify,
    em020_motion_events,
    {0},
    em020_model_refresh,
    em020_condition_ck,
    {0},
    em020_area_model_set,
    {0},
    em020_hp_ratio_ck,
    {0},
    em020_map_area_action_set,
    {0},
};

/* The lobby state block (`.bss` 0x806BF530). */
LobbyTimerBlock lobby_state_block;
/* `.sbss` 0x80794BF0: the message id of the last failed network layer command. */
s32 net_layer_error_message;
/* `.sbss` 0x80794BF4: the error-confirm flag `em020_unknown_flag_set` writes. */
s8 lobby_net_err_confirm;

/* The circling turn sequence `em020_turn_back` walks (`.rodata` 0x80570A20) and the two charge sequences
 * `em020_circle_charge` picks by direction (0x80570A60 / 0x80570AA0): a four-word header (motion, frames, blend,
 * the turn word) and three 0x10-byte steps.  The words are kept as they stand; only the walker gives them meaning. */
const u32 em020_turn_seq_back[16] = {
    0x00000014, 0x00000060, 0x00000000, 0x80002AAB,
    0x00380000, 0x00000014, 0x00000000, 0x00000001,
    0x00330000, 0x00000014, 0x00000000, 0x00000001,
    0x00340000, 0x00000014, 0x00000000, 0x00000001,
};

const u32 em020_turn_seq_charge_l[16] = {
    0x00000000, 0x000000B4, 0x00000000, 0xA0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00D00000, 0x00000028, 0x00000050, 0x00000001,
    0x00D00000, 0x00000028, 0x00000050, 0x00000001,
};

const u32 em020_turn_seq_charge_r[16] = {
    0x00000000, 0x000000B4, 0x00000000, 0xA0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00CF0000, 0x00000028, 0x00000050, 0x00000001,
    0x00CF0000, 0x00000028, 0x00000050, 0x00000001,
};

/* The demo glow keys `em020_glow_update` reads at twice the demo frame (`.data` 0x805EE108): (frame, level) pairs
 * ending at frame -1. */
f32 em020_glow_keys_demo[54] = {
    0.0f,    0.0f,  492.0f,  0.0f,  537.0f,  0.45f, 642.0f,  1.07f, 657.0f,  1.1f,  672.0f,  1.07f,
    694.0f,  0.93f, 707.0f,  0.9f,  721.0f,  0.93f, 744.0f,  1.12f, 758.0f,  1.15f, 772.0f,  1.12f,
    796.0f,  0.94f, 820.0f,  0.91f, 844.0f,  1.17f, 858.0f,  1.2f,  872.0f,  1.17f, 894.0f,  0.94f,
    912.0f,  0.91f, 928.0f,  0.94f, 942.0f,  1.2f,  1121.0f, 1.2f,  1170.0f, 0.43f, 1216.0f, 0.92f,
    1254.0f, 1.0f,  1290.0f, 1.0f,  -1.0f,   1.0f,
};

/* 0x8036E2BC (0x64): the program's reset slot: clears the timers and the latch, takes the stage light's value and
 * picks the K-colour triple. */
void em020_reset(_ENEMY_WORK* self)
{
    self->em020_0x328.timer_0x334 = 0;
    self->em020_0x328.value_0x336 = light_id_next();
    self->em020_0x328.latch_0x338 = 0;
    self->em020_0x328.flag_0x33C = 0;
    em020_glow_color_get(self, &self->em020_0x328.sel_0x339, &self->em020_0x328.sel_0x33A, &self->em020_0x328.sel_0x33B);
    self->em020_0x328.timer_0x33E = 900;
}

/* 0x8036E320 (0x214): the program's init slot: installs the user-data helper, arms the idle action and, on a live
 * quest stat, starts in area 3 with the stat's hit points and part flags. */
void em020_init(_ENEMY_WORK* self)
{
    Q_QuestStat* stat;
    s32 hp;

    if (em_res_user_data_ck(self) == 0) {
        Em020UserData* rec = (Em020UserData*)operator new(sizeof(Em020UserData));

        new (rec) Em020UserData;
        em_res_user_data_set(self, rec);
    }
    em_move_mode_set(self, 2);
    em_act_arm_unless_down(self, 0, 4);
    self->field_0x7B0 = 0.75f;
    stat = quest_stat_get();
    if (stat != NULL && stat->valid_0x00 == 1) {
        self->area_no = 3;
        em_act_arm_unless_down(self, 13, 1);
        if (quest_element_state_find(self->team) == 0x404)
            hp = 0.6f * (s32)self->field_0x7A4;
        else
            hp = 0.25f * (s32)self->field_0x7A4;
        if ((s32)stat->word_0x04 < hp)
            self->field_0x7A0 = hp;
        else
            self->field_0x7A0 = stat->word_0x04;
        self->field_0x7AC = self->field_0x7A0;
        if ((stat->byte_0x01 & 1) != 0) {
            em_part_hit_set(self, 0, 4);
            self->parts_0x838[0].field_0x01 = 1;
        } else {
            self->parts_0x838[0].value_0x04 = stat->half_0x02;
        }
        if ((stat->byte_0x01 & 2) != 0) {
            self->field_0x7B0 = -1.0f;
            self->flags_0x836 |= 0x8000;
        }
        if ((stat->byte_0x01 & 4) != 0)
            self->parts_0x838[3].damage_level = 2;
        if ((stat->byte_0x01 & 8) != 0)
            self->parts_0x838[5].damage_level = 1;
        fn_802B0A98(6, 0);
        fn_802B0A98(7, 0);
        fn_802B0A98(9, 0);
        fn_802B0A98(10, 0);
        fn_802D94C4(1);
    }
}

} /* extern "C" */

/* 0x8036E534 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em020UserData::Em020UserData()
{
}

extern "C" {

/* One handler slot of `em020_prog_tbl` that does nothing - the table's index 4. */
void em020_nop(_ENEMY_WORK* self)
{
    (void)self;
}

/* 0x8036E574 (0x144): the program's event slot: arms the 0x189C countdown on event (5, 0x17), spawns the
 * shock wave on (10, 0xCB), breaks the head on (10, 0xF7) and finishes the action on (13, 3). */
void em020_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (self->area_no == 0)
        self->field_0x38C = 0;
    switch (kind) {
    case 5:
        switch (sub) {
        case 0x17:
            self->em020_0x328.timer_0x334 = 0x189C;
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 0xCB:
            em_part_hit_set(self, 0, 0);
            setVector3(&pos, -200.0f, 0.0f, 150.0f);
            eft_em_spawn_tinted(self, 0, 0x17, &pos, 2.0f, 0xC2BF83FF);
            break;
        case 0xF7:
            self->field_0x7B0 = -1.0f;
            em_parts_break(self, 1);
            self->flags_0x836 |= 0x8000;
            eft_em_spawn(self, 0xB7, 0x14, NULL, 1.0f);
            break;
        }
        break;
    case 13:
        switch (sub) {
        case 3:
            em_combat_reset_c1(self);
            break;
        }
        break;
    }
}

/* Ticks the +0x334 countdown, and +0x33E in area 2 at stack depth 6; +0x338 latches "map 7, area 3"
 * each frame. */
void em020_timers_tick(_ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3)
        self->em020_0x328.latch_0x338 = 1;
    else
        self->em020_0x328.latch_0x338 = 0;

    if (self->em020_0x328.timer_0x334 > 0)
        self->em020_0x328.timer_0x334 -= 1;

    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 2 && self->stack_0x961[0] == 6) {
        if (self->em020_0x328.timer_0x33E > 0)
            self->em020_0x328.timer_0x33E -= 1;
    }
}

/* The two-step wake-up: tick, step the angle, then arm the 0x14-frame pose or wait for the motion and
 * release the record. */
void em020_arm_mot1_wait20(_ENEMY_WORK* self)
{
    em_hit_arm_set(self);
    em020_angle_step_to_zero(self);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 1, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* The per-frame byte the +0x1E6 sub-state makes `em020_arm_mot1_wait20` run for each of its five steps, with
 * every other sub-state falling straight through. */
void em020_substate_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot1_wait20(self);
        break;
    case 1:
        em020_arm_mot1_wait20(self);
        break;
    case 2:
        em020_arm_mot1_wait20(self);
        break;
    case 4:
        em020_arm_mot1_wait20(self);
        break;
    case 5:
        em020_arm_mot1_wait20(self);
        break;
    default:
        break;
    }
}

/* The same two-step wake-up for the second half of the pose: step 0 arms a 0x32/0x28-frame motion
 * (kind 3), step 1 waits for it and releases the record. */
void em020_arm_mot3_wait50(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x28, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* 0x8036E8C4 (0x174): the back-turn sequence; with `push` set the record is also pushed 5 units away from the
 * target for 0x28 frames. */
void em020_turn_back(_ENEMY_WORK* self, u8 push)
{
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em020_turn_seq_back, 0, 1, 0);
        em_move_vec_clr(self);
        if (push == 1) {
            s32 angle = calcVecAng2(&self->pos, &self->vec_0x36C);

            setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, -5.0f);
            rotVecY(&self->offset_0x30C.vec_0x310, angle);
            self->timer_0x020 = 0x28;
        }
        break;
    case 1:
        if (push == 1 && self->state_0x006 == 0) {
            em_move_offset_apply(self);
            if (--self->timer_0x020 <= 0)
                self->state_0x006 += 1;
        }
        if (em_turn_seq_step(self, (void*)em020_turn_seq_back) == 1) {
            em_action_finish_swim(self);
        } else {
            if (self->field_0x482 != 0)
                em_height_follow(self, 5.0f);
            else
                em_height_follow(self, 10.0f);
            em_turn_to_target_3d_c1(self, 0x40, 0);
        }
        break;
    }
}

/* The three-step wake-up: arm 0x37/0x14 and re-aim (the argument picks the +0x378 approach float), arm the
 * closing 0x2f/0x28 pose after the 0x80-frame window, then release. */
void em020_arm_approach_mot1_wait55(_ENEMY_WORK* self, u8 mode)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);

        switch (mode) {
        default:
            em_approach_start(self, 0.0f, 0x12);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 1400.0f;
            break;
        case 2:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 3000.0f;
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && em_mot_frame_started_ck(self) == 0) {
            self->state += 1;
            em_mot_set_blend(self, 0x2f, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* The 0x3a/0x0a-frame pose's two-step wake-up. */
void em020_arm_mot1_wait58(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3a, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* 0x8036EBF4 (0x5E0): the circling charge: a turn sequence (`dir` picks the side), then the run around the
 * target until the window or the 1000-frame budget ends, then the closing motion; `mode` picks the variant. */
void em020_circle_charge(_ENEMY_WORK* self, u8 dir, u8 mode)
{
    nw4r::math::VEC3* home = &self->em020_0x328.home_0x328;
    s32 step;
    u32 done;

    if (isServerSelectState() == 0)
        em_target_pos_set(self, 0);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        if (mode != 0) {
            step = 0x1800;
            em_approach_start(self, 0.0f, 0x1A);
        } else {
            step = 0x3000;
            copyVec3(home, &self->vec_0x36C);
            self->vec_0x36C.y += 2000.0f;
            if (self->vec_0x36C.y > self->field_0x210)
                self->vec_0x36C.y = self->field_0x210;
        }
        if (dir == 0)
            em_turn_seq_start(self, (void*)em020_turn_seq_charge_l, 0, 0x45, (u16)(0x10000 - step));
        else
            em_turn_seq_start(self, (void*)em020_turn_seq_charge_r, 0, 0x43, step);
        break;
    case 1:
        if (dir == 0)
            done = em_turn_seq_step(self, (void*)em020_turn_seq_charge_l);
        else
            done = em_turn_seq_step(self, (void*)em020_turn_seq_charge_r);
        if (mode != 0)
            em_approach_step(self, 0, 0);
        if (done == 1) {
            self->state += 1;
            self->state_0x006 = 0;
            self->timer_0x020 = 1000;
            if (dir == 0)
                em_mot_set_blend(self, 0x29, 0x30, 0, 1);
            else
                em_mot_set_blend(self, 0x2C, 0x30, 0, 1);
        }
        break;
    case 2:
        switch (mode) {
        case 0:
            if (em_frame_check(self, 3, 160.0f, 260.0f) == 1)
                em_turn_to_target_3d_c1(self, 0x40, 0x80);
            else
                em_turn_to_target_3d_c1(self, 0x40, 0x40);
            em_hit_arm_set(self);
            done = 0;
            break;
        default:
            done = em_approach_step(self, 0, 0x10);
            if (em_mot_frame_started_ck(self) == 1)
                done = 0;
            if (em_frame_check(self, 3, 160.0f, 260.0f) == 1)
                step = 0x50;
            step = step * get_em_base_scale(self);
            if (em_get_mot_no(self) == 0x29)
                self->field_0x1C0 += step;
            else
                self->field_0x1C0 -= step;
            break;
        case 3:
            if (em_frame_check(self, 3, 160.0f, 260.0f) == 1)
                step = 0x50;
            step = step * get_em_base_scale(self);
            if (em_get_mot_no(self) == 0x29)
                self->field_0x1C0 += step;
            else
                self->field_0x1C0 -= step;
            if (em_mot_end_ck(self) == 1)
                done = 1;
            break;
        }
        if (self->timer_0x020 > 0)
            self->timer_0x020 -= 1;
        if (done != 0 || self->timer_0x020 <= 0) {
            if (em_frame_check(self, 3, 100.0f, 400.0f) == 0) {
                self->state += 1;
                if ((u32)(mode - 2) > 1) {
                    self->state_0x007 = 1;
                    em_mot_set_blend(self, 0x2F, 0x3C, 0, 1);
                } else {
                    em_mot_set_blend(self, 0x23, 0x28, 0, 1);
                }
            }
        } else if (em_mot_end_ck(self) == 1) {
            if (mode == 0) {
                if (++self->state_0x006 >= 3) {
                    self->state += 1;
                    self->state_0x006 = 0;
                    em_mot_set_blend(self, 0x13, 0x28, 0, 1);
                    copyVec3(&self->vec_0x36C, home);
                    break;
                }
            }
            if (self->state_0x007 != 1) {
                if (em_get_mot_no(self) == 0x29)
                    em_mot_set_blend(self, 0x2C, 0x30, 0, 1);
                else
                    em_mot_set_blend(self, 0x29, 0x30, 0, 1);
            }
        }
        break;
    case 3:
        if (self->state_0x007 == 1) {
            if (em_mot_end_ck(self) == 1)
                em_action_finish_swim(self);
        } else {
            if (em_frame_check(self, 1, 40.0f, 0.0f) == 1)
                em_turn_to_target_3d_c1(self, 0x20, 0x100);
            em_hit_arm_set(self);
            if (mode != 0) {
                if (em_mot_end_ck(self) == 1) {
                    em_mot_set(self, 0x24, 0xA, 0);
                    em_state_set(self, 7, 0x13);
                }
            } else if (em_mot_end_ck(self) == 1) {
                em_mot_set(self, 0x43, 0xA, 0);
                em_state_set(self, 7, 0x12);
            }
        }
        break;
    }
}

/* 0x8036F1D4 (0x1EC): the blast: motion 0x35 with its four effect windows and the shape arm at frame 228. */
void em020_blast(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x35, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x23);
        self->timer_0x020 = 0;
        break;
    case 1:
        setVector3(&pos, 0.0f, -45.0f, 120.0f);
        if (em_frame_check(self, 3, 6.0f, 80.0f) == 1 && (system_w.field_0x0c & 3) == 0)
            eft_em_spawn(self, 2, 0x17, &pos, 1.0f);
        if (em_frame_check(self, 0, 228.0f, 0.0f) == 1)
            draw_shape_arm((u32)self, 0x18, 0xA);
        if (em_frame_check(self, 0, 228.0f, 0.0f) == 1)
            eft_em_spawn(self, 0, 0x17, &pos, 1.0f);
        if (em_frame_check(self, 3, 230.0f, 580.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0)
                eft_em_spawn(self, 1, 0x17, &pos, 1.0f);
            self->timer_0x020 += 1;
        }
        if (em_frame_check(self, 0, 290.0f, 0.0f) == 1)
            eft_em_spawn(self, 3, 0x17, &pos, 1.0f);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x8036F3C0 (0x524): the walking approach: motion 2 (alternating with 0xD4 for `kind` 1 and 2) with its hit
 * windows and stomps until the approach ends, then the closing motion (`end` 1 the swipe). */
void em020_walk_approach(_ENEMY_WORK* self, u8 kind, u8 end, u8 fast)
{
    u16 mot;
    u32 next;

    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 2, 0x14, 0, 1);
        if (kind == 3)
            em_mot_speed_set(self, 1.6f);
        else if (fast == 0)
            em_mot_speed_set(self, 0.85f);
        em_approach_start(self, 0.0f, 0x12);
        break;
    case 1:
        mot = em_get_mot_no(self);
        if ((mot == 2 && em_frame_check(self, 0, 340.0f, 0.0f) == 1) || em_mot_end_ck(self) == 1) {
            switch (kind) {
            case 0:
            case 3:
                next = 2;
                break;
            case 1:
                switch (self->state_0x006 & 1) {
                case 0:
                    next = 2;
                    break;
                case 1:
                    next = 0xD4;
                    break;
                }
                self->state_0x006 += 1;
                break;
            case 2:
                switch ((u32)self->state_0x006) {
                case 0:
                case 1:
                    next = 2;
                    break;
                case 2:
                    next = 0xD4;
                    break;
                }
                if (++self->state_0x006 > 2)
                    self->state_0x006 = 0;
                break;
            }
            if (mot != next) {
                if (next == 2) {
                    em_mot_set(self, next, 0, 0x156);
                    if (kind == 3)
                        em_mot_speed_set(self, 1.6f);
                    else if (fast == 0)
                        em_mot_speed_set(self, 0.85f);
                } else {
                    em_mot_set(self, next, 0, 0);
                    if (fast == 0)
                        em_mot_speed_set(self, 0.85f);
                }
            }
        }
        if (em_get_mot_no(self) == 2) {
            if (em_frame_check(self, 0, 50.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 0, 2);
            else if (em_frame_check(self, 0, 500.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 0, 3);
            else if (em_frame_check(self, 0, 800.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 0, 4);
            if (em_frame_check(self, 0, 500.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 1, 0x1F);
            else if (em_frame_check(self, 0, 700.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 1, 0x20);
            if (em_frame_check(self, 0, 370.0f, 0.0f) == 1 || em_frame_check(self, 0, 680.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 990.0f, 0.0f) == 1)
                em_eft_fx_spawn_c1(self, 2, 0x25, 2.0f, 0x28);
        } else {
            if (em_frame_check(self, 0, 180.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 0, 0x18);
            else if (em_frame_check(self, 0, 500.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 0, 0x19);
            if (em_frame_check(self, 0, 200.0f, 0.0f) == 1)
                em_hit_window_set_default(self, 1, 0x17);
        }
        if (em_approach_step(self, 0, 0x80) != 0 && em_mot_frame_started_ck(self) == 0) {
            self->state += 1;
            switch (end) {
            default:
                em_mot_set_blend(self, 3, 0x50, 0, 1);
                if (fast == 0)
                    em_mot_speed_set(self, 0.85f);
                break;
            case 1:
                em_mot_set_blend(self, 0x52, 0x50, 0, 1);
                if (fast == 0)
                    em_mot_speed_set(self, 0.85f);
                em_hit_window_set_default(self, 0, 0x1D);
                em_hit_window_set_default(self, 1, 0x1E);
                break;
            }
        }
        break;
    case 2:
        em020_angle_step_to_zero(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x8036F8E4 (0x174): action 5's sub-state handlers. */
void em020_action5_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot3_wait50(self);
        break;
    case 1:
        em020_turn_back(self, 0);
        break;
    case 2:
        em020_turn_back(self, 1);
        break;
    case 3:
        em020_arm_approach_mot1_wait55(self, 0);
        break;
    case 4:
        em020_arm_approach_mot1_wait55(self, 1);
        break;
    case 5:
        em020_arm_approach_mot1_wait55(self, 2);
        break;
    case 6:
        em020_arm_mot1_wait58(self);
        break;
    case 7:
        em020_circle_charge(self, 0, 0);
        break;
    case 8:
        em020_circle_charge(self, 1, 0);
        break;
    case 9:
        em020_circle_charge(self, 0, 1);
        break;
    case 10:
        em020_circle_charge(self, 1, 1);
        break;
    case 11:
        em020_circle_charge(self, 0, 2);
        break;
    case 12:
        em020_circle_charge(self, 1, 2);
        break;
    case 13:
        em020_blast(self);
        break;
    case 14:
        em020_walk_approach(self, 0, 0, 0);
        break;
    case 15:
        em020_circle_charge(self, 0, 3);
        break;
    case 16:
        em020_circle_charge(self, 1, 3);
        break;
    case 17:
        em020_walk_approach(self, 1, 0, 0);
        break;
    case 18:
        em020_walk_approach(self, 2, 0, 0);
        break;
    case 19:
        em020_walk_approach(self, 0, 1, 0);
        break;
    case 20:
        em020_walk_approach(self, 1, 1, 0);
        break;
    case 21:
        em020_walk_approach(self, 2, 1, 0);
        break;
    case 22:
        em020_walk_approach(self, 3, 0, 0);
        break;
    case 23:
        em020_arm_approach_mot1_wait55(self, 0);
        break;
    case 24:
        em020_walk_approach(self, 1, 0, 1);
        break;
    case 25:
        em020_walk_approach(self, 2, 0, 1);
        break;
    case 26:
        em020_walk_approach(self, 1, 1, 0);
        break;
    case 27:
        em020_walk_approach(self, 2, 1, 0);
        break;
    }
}

/* The 0x4d/0x14-frame pose's wake-up: step 1 keeps facing the target (`em_turn_to_target`) until the
 * motion is done. */
void em020_arm_mot1_facing_wait77(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4d, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0xa);
        break;
    case 1:
        if (em_frame_check(self, 1, 40.0f, 0.0f) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* 0x8036FB10 (0x2D8): the slam: motion 0x4A, blended with 0x4B (`kind` 1/3) or 0x4C (`kind` 2/4) on the second
 * layer, whose weight follows the motion frame (half strength for `kind` 3/4); two turn windows. */
void em020_slam(_ENEMY_WORK* self, u8 kind)
{
    f32 weight;

    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_mat_blend_reset_c1(self);
        switch (kind) {
        case 1:
        case 3:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_mot_layer_blend_set_c1(self, 1, 0x4B, 0xE, 0, 1);
            em_hit_window_set(self, 0, 9, 8);
            break;
        case 2:
        case 4:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_mot_layer_blend_set_c1(self, 1, 0x4C, 0xE, 0, 1);
            em_hit_window_set(self, 0, 9, 8);
            break;
        default:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_hit_window_set(self, 0, 9, 8);
            break;
        }
        break;
    case 1:
        if ((u32)(kind - 1) <= 3) {
            if (em_frame_check(self, 1, 40.0f, 0.0f) == 0)
                weight = em_mot_frame_get_c1(self) / 40.0f;
            else if (em_frame_check(self, 1, 160.0f, 0.0f) == 0)
                weight = 1.0f;
            else if (em_frame_check(self, 1, 230.0f, 0.0f) == 0)
                weight = 1.0f - (em_mot_frame_get_c1(self) - 160.0f) / 70.0f;
            else
                weight = 0.0f;
            if (weight < 0.0f)
                weight = 0.0f;
            else if (weight > 1.0f)
                weight = 1.0f;
            if ((u32)(kind - 3) <= 1)
                weight *= 0.5f;
            mhchar_mat_blend_set_c1((MHchar*)&self->char_0x024, 0, 1.0f - weight);
            mhchar_mat_blend_set_c1((MHchar*)&self->char_0x024, 1, weight);
        }
        if (self->state_0x006 == 0 && self->field_0xA0D == 0)
            self->state_0x006 += 1;
        em_turn_in_window(self, 0.0f, 160.0f, -0x1555);
        em_turn_in_window(self, 160.0f, 260.0f, 0x9555);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x8036FDE8 (0x48C): the leap: approaches (`kind` 1 from the saved position), lands with motion 0x47, then the
 * closing motion 0x48/0x49 with its hit windows; `kind` 1 chains into a second closing pass. */
void em020_leap(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3* home = &self->em020_0x328.home_0x328;

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        switch (kind) {
        default:
            em_approach_start(self, -1400.0f, 0x12);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0x12);
            copyVec3(home, &self->pos);
            break;
        }
        em_mot_set_blend(self, 0x46, 0xA, 0, 1);
        break;
    case 1:
        em_approach_step(self, 0, 0x80);
        if ((u32)((u16)self->field_0x1BC - 0x1001) <= 0x6FFE)
            em020_angle_step_to_zero(self);
        else if ((u32)((u16)self->field_0x1BC - 0x8001) <= 0x6FFE)
            em020_angle_step_to_zero(self);
        if (em_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 6, 0xB);
            em_hit_window_set(self, 1, 0x21, 0x23);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x47, 0, 0);
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state += 1;
            switch (kind) {
            default:
                em_mot_set_blend(self, 0x48, 0x10, 0, 1);
                break;
            case 1:
                em_mot_set_blend(self, 0x49, 0x10, 0, 1);
                break;
            }
        }
        if ((u32)((u16)self->field_0x1BC - 0x1001) <= 0x6FFE)
            em020_angle_step_to_zero(self);
        else if ((u32)((u16)self->field_0x1BC - 0x8001) <= 0x6FFE)
            em020_angle_step_to_zero(self);
        break;
    case 3:
        switch (kind) {
        default:
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
                em_hit_window_set(self, 0, 0xD, 0x10);
            }
            if (em_frame_check(self, 0, 32.0f, 0.0f) == 1)
                em_hit_window_clear(self, 1);
            em_turn_in_window(self, 150.0f, 240.0f, 0x8000);
            if (em_mot_end_ck(self) == 1)
                em_action_finish_swim(self);
            break;
        case 1:
            em_turn_in_window(self, 40.0f, 130.0f, 0x8000);
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
                em_hit_window_set(self, 0, 0xE, 0x10);
            }
            if (em_frame_check(self, 0, 32.0f, 0.0f) == 1)
                em_hit_window_clear(self, 1);
            if (em_frame_check(self, 0, 360.0f, 0.0f) == 1) {
                em_hit_window_set(self, 0, 6, 0xB);
                em_hit_window_set(self, 1, 0x21, 0x23);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state = 5;
                em_mot_set_blend(self, 0x48, 0x10, 0, 1);
            }
            break;
        }
        break;
    case 4:
        if (--self->timer_0x020 <= 0) {
            self->state += 1;
            em_mot_set_blend(self, 0x48, 0x10, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            em_hit_window_set(self, 0, 0xD, 0x10);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1)
            em_hit_window_clear(self, 1);
        em_turn_in_window(self, 150.0f, 240.0f, 0x8000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* The 0x4f/0x0a-frame pose's wake-up: step 1 re-aims with `em_height_follow`, closes with `em_turn_to_target`
 * when a frame window ends, and re-arms the turn. */
void em020_arm_mot1_turn_wait79(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4f, 0xa, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1: {
        if (em_frame_check(self, 2, 60.0f, 0.0f) == 1 ||
            em_frame_check(self, 3, 140.0f, 200.0f) == 1) {
            em_height_follow(self, 10.0f);
            em_turn_to_target(self, 0x50);
        }
        em_turn_in_window(self, 60.0f, 140.0f, 0x10000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
    default:
        break;
    }
}

/* 0x80370368 (0x240): the dash: motion 0x3A with an approach whose reach depends on `next`, then the follow-up
 * action `next` picks once the dash window closes. */
void em020_dash(_ENEMY_WORK* self, u8 next)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        switch (next) {
        case 0:
            em_approach_start(self, -2000.0f, 0x10);
            break;
        case 6:
            em_approach_start(self, -1300.0f, 0x10);
            break;
        case 7:
            em_approach_start(self, -1600.0f, 0x10);
            break;
        default:
            em_approach_start(self, -1450.0f, 0x10);
            break;
        }
        break;
    case 1:
        if (em_frame_check(self, 1, 20.0f, 0.0f) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_frame_check(self, 1, 168.0f, 0.0f) == 1)) {
            switch (next) {
            case 0:
                self->state += 1;
                em_mot_set_blend(self, 0x4D, 0x1E, 0x1E, 1);
                em_hit_window_set_default(self, 0, 0xA);
                break;
            case 1:
                em_act_set_unless_act11_c1(self, 7, 1);
                break;
            case 2:
                em_act_set_unless_act11_c1(self, 7, 2);
                break;
            case 3:
                em_act_set_unless_act11_c1(self, 7, 3);
                break;
            case 4:
                em_act_set_unless_act11_c1(self, 7, 4);
                break;
            case 5:
                em_act_set_unless_act11_c1(self, 7, 5);
                break;
            case 6:
                em_act_set_unless_act11_c1(self, 7, 0x11);
                break;
            case 7:
                em_act_set_unless_act11_c1(self, 7, 8);
                break;
            default:
                em_action_finish_swim(self);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* The 0x4e/0x0a-frame pose's wake-up: step 0 arms the motion and the two 8/0x10-part effect slots,
 * step 1 re-aims and closes the pose once the frame window reports done. */
void em020_arm_mot1_effects_wait78(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4e, 0xa, 0);
        em_hit_window_set(self, 0, 0xb, 8);
        em_hit_window_set(self, 1, 0xc, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            em_height_follow(self, 10.0f);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* 0x80370680 (0x12C): the tail sweep: motion 0x44 with its two hit windows and the 0x8000 turn between frames 358
 * and 398. */
void em020_tail_sweep(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x44, 4, 0);
        em_hit_window_set(self, 0, 0xF, 8);
        em_hit_window_set(self, 1, 0x10, 0x10);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 2, 60.0f, 0.0f) == 1)
            em_turn_to_target_3d_c1(self, 0xC0, 0x140);
        if (em_frame_check(self, 2, 190.0f, 0.0f) == 1)
            em_hit_arm_set(self);
        em_turn_in_window(self, 358.0f, 398.0f, 0x8000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x803707AC (0x114): the spin: motion 0x25 with the default hit windows 7/8 and the 0x8000 turn between frames
 * 338 and 428. */
void em020_spin(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x25, 6, 0);
        em_hit_window_set_default(self, 0, 7);
        em_hit_window_set_default(self, 1, 8);
        break;
    case 1:
        if (em_frame_check(self, 2, 60.0f, 0.0f) == 1)
            em_turn_to_target_3d_c1(self, 0xC0, 0x140);
        if (em_frame_check(self, 2, 380.0f, 0.0f) == 1)
            em_hit_arm_set(self);
        em_turn_in_window(self, 338.0f, 428.0f, 0x8000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x803708C0 (0x360): the first burrow attack: motion 0xC8 with its dust, the shell pair at frame 344 and the
 * camera shake. */
void em020_burrow_attack_a(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xC8, 0x14, 0);
        em_hit_arm_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 400.0f, 0.0f) == 1)
            em_turn_to_target_3d_c1(self, 0x20, 0x20);
        if (em_frame_check(self, 2, 780.0f, 0.0f) == 1)
            em_hit_arm_set(self);
        if (em_frame_check(self, 3, 172.0f, 192.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            setVector3(&pos, 0.0f, -80.0f, 100.0f);
            eft_em_spawn(self, 0x5F, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 196.0f, 308.0f) == 1 && (system_w.field_0x0c & 3) == 0)
            eft007_part_set(self, 0x2D);
        if (em_frame_check(self, 3, 196.0f, 340.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 0xA7, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 336.0f, 0.0f) == 1)
            eft007_part_spawn(self, 0x2C, 0, 0);
        if (em_frame_check(self, 0, 344.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 0.0f);
            shell_set_func_ptr->method_0x74(self, 0, 0x17, &pos, 1.0f, 0, 0xFFFF, shell_set_func_ptr);
            shell_set_func_ptr->method_0x64(self, 2, 0x17, 1.0f, 0xFFFF, shell_set_func_ptr);
            em_camera_req(self, 0x17, 6);
        }
        if (em_frame_check(self, 0, 520.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn(self, 0x88, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 780.0f, 800.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            setVector3(&pos, 0.0f, -80.0f, 100.0f);
            eft_em_spawn(self, 0x5F, 0x17, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x80370C20 (0x3A8): the second burrow attack: motion 0xC9, turning back between frames 80 and 240, with its
 * dust, the shell pair at frame 556 and the camera shake. */
void em020_burrow_attack_b(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xC9, 0x14, 0);
        break;
    case 1:
        em_turn_in_window(self, 80.0f, 240.0f, -0x10000);
        if (em_frame_check(self, 2, 240.0f, 0.0f) == 1)
            em_turn_to_target_3d_c1(self, 0x20, 0);
        else if (em_frame_check(self, 3, 240.0f, 380.0f) == 1)
            em_turn_to_target_3d_c1(self, 0x20, 0x100);
        else if (em_frame_check(self, 1, 900.0f, 0.0f) == 1)
            em020_angle_step_to_zero(self);
        if (em_frame_check(self, 3, 210.0f, 292.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 50.0f, 50.0f);
            eft_em_spawn(self, 0x60, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 320.0f, 500.0f) == 1 && (system_w.field_0x0c & 3) == 0)
            eft007_part_set(self, 0x2F);
        if (em_frame_check(self, 3, 320.0f, 496.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 80.0f);
            eft_em_spawn(self, 0xA8, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 546.0f, 0.0f) == 1)
            eft007_part_spawn(self, 0x2C, 0, 0);
        if (em_frame_check(self, 0, 556.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 0.0f);
            shell_set_func_ptr->method_0x74(self, 1, 0x17, &pos, 1.0f, 0, 0xFFFF, shell_set_func_ptr);
            shell_set_func_ptr->method_0x64(self, 3, 0x17, 1.0f, 0xFFFF, shell_set_func_ptr);
            em_camera_req(self, 0x17, 6);
        }
        if (em_frame_check(self, 0, 706.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -80.0f, 50.0f);
            eft_em_spawn(self, 0x89, 0x17, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 924.0f, 944.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            setVector3(&pos, 0.0f, -80.0f, 100.0f);
            eft_em_spawn(self, 0x61, 0x17, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* The two-directional 0x50/0x51-frame pose's wake-up: the argument picks the motion (0x51 when set,
 * 0x50 otherwise) and the sign of the 0x4000-step turn `em_turn_in_window` re-arms every frame. */
void em020_arm_mot1_side_wait80(_ENEMY_WORK* self, u8 right)
{
    Vec3 keys;

    VEC3_ctor(&keys);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, (u16)(right == 0 ? 0x50 : 0x51), 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x11);
        break;
    case 1:
        em_turn_in_window(self, 120.0f, 200.0f, right == 0 ? 0x4000 : -0x4000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    default:
        break;
    }
}

/* 0x803710AC (0x80): the 0xCB pose's two-step wake-up. */
void em020_pose_cb(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0xCB, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x8037112C (0xA0): the 0xCC pose with the default hit windows 0x12/0x13. */
void em020_pose_cc(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0xCC, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x12);
        em_hit_window_set_default(self, 1, 0x13);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x803711CC (0xB0): the 0xCD pose with the default hit window 0x15 and a full turn between frames 60 and 340. */
void em020_pose_cd(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0xCD, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x15);
        break;
    case 1:
        em_turn_in_window(self, 60.0f, 340.0f, 0x10000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x8037127C (0x234): the 0xCE hop: the two hit windows, and the height moved in five frame bands scaled by the
 * model and base scales. */
void em020_hop(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0xCE, 0x28, 0, 1);
        em_hit_window_set(self, 0, 0x14, 8);
        em_hit_window_set(self, 1, 0x22, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 4, 520.0f, 600.0f) == 1)
            self->pos.y -= 8.375f * get_em_base_scale(self) * get_em_scale(self);
        else if (em_frame_check(self, 4, 600.0f, 660.0f) == 1)
            self->pos.y += 2.1666667f * get_em_base_scale(self) * get_em_scale(self);
        else if (em_frame_check(self, 4, 660.0f, 750.0f) == 1)
            self->pos.y += 10.444445f * get_em_base_scale(self) * get_em_scale(self);
        else if (em_frame_check(self, 4, 840.0f, 1000.0f) == 1)
            self->pos.y -= 4.3125f * get_em_base_scale(self) * get_em_scale(self);
        else if (em_frame_check(self, 4, 1000.0f, 1204.0f) == 1)
            self->pos.y += 1.4215686f * get_em_base_scale(self) * get_em_scale(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x803714B0 (0xDC): action 7's sub-state handlers. */
void em020_action7_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot1_facing_wait77(self);
        break;
    case 1:
        em020_slam(self, 0);
        break;
    case 2:
        em020_slam(self, 1);
        break;
    case 3:
        em020_slam(self, 2);
        break;
    case 4:
        em020_slam(self, 3);
        break;
    case 5:
        em020_slam(self, 4);
        break;
    case 6:
        em020_leap(self, 0);
        break;
    case 7:
        em020_leap(self, 1);
        break;
    case 8:
        em020_arm_mot1_turn_wait79(self);
        break;
    case 9:
        em020_dash(self, 0);
        break;
    case 10:
        em020_dash(self, 1);
        break;
    case 11:
        em020_dash(self, 2);
        break;
    case 12:
        em020_dash(self, 3);
        break;
    case 13:
        em020_dash(self, 4);
        break;
    case 14:
        em020_dash(self, 5);
        break;
    case 15:
        em020_dash(self, 6);
        break;
    case 16:
        em020_dash(self, 7);
        break;
    case 17:
        em020_arm_mot1_effects_wait78(self);
        break;
    case 18:
        em020_tail_sweep(self);
        break;
    case 19:
        em020_spin(self);
        break;
    case 20:
        em020_burrow_attack_a(self);
        break;
    case 21:
        em020_burrow_attack_b(self);
        break;
    case 22:
        em020_arm_mot1_side_wait80(self, 0);
        break;
    case 23:
        em020_arm_mot1_side_wait80(self, 1);
        break;
    case 24:
        em020_pose_cb(self);
        break;
    case 25:
        em020_pose_cc(self);
        break;
    case 26:
        em020_pose_cd(self);
        break;
    case 27:
        em020_hop(self);
        break;
    }
}

/* The sound-effect programs, defined after `em020_action11_play`, the last handler that passes them. */
extern EmSeEntry em020_se_tbl_mot_a[];
extern EmSeEntry em020_se_tbl_mot_b[];
extern EmSeEntry em020_se_tbl_mot_c[];
extern EmSeEntry em020_se_tbl_act11[];

/* 0x8037158C (0x12C): action 10's sub-state handler: plays the motion's sound table, or finishes the action for any
 * other motion. */
void em020_action10_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x28:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x28);
        break;
    case 0x29:
        em_se_tbl_play(self, em020_se_tbl_mot_b, 2, 0x29);
        break;
    case 0x2A:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x2A);
        break;
    case 0x2B:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x2B);
        break;
    case 0x2C:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x2C);
        break;
    case 0x2D:
        em_se_tbl_play(self, em020_se_tbl_mot_b, 2, 0x2D);
        break;
    case 0x2E:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x2E);
        break;
    case 0x2F:
        em_se_tbl_play(self, em020_se_tbl_mot_a, 2, 0x2F);
        break;
    case 0xF7:
        em_se_tbl_play(self, em020_se_tbl_mot_c, 2, 0xF7);
        break;
    case 0xCB:
        em_se_tbl_play(self, em020_se_tbl_mot_c, 2, 0xCB);
        break;
    default:
        em_action_finish_swim(self);
        break;
    }
}

/* 0x803716B8 (0x14): action 11's handler: the branching sound table. */
void em020_action11_play(_ENEMY_WORK* self)
{
    em_se_tbl_play_alt(self, em020_se_tbl_act11, 2, 0xA);
}

/* The sound-effect programs `em020_action10_dispatch`/`em020_action11_play` pass to the table walker
 * (`.data` 0x805EE2E0..0x805EE3A8). */
EmSeRecord em020_se_rec_mot_a = {0x64, 0, 0xA, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em020_se_tbl_mot_a[2] = {
    {0x00, {0, 0, 0}, &em020_se_rec_mot_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em020_se_rec_mot_b = {0x65, 0, 0xA, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em020_se_tbl_mot_b[2] = {
    {0x00, {0, 0, 0}, &em020_se_rec_mot_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em020_se_rec_mot_c = {0x66, 0, 0xA, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em020_se_tbl_mot_c[2] = {
    {0x00, {0, 0, 0}, &em020_se_rec_mot_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em020_se_rec_act11_a = {0x6F, 0, 0xA, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em020_se_rec_act11_b = {0x70, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em020_se_tbl_act11[4] = {
    {0x00, {0, 0, 0}, &em020_se_rec_act11_a},
    {0x03, {0, 0, 0}, (const EmSeRecord*)1},
    {0x00, {0, 0, 0}, &em020_se_rec_act11_b},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x803716CC (0xD24): the opening demo: places the record, waits for demo frame 0x104, then plays the two demo
 * motions (0xCA, 0xD3) with their effect timeline. */
void em020_demo_intro(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_water_surface_latch(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 4138.0f, -2741.0f, -7885.0f);
        em_demo_rot_set(self, 0.0f, -78.75f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x104) == 1) {
            self->state += 1;
            self->timer_0x020 = 0;
            em_mot_set(self, 0xCA, 0, 0x2A);
        }
        break;
    case 2:
        if (em_frame_check(self, 3, 82.0f, 484.0f) == 1) {
            if (--self->timer_0x020 < 0) {
                eft_em_spawn(self, 0xBF, 0x17, NULL, 7.5f);
                self->timer_0x020 = 0x10;
            }
        }
        if (em_frame_check(self, 0, 162.0f, 0.0f) == 1 || em_frame_check(self, 0, 182.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBE, 0x17, NULL, 5.0f);
        if (em_frame_check(self, 0, 202.0f, 0.0f) == 1 || em_frame_check(self, 0, 222.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBE, 0x17, NULL, 7.0f);
        if (em_frame_check(self, 0, 240.0f, 0.0f) == 1) {
            setVector3(&pos, -125.0f, -200.0f, 75.0f);
            eft_em_spawn(self, 0xC8, 0x17, &pos, 2.8f);
        }
        if (em_frame_check(self, 0, 250.0f, 0.0f) == 1) {
            setVector3(&pos, -100.0f, -125.0f, 100.0f);
            eft_em_spawn(self, 0xBD, 0x17, &pos, 1.8f);
        }
        if (em_frame_check(self, 0, 260.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -250.0f, 400.0f);
            eft_em_spawn(self, 0xBD, 0x17, &pos, 2.2f);
        }
        if (em_frame_check(self, 0, 300.0f, 0.0f) == 1 || em_frame_check(self, 0, 320.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 340.0f, 0.0f) == 1 || em_frame_check(self, 0, 360.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 380.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC0, 0x17, NULL, 1.2f);
        if (em_frame_check(self, 0, 402.0f, 0.0f) == 1) {
            setVector3(&pos, -150.0f, 0.0f, 50.0f);
            eft_em_spawn(self, 0xBD, 0x17, &pos, 2.0f);
        }
        if (em_frame_check(self, 0, 438.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC0, 0x13, NULL, 1.0f);
        if (em_frame_check(self, 0, 468.0f, 0.0f) == 1) {
            setVector3(&pos, 200.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0xC0, 1, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 488.0f, 0.0f) == 1) {
            setVector3(&pos, -100.0f, 0.0f, -50.0f);
            eft_em_spawn(self, 0xC0, 1, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 508.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC0, 0x22, NULL, 1.0f);
        if (em_frame_check(self, 0, 522.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 0x23, NULL, 6.0f);
        if (em_frame_check(self, 0, 528.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC0, 0x23, NULL, 1.0f);
        if (em_frame_check(self, 0, 552.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBE, 0x24, NULL, 3.0f);
        if (em_frame_check(self, 0, 574.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBD, 0x26, NULL, 1.3f);
        if (em_frame_check(self, 0, 562.0f, 0.0f) == 1 || em_frame_check(self, 0, 602.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 642.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 0x24, NULL, 6.0f);
        if (em_frame_check(self, 0, 622.0f, 0.0f) == 1 || em_frame_check(self, 0, 642.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 662.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBE, 0x29, NULL, 4.0f);
        if (em_frame_check(self, 1, 688.0f, 0.0f) == 1)
            em_height_add_c1(self, -25.0f);
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            self->timer_0x020 = 0;
            em_mot_set(self, 0xD3, 0, 0);
            em_demo_pos_set(self, 4465.0f, -2432.0f, -3705.0f);
            em_height_set_c1(self, 0.0f);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1 || em_frame_check(self, 0, 20.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 0x17, NULL, 7.0f);
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            setVector3(&pos, -100.0f, -150.0f, 250.0f);
            eft_em_spawn(self, 0xC1, 0x17, &pos, 2.0f);
        }
        if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -40.0f, 150.0f);
            eft_em_spawn(self, 0xC3, 0xF, &pos, 0.85f);
        }
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 1, NULL, 7.0f);
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -80.0f, 0.0f);
            eft_em_spawn(self, 0xC3, 0xF, &pos, 0.85f);
        }
        if (em_frame_check(self, 0, 106.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBE, 1, NULL, 5.0f);
        if (em_frame_check(self, 0, 126.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 1, NULL, 5.0f);
        if (em_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            setVector3(&pos, -80.0f, -80.0f, 30.0f);
            eft_em_spawn(self, 0xC3, 0xF, &pos, 0.85f);
        }
        if (em_frame_check(self, 0, 202.0f, 0.0f) == 1 || em_frame_check(self, 0, 222.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -200.0f, 135.0f);
            eft_em_spawn(self, 0xC2, 3, &pos, 1.1f);
        }
        if (em_frame_check(self, 0, 342.0f, 0.0f) == 1) {
            setVector3(&pos, 175.0f, -200.0f, 135.0f);
            eft_em_spawn(self, 0xC2, 3, &pos, 1.1f);
        }
        if (em_frame_check(self, 0, 402.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 0x17, NULL, 7.0f);
        if (em_frame_check(self, 0, 426.0f, 0.0f) == 1) {
            setVector3(&pos, 50.0f, 0.0f, 1000.0f);
            eft_em_spawn(self, 0xC9, 1, &pos, 1.2f);
        }
        if (em_frame_check(self, 0, 434.0f, 0.0f) == 1) {
            setVector3(&pos, 50.0f, 0.0f, 1200.0f);
            eft_em_spawn(self, 0xC1, 1, &pos, 2.0f);
        }
        if (em_frame_check(self, 0, 442.0f, 0.0f) == 1 || em_frame_check(self, 0, 482.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 522.0f, 0.0f) == 1 || em_frame_check(self, 0, 562.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 602.0f, 0.0f) == 1 || em_frame_check(self, 0, 642.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xBF, 1, NULL, 6.0f);
        if (em_frame_check(self, 0, 444.0f, 0.0f) == 1) {
            setVector3(&pos, 50.0f, 0.0f, 1200.0f);
            eft_em_spawn(self, 0xC9, 1, &pos, 1.5f);
        }
        if (em_frame_check(self, 0, 492.0f, 0.0f) == 1 || em_frame_check(self, 0, 512.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 532.0f, 0.0f) == 1 || em_frame_check(self, 0, 552.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 572.0f, 0.0f) == 1 || em_frame_check(self, 0, 592.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 612.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC0, 1, NULL, 1.2f);
        if (em_frame_check(self, 3, 532.0f, 574.0f) == 1 && em_demo_frame_get() % 10 == 0)
            eft_em_spawn(self, 0xCF, 1, NULL, 1.3f);
        if (em_frame_check(self, 0, 652.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xC1, 0x24, NULL, 1.5f);
        if (em_frame_check(self, 3, 652.0f, 1174.0f) == 1 && em_demo_frame_get() % 10 == 0)
            eft_em_spawn(self, 0xBF, 0x22, NULL, 6.0f);
        break;
    }
}

/* 0x803723F0 (0x80): places the record for the second demo and holds motion 0xD1. */
void em020_demo_hold(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xD1, 0, 0);
        em_demo_pos_set(self, -1655.0f, -20442.0f, -32.0f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        break;
    }
}

/* 0x80372470 (0x4C0): the second demo: motion 0xD1 until demo frame 0x11A, then the three 0xD2 passes with their
 * effects and the shell pair at frame 794. */
void em020_demo_burrow(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xD1, 0, 0x12C);
        em_demo_pos_set(self, -1655.0f, -20442.0f, -32.0f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x11A) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD2, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            setVector3(&pos, -50.0f, -100.0f, 125.0f);
            eft_em_spawn(self, 0xC4, 0x18, &pos, 0.7f);
        }
        if (em_frame_check(self, 0, 150.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -100.0f, 50.0f);
            eft_em_spawn(self, 0xC5, 0x18, &pos, 0.7f);
        }
        if (em_frame_check(self, 0, 152.0f, 0.0f) == 1) {
            setVector3(&pos, 50.0f, -100.0f, 50.0f);
            eft_em_spawn(self, 0xC4, 0x18, &pos, 0.7f);
        }
        if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x25, 6, &pos, 1.5f, 0x64);
        }
        if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -300.0f, 150.0f);
            eft_em_spawn_joint(self, 0x13, 6, &pos, 2.5f, 0x64);
        }
        if (em_frame_check(self, 0, 230.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -150.0f, 400.0f);
            eft_em_spawn(self, 0xC6, 0x13, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 320.0f, 720.0f) == 1 && (system_w.field_0x0c & 3) == 0)
            eft007_part_set(self, 0x2F);
        if (em_demo_time_ck(0x334) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD2, 0, 0x28A);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 784.0f, 0.0f) == 1)
            eft007_part_spawn(self, 0x2C, 0, 0);
        if (em_frame_check(self, 0, 794.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 0.0f);
            shell_set_func_ptr->method_0x74(self, 5, 0x17, &pos, 1.0f, 0, 0xFFFF, shell_set_func_ptr);
            shell_set_func_ptr->method_0x64(self, 4, 0x17, 1.0f, 0xFFFF, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, 920.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -80.0f, 50.0f);
            eft_em_spawn(self, 0x89, 0x17, &pos, 0.5f);
        }
        if (em_demo_time_ck(0x4BC) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD2, 0, 0x412);
            em_mot_speed_set(self, 0.8f);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, 1150.0f, 1160.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            setVector3(&pos, 0.0f, -80.0f, 100.0f);
            eft_em_spawn(self, 0x61, 0x17, &pos, 1.0f);
        }
        break;
    }
}

/* 0x80372930 (0x28C): the third demo: motion 0x65 with the stage event, two joint effects, the 0xCF turn and the
 * closing 0x2C/0x29 motions. */
void em020_demo_emerge(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x65, 0, 0);
        camera_demo_event_set(0x3E, self);
        em_demo_pos_set(self, -4991.0f, -20390.0f, -3126.0f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x17, 0x20, &pos, 1.0f, 0x64);
        }
        if (em_after_frame_check(self, 0, 282.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x17, 0x20, &pos, 1.5f, 0x64);
        }
        if (++self->timer_0x020 >= 0xD7) {
            self->state += 1;
            em_mot_set(self, 0xCF, 0, 0x6E);
            em_demo_rot_set(self, -27.5f, 0.0f, 0.0f);
            self->timer_0x020 = 0;
        }
        break;
    case 2:
        if (em_frame_check(self, 5, 110.0f, 360.0f) != 0)
            self->field_0x1BC -= (s32)(45.512f * get_em_base_scale(self));
        if (++self->timer_0x020 >= 0x7D) {
            self->state += 1;
            em_mot_set(self, 0x2C, 0, 0);
            em_demo_rot_set(self, -90.0f, 0.0f, 0.0f);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 0x29, 0x28, 0, 1);
        }
        break;
    case 4:
        em_water_surface_latch(self);
        break;
    }
}

/* 0x80372BBC (0xA4): places the record for the first area's entry and plays motion 1. */
void em020_entry_area_a(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 4138.0f, -1979.0f, -7885.0f);
        em_demo_rot_set(self, 0.0f, -67.5f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x80372C60 (0xA4): places the record for the second area's entry and plays motion 1. */
void em020_entry_area_b(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -1655.0f, -27000.0f, -32.0f);
        em_demo_rot_set(self, 0.0f, 55.0f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_swim(self);
        break;
    }
}

/* 0x80372D04 (0x54): action 13's sub-state handlers (the demos). */
void em020_action13_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_demo_intro(self);
        break;
    case 1:
        em020_demo_hold(self);
        break;
    case 2:
        em020_demo_burrow(self);
        break;
    case 3:
        em020_demo_emerge(self);
        break;
    case 4:
        em020_entry_area_a(self);
        break;
    case 5:
        em020_entry_area_b(self);
        break;
    }
}

/* 0x80372D58 (0x54): the program's action handler: the sub-state handler of the current action. */
void em020_action_dispatch(_ENEMY_WORK* self)
{
    switch (self->action) {
    case 0:
        em020_substate_dispatch(self);
        break;
    case 5:
        em020_action5_dispatch(self);
        break;
    case 7:
        em020_action7_dispatch(self);
        break;
    case 10:
        em020_action10_dispatch(self);
        break;
    case 11:
        em020_action11_play(self);
        break;
    case 13:
        em020_action13_dispatch(self);
        break;
    }
}

/* The glow keys `em020_glow_color_get` reads over the two burrow motions 0xC8/0xC9 (`.data` 0x805EE3A8 and
 * 0x805EE3D0): (frame, level) pairs ending at frame -1. */
f32 em020_glow_keys_c8[10] = {
    0.0f, 0.0f, 176.0f, 1.0f, 280.0f, 1.0f, 400.0f, 0.0f, -1.0f, 0.0f,
};

f32 em020_glow_keys_c9[10] = {
    100.0f, 0.0f, 274.0f, 1.0f, 360.0f, 1.0f, 500.0f, 0.0f, -1.0f, 0.0f,
};

/* 0x80372DAC (0x214): the target K-colour of the glow: off outside the latched area, else a pulsing level of the
 * mode's colour, raised to the burrow motion's key level. */
void em020_glow_color_get(_ENEMY_WORK* self, u8* r, u8* g, u8* b)
{
    f32 level;
    f32 key;

    if (self->em020_0x328.latch_0x338 == 0) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }
    if (em_alt_mode_ck(self) == 1) {
        level = 0.5f + 0.2f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c << 9)));
        *r = 0xA8;
        *g = 0x50;
        *b = 0x63;
    } else {
        level = 0.5f + 0.25f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c << 9)));
        *r = 0x89;
        *g = 0xFB;
        *b = 0xE5;
    }
    switch (em_get_mot_no(self)) {
    case 0xC8:
        key = getKeyData(em020_glow_keys_c8, em_mot_frame_get_c1(self));
        break;
    case 0xC9:
        key = getKeyData(em020_glow_keys_c9, em_mot_frame_get_c1(self));
        break;
    default:
        key = 0.0f;
        break;
    }
    if (key > level)
        level = key;
    if (level >= 1.0f)
        level = 1.0f;
    *r = *r * level;
    *g = *g * level;
    *b = *b * level;
}

/* 0x80372FC0 (0x50): steps the colour component `cur` toward `target` by `step`, landing on it. */
u8 em020_color_step(u8 cur, u8 target, u8 step)
{
    if (cur < target) {
        if (cur + step < target)
            return cur + step;
    } else if (cur > target) {
        if (cur > target + step)
            return cur - step;
    }
    return target;
}

/* 0x80373010 (0x18C): the per-frame glow: in the area-13 demo the K-colour follows the demo keys, otherwise it
 * steps toward `em020_glow_color_get`'s target. */
void em020_glow_update(_ENEMY_WORK* self)
{
    u8 r;
    u8 g;
    u8 b;

    if ((u32)em_act_ck(self, 13, 2) == 1) {
        f32 level;
        f32 v;

        r = 0x89;
        g = 0xFB;
        b = 0xE5;
        level = getKeyData(em020_glow_keys_demo, em_demo_frame_get() * 2);
        v = r * level;
        if (v > 255.0f)
            v = 255.0f;
        self->em020_0x328.sel_0x339 = v;
        v = g * level;
        if (v > 255.0f)
            v = 255.0f;
        self->em020_0x328.sel_0x33A = v;
        v = b * level;
        if (v > 255.0f)
            v = 255.0f;
        self->em020_0x328.sel_0x33B = v;
    } else {
        em020_glow_color_get(self, &r, &g, &b);
        self->em020_0x328.sel_0x339 = em020_color_step(self->em020_0x328.sel_0x339, r, 12);
        self->em020_0x328.sel_0x33A = em020_color_step(self->em020_0x328.sel_0x33A, g, 12);
        self->em020_0x328.sel_0x33B = em020_color_step(self->em020_0x328.sel_0x33B, b, 12);
    }
}

/* 0x8037319C (0xEC): on the type-7 map, outside area 0 and while the record is in its area, publishes the stage
 * light at the record's position (dim white, or the mode's colour while the glow latch is set). */
void em020_light_update(_ENEMY_WORK* self)
{
    _GXColor color;
    f32 range;
    f32 falloff;

    if (stage_map_kind_get(self->field_0x1E0) == 7) {
        if (self->area_no == 0) {
        } else if (em_area_ck(self) != 0) {
            if (self->em020_0x328.latch_0x338 == 0) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xAA;
                color.a = 0xFF;
                range = 1210.0f;
                falloff = 0.97f;
            } else {
                if (em_alt_mode_ck(self) == 1) {
                    color.r = 0xD2;
                    color.g = 0x69;
                    color.b = 0x87;
                } else {
                    color.r = 0xFF;
                    color.g = 0xFF;
                    color.b = 0xAA;
                }
                color.a = 0xFF;
                range = 90000.0f;
                falloff = 1.0f;
            }
            light_point_set(self->em020_0x328.value_0x336, &self->pos, &color, range, falloff);
        }
    }
}

/* 0x80373288 (0x134): in the alternate mode outside the burrow motions, every 32 frames, spawns the drool effect at
 * joint 0x17 (0x1E facing down, 0x1F otherwise). */
void em020_drool_spawn(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    u16 pitch;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    if (em_alt_mode_ck(self) == 1 && (u32)(em_get_mot_no(self) - 0xC8) > 1) {
        pitch = ((u32 (*)(nw4r::math::VEC3*))calcVecAngX)(&self->vec_0x76C);
        if ((system_w.field_0x0c & 0x1F) == 0) {
            setVector3(&pos, 0.0f, -45.0f, 120.0f);
            get_joint_wmat_em(self, 0x17, &mtx);
            mulVecMat(&pos, &mtx);
            pos.x += mtx.m[0][3];
            pos.y += mtx.m[1][3];
            pos.z += mtx.m[2][3];
            if ((u16)(pitch + 0x8000) > 0x6E38)
                eft_spawn_type10(self, 0x1F, 0x17, &pos, 1.0f);
            else
                eft_spawn_type10(self, 0x1E, 0x17, &pos, 1.0f);
        }
    }
}

/* 0x803733BC (0x1CC8): the program's motion-event slot: the drool, then the effects, sounds and camera requests
 * keyed to the frames of the running motion, then the glow and the stage light. */
void em020_motion_events(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 sum;

    VEC3_ctor(&pos);
    VEC3_ctor(&from);
    VEC3_ctor(&to);
    em020_drool_spawn(self);
    switch (em_get_mot_no(self)) {
    case 0x02:
        if ((em_after_frame_check(self, 3, 140.0f, 212.0f) == 1 || em_after_frame_check(self, 3, 490.0f, 554.0f) == 1 ||
             em_after_frame_check(self, 3, 890.0f, 986.0f) == 1) &&
            (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x27, NULL, 1.0f);
        if ((em_after_frame_check(self, 3, 150.0f, 246.0f) == 1 || em_after_frame_check(self, 3, 500.0f, 580.0f) == 1 ||
             em_after_frame_check(self, 3, 900.0f, 1012.0f) == 1) &&
            (system_w.field_0x0c & 7) == 3)
            eft_em_spawn(self, 0x92, 0x28, NULL, 1.0f);
        if (em_after_frame_check(self, 0, 624.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 830.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0x92, 6, NULL, 0.8f);
            eft_em_spawn(self, 0x93, 6, NULL, 0.5f);
            eft_em_spawn(self, 0x92, 0xB, NULL, 0.8f);
            eft_em_spawn(self, 0x93, 0xB, NULL, 0.5f);
        }
        break;
    case 0x25:
        if (em_after_frame_check(self, 3, 100.0f, 172.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, -50.0f, 300.0f);
            eft_em_spawn(self, 0xB2, 0x17, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 1, 110.0f, 0.0f) == 1 && (system_w.field_0x0c & 0xF) == 0)
            em_camera_req(self, -1, 5);
        break;
    case 0x33:
    case 0x34:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1)
            em_hit_window_set_default(self, 0, 0x1A);
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        break;
    case 0x3A:
        if (em_after_frame_check(self, 3, 110.0f, 158.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x92, 0x28, NULL, 1.0f);
        if (em_after_frame_check(self, 3, 120.0f, 184.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x28, NULL, 1.0f);
        break;
    case 0x44:
        if (em_after_frame_check(self, 3, 120.0f, 248.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, -50.0f, 300.0f);
            eft_em_spawn(self, 0xB2, 0x17, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 1, 120.0f, 0.0f) == 1 && (system_w.field_0x0c & 0xF) == 0)
            em_camera_req(self, -1, 5);
        break;
    case 0x46:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 0.7f);
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 0.7f);
        if (em_after_frame_check(self, 3, 140.0f, 172.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, -50.0f, 300.0f);
            eft_em_spawn(self, 0x97, 0x14, &pos, 1.0f);
        }
        break;
    case 0x47:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 200.0f);
            eft_em_spawn(self, 0xB3, 0x17, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 200.0f);
            eft_em_spawn(self, 0xB3, 0x17, &pos, 1.0f);
        }
        if ((system_w.field_0x0c & 0xF) == 0)
            em_camera_req(self, -1, 5);
        break;
    case 0x49:
        if (em_after_frame_check(self, 0, 356.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 200.0f);
            eft_em_spawn(self, 0xB3, 0x17, &pos, 1.0f);
        }
        break;
    case 0x4A:
    case 0x4B:
    case 0x4C:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 200.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x27, 0.7f);
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 210.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn_joint(self, 0x17, 4, &pos, 2.0f, 0x64);
            eft_em_spawn_joint(self, 0x17, 6, &pos, 2.0f, 0x64);
        }
        if (em_after_frame_check(self, 0, 200.0f, 0.0f) == 1)
            em_camera_req(self, 0x17, 5);
        break;
    case 0x4D:
        if (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x27, 6, &pos, 2.0f, 0x64);
        }
        if (em_after_frame_check(self, 0, 210.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 1200.0f * get_em_scale(self));
            eft026_pl_spawn_c1((struct _EFT26_EM*)self, 8, &pos, 3, get_em_scale(self));
            setVector3(&pos, 0.0f, 0.0f, 1800.0f * get_em_chg_scale(self));
            rotVecY(&pos, self->field_0x1C0);
            addVec3(&sum, &self->pos, &pos);
            copyVec3(&from, copyVec3(&to, &sum));
            from.y -= 1000.0f * get_em_chg_scale(self);
            to.y += 700.0f * get_em_chg_scale(self);
            shell_set_func_ptr->method_0x30(self, 0xA, &from, &to, 1.8f, 0xFFFF, shell_set_func_ptr);
            em_camera_req(self, 0x27, 5);
        }
        break;
    case 0x4E:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn_joint(self, 0x17, 4, &pos, 2.0f, 0x64);
        }
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 150.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn_joint(self, 0x17, 3, &pos, 1.0f, 0x64);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 176.0f, 0.0f) == 1)
            em_camera_req(self, 0x17, 5);
        break;
    case 0x4F:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 22.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 42.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 52.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x98, 0x17, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x99, 0x21, NULL, 1.0f);
        if (em_after_frame_check(self, 0, 330.0f, 0.0f) == 1) {
            setVector3(&pos, -100.0f, 100.0f, 0.0f);
            eft_em_spawn(self, 0x94, 0x21, &pos, 1.0f);
            eft_em_spawn(self, 0x95, 0x21, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 360.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 372.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 384.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 396.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 408.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x93, 0x28, NULL, 0.8f);
        if (em_after_frame_check(self, 0, 370.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 382.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 394.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 406.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 418.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x92, 0x27, NULL, 0.8f);
        if (em_after_frame_check(self, 0, 314.0f, 0.0f) == 1)
            em_camera_req(self, -1, 6);
        break;
    case 0x52:
        if (em_after_frame_check(self, 0, 156.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xB4, 0x14, &pos, 1.0f, 0xC000);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 3, 30.0f, 78.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x17, NULL, 0.7f);
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 340.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x97, 0x27, NULL, 1.5f);
        if ((em_after_frame_check(self, 3, 40.0f, 88.0f) == 1 || em_after_frame_check(self, 3, 280.0f, 328.0f) == 1) &&
            (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x92, 0x17, NULL, 1.0f);
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 62.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 74.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 86.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 350.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 362.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 374.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 386.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x96, 0x28, NULL, 1.5f);
        if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x27, 0.7f);
        if (em_after_frame_check(self, 0, 250.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if (em_after_frame_check(self, 3, 290.0f, 338.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x17, NULL, 0.5f);
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1)
            em_camera_req(self, -1, 5);
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 42.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 54.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 66.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x93, 0x17, &pos, 0.7f);
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 52.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 64.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 76.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x92, 0x17, &pos, 0.7f);
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x27, 1.0f);
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if ((em_after_frame_check(self, 3, 280.0f, 328.0f) == 1 || em_after_frame_check(self, 3, 380.0f, 428.0f) == 1) &&
            (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x92, 0x17, &pos, 0.5f);
        }
        if ((em_after_frame_check(self, 3, 290.0f, 338.0f) == 1 || em_after_frame_check(self, 3, 390.0f, 438.0f) == 1) &&
            (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x93, 0x17, &pos, 0.5f);
        }
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1)
            em_camera_req(self, -1, 5);
        break;
    case 0x6F:
        if (em_after_frame_check(self, 3, 70.0f, 250.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x94, 0x27, NULL, 1.0f);
        if (em_after_frame_check(self, 3, 80.0f, 260.0f) == 1 && (system_w.field_0x0c & 7) == 3)
            eft_em_spawn(self, 0x95, 0x28, NULL, 1.0f);
        if (em_after_frame_check(self, 3, 90.0f, 218.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x93, 0x17, &pos, 0.7f);
        }
        if (em_after_frame_check(self, 3, 390.0f, 550.0f) == 1 && (system_w.field_0x0c & 0xF) == 3)
            eft_em_spawn(self, 0x93, 0x27, NULL, 1.0f);
        if (em_after_frame_check(self, 3, 400.0f, 560.0f) == 1 && (system_w.field_0x0c & 0xF) == 0)
            eft_em_spawn(self, 0x92, 0x28, NULL, 1.0f);
        if (em_after_frame_check(self, 0, 410.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 418.0f, 0.0f) == 1)
            eft_em_spawn(self, 0xB9, 0x17, NULL, 2.0f);
        break;
    case 0xC8:
        if (em_after_frame_check(self, 0, 346.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 356.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        break;
    case 0xC9:
        if (em_after_frame_check(self, 0, 710.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 760.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 720.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 770.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        break;
    case 0xCC:
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 160.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 170.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if (em_after_frame_check(self, 3, 150.0f, 246.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x93, 0x17, &pos, 0.5f);
        }
        if (em_after_frame_check(self, 3, 160.0f, 256.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x92, 0x17, &pos, 0.5f);
        }
        if (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            setVector3(&pos, 200.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xB4, 0x14, &pos, 1.0f, 0x4000);
        }
        if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1) {
            setVector3(&pos, -200.0f, -100.0f, 0.0f);
            eft_em_spawn_param(self, 0xB4, 0x14, &pos, 1.0f, 0xC000);
        }
        break;
    case 0xCD:
        if (em_after_frame_check(self, 0, 280.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
            em_camera_req(self, 0x21, 6);
        }
        if (em_after_frame_check(self, 0, 290.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        break;
    case 0xCE:
        if (em_after_frame_check(self, 3, 630.0f, 694.0f) == 1) {
            if ((system_w.field_0x0c & 3) == 0) {
                setVector3(&pos, 0.0f, -50.0f, 300.0f);
                eft_em_spawn(self, 0x97, 0x17, &pos, 1.0f);
            }
            if ((system_w.field_0x0c & 0xF) == 0)
                em_camera_req(self, -1, 5);
        }
        if (em_after_frame_check(self, 0, 1150.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 1160.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, 940.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 1000.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 8, 0x28, 1.0f);
        if (em_after_frame_check(self, 0, 950.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 1010.0f, 0.0f) == 1)
            em_eft_fx_spawn_anchor_c1(self, 9, 0x28, 1.0f);
        break;
    case 0xD4:
        if (em_after_frame_check(self, 3, 140.0f, 212.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x27, NULL, 0.5f);
        if (em_after_frame_check(self, 3, 150.0f, 246.0f) == 1 && (system_w.field_0x0c & 7) == 3)
            eft_em_spawn(self, 0x92, 0x28, NULL, 0.5f);
        if (em_after_frame_check(self, 3, 310.0f, 406.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x93, 0x27, NULL, 0.5f);
        if (em_after_frame_check(self, 0, 470.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 494.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 518.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 542.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 566.0f, 0.0f) == 1)
            eft_em_spawn(self, 0x93, 0x27, NULL, 0.5f);
        if (em_after_frame_check(self, 3, 480.0f, 592.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft_em_spawn(self, 0x92, 0x28, NULL, 0.5f);
        break;
    }
    em020_glow_update(self);
    em020_light_update(self);
}

#ifdef __cplusplus
}
#endif

/* Pushes the model state into the scene model: the effect matrices of material slots 8 and 9, then the
 * K-colours (6/3 from the record, 0..3 from the area/action test). */
extern "C" void em020_model_refresh(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access_a((nw4r::g3d::ScnMdl*) self->field_0x13C, 8);
        nw4r::g3d::ScnMdl::CopiedMatAccess access_b((nw4r::g3d::ScnMdl*) self->field_0x13C, 9);

        if (copied_mat_access_valid_ck(&access_a) != 0 && copied_mat_access_valid_ck(&access_b) != 0) {
            nw4r::g3d::ResTexSrt srt_a;
            nw4r::g3d::ResTexSrt srt_b;

            res_tex_srt_copy_ctor(&srt_a, access_a.GetResTexSrt(false));
            res_tex_srt_copy_ctor(&srt_b, access_b.GetResTexSrt(false));
            srt_a.GetEffectMtx(1, &mtx);
            mtx.m[0][3] = 0.5f * sin_idx_get((u16) (system_w.field_0x0c << 7));
            mtx.m[1][3] += 0.0025f;
            if (mtx.m[1][3] > 1.0f) {
                mtx.m[1][3] -= 1.0f;
            }
            srt_a.SetEffectMtx(1, &mtx);
            srt_b.SetEffectMtx(1, &mtx);
        }
    }

    ((MHchar*) &self->char_0x024)->getTevKColor(6, GX_KCOLOR3, &color);
    color.r = self->em020_kcolor_0x328.kcolor_r_0x339;
    color.g = self->em020_kcolor_0x328.kcolor_g_0x33A;
    color.b = self->em020_kcolor_0x328.kcolor_b_0x33B;
    ((MHchar*) &self->char_0x024)->setTevKColor(6, GX_KCOLOR3, &color);

    ((MHchar*) &self->char_0x024)->getTevKColor(0, GX_KCOLOR0, &color);
    if (self->area_no == 0 || em_act_ck(self, 13, 2) != 0) {
        color.r = 150;
        color.g = 150;
        color.b = 150;
    } else {
        color.r = 255;
        color.g = 255;
        color.b = 255;
    }
    ((MHchar*) &self->char_0x024)->setTevKColor(0, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(1, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(2, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(3, GX_KCOLOR0, &color);
}

/* The program's per-mode condition query: mode 0 is the target/self height difference as a 0..4
 * level, mode 1 the aim-target flag, mode 2 the slot-free test and mode 3 a timer's sign. */
extern "C" u8 em020_condition_ck(_ENEMY_WORK* self, u32 mode) {
    f32 diff;

    switch ((u8) mode) {
    case 0:
        diff = self->vec_0x36C.y - self->pos.y;
        if (diff >= 1400.0f) {
            return 2;
        }
        if (diff >= 400.0f) {
            return 1;
        }
        if (diff <= -1400.0f) {
            return 4;
        }
        if (diff <= -400.0f) {
            return 3;
        }
        return 0;
    case 1:
        return em020_aim_target_ck(self) == 1;
    case 2:
        if (self->em020_0x328.timer_0x334 <= 0 && fn_8027DC64() == 1) {
            return 1;
        }
        return 0;
    case 3:
        return self->em020_0x328.timer_0x33E > 0;
    default:
        return 0;
    }
}

/* On the map that owns the two area models: reset the interpreter stack, then pick the stage table entry
 * and the model's area mode by area. */
extern "C" void em020_area_model_set(_ENEMY_WORK* self) {
    fn_8013A9F4(self);

    if (stage_map_kind_get(self->field_0x1E0) == 7) {
        switch (self->area_no) {
        case 2:
            fn_802B0A98(9, 1);
            fn_802D94C4(0);
            break;
        case 3:
            fn_802B0A98(10, 1);
            fn_802D94C4(1);
            break;
        }
    }
}

extern "C" {

/* 0x80375424: the low-HP predicate: area 3 and the damage numerator's share of the denominator at or below
 * 0.65. */
u32 em020_hp_ratio_ck(struct _ENEMY_WORK* self)
{
    f32 ratio;

    if (self->area_no != 3) {
        return 0;
    }
    ratio = (f32)self->field_0x7A0 / (f32)self->field_0x7A4;
    if (ratio <= 0.65f) {
        return 1;
    }
    return 0;
}

/* 0x80375494: on map 7 in area 3, request action 13 sub-state 3. */
void em020_map_area_action_set(struct _ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) {
        fn_80128AEC(self, 13, 3);
    }
}

} /* extern "C" */

/* 0x803754EC (0x8): Sets the accessor's flag word to 3 once it is installed. */
void Em020UserData::Attach()
{
    em_res_user_data_flag_set((struct _ENEMY_WORK*)this, 3);
}

extern "C" {

/* The em020 aim-target predicate: whether the `+0x836` bit 15 "aim target found" flag is set.
 * 0x803754F4 */
u32 em020_aim_target_ck(struct _ENEMY_WORK* self)
{
    return (self->flags_0x836 & 0x8000) != 0;
}

/* The em020 area-2 action-20 sub-state-30 predicate.
 * 0x8037550C */
u32 em020_area2_action20_ck(struct _ENEMY_WORK* self)
{
    if (self->team == 20 && self->area_no == 2 && self->stack_0x961[0] == 30) {
        return 1;
    }
    return 0;
}

/* 0x80375540: the area hit's damage-level gate: fold every part's damage level into `out->levels_0x01` and
 * copy out the facing angle and damage numerator. */
void em020_hit_info_get(struct _ENEMY_WORK* self, struct Em020HitInfo* out)
{
    if (self->area_no == 3) {
        out->hit_0x00 = 1;
        out->levels_0x01 = 0;
        if ((self->flags_0x836 & 2) != 0) {
            out->levels_0x01 |= 1;
        }
        if ((self->flags_0x836 & 0x8000) != 0) {
            out->levels_0x01 |= 2;
        }
        if (em_parts_damage_level_get(self, 3) >= 2) {
            out->levels_0x01 |= 4;
        }
        if (em_parts_damage_level_get(self, 5) >= 1) {
            out->levels_0x01 |= 8;
        }
        out->angle_0x02 = self->parts_0x838[0].value_0x04;
        out->damage_0x04 = self->field_0x7A0;
    } else {
        out->hit_0x00 = 0;
    }
}

/* 0x80375628 (0x1B8): the em020 status bits the HUD and network read: 0x80 aim target, then in area 3 0x40 the
 * alternate mode's cooldown, 0x08 low HP and 0x04/0x02/0x01 the head break stage, or in area 2 0x10/0x20 the
 * interpreter's progress. */
u8 em020_status_flags_get(_ENEMY_WORK* self)
{
    u8 flags = 0;

    if (self->active != 0 && self->team == 20) {
        if (em020_aim_target_ck(self) == 1)
            flags |= 0x80;
        if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) {
            if (em_alt_mode_ck(self) == 1 && self->em020_0x328.timer_0x334 <= 0)
                flags |= 0x40;
            if ((s32)self->field_0x7A0 <= (s32)(0.1f * (s32)self->field_0x7A4))
                flags |= 8;
            if ((self->flags_0x836 & 2) != 0)
                flags |= 4;
            else if ((s32)self->field_0x7A0 <= (s32)(0.65f * (s32)self->field_0x7A4) + ((EmPartLimit* (*)(_ENEMY_WORK*, u8))em_get_part_limit)(self, 0)->break_limit_0x04)
                flags |= 2;
            else
                flags |= 1;
        } else if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 2 && em020_aim_target_ck(self) == 0) {
            if (self->stack_0x961[0] >= 9)
                flags |= 0x10;
            else
                flags |= 0x20;
        }
    }
    return flags;
}

} /* extern "C" */

/* 0x803757E0 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em020UserData::~Em020UserData()
{
}

extern "C" {

/* The lobby's profile-part packets (`sendUserProfilePart` kind 0) all open with this 0x24-byte head: the packet kind,
 * a valid byte, the sender's id text and, for the addressed kinds, the receiver's id text.  size: 0x24 */
struct LbNetPacketHead {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 valid_0x01;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ char from_0x04[0xA];
    /* +0x0E */ u8 pad_0x0E[0x6];
    /* +0x14 */ char to_0x14[0xA];
    /* +0x1E */ u8 pad_0x1E[0x6];
}; /* size: 0x24 */

/* The head-only packets (kinds 12, 18 and 19): the head and a spare tail the send leaves out.  size: 0x30 */
struct LbNetShortPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u8 spare_0x24[0xC];
}; /* size: 0x30 */

/* Fills a packet head: the kind, the valid byte and the sender's id text. */
inline void lb_net_head_set(LbNetPacketHead* head, u8 kind)
{
    head->kind_0x00 = kind;
    head->valid_0x01 = 1;
    memcpy(head->from_0x04, em020_quest_page_ptr(), 0xA);
}


/* The position packet the network layer itself builds (kinds 0 and 5): the position, the facing, the area and the
 * player's three latch bytes.  size: 0x40 */
struct LbNetPositionPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ nw4r::math::VEC3 pos_0x24;
    /* +0x30 */ u16 angle_0x30;
    /* +0x32 */ u8 area_0x32;
    /* +0x33 */ u8 latch_0x33;
    /* +0x34 */ u8 board_0x34;
    /* +0x35 */ u8 meal_0x35;
    /* +0x36 */ u8 pad_0x36[0xA];

    LbNetPositionPacket() { VEC3_ctor(&pos_0x24); }
}; /* size: 0x40 */

/* The action packet (kind 1): the facing, the position as whole units, the action and its flags.  size: 0x38 */
struct LbNetActionPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u16 angle_0x24;
    /* +0x26 */ s16 x_0x26;
    /* +0x28 */ s16 y_0x28;
    /* +0x2A */ s16 z_0x2A;
    /* +0x2C */ u16 bits_0x2C;
    /* +0x2E */ u16 act_no_0x2E;
    /* +0x30 */ u8 act_kind_0x30;
    /* +0x31 */ u8 pad_0x31;
    /* +0x32 */ u16 act_flags_0x32;
    /* +0x34 */ u8 hidden_0x34;
    /* +0x35 */ u8 latch_0x35;
    /* +0x36 */ u8 area_0x36;
    /* +0x37 */ u8 pad_0x37;
}; /* size: 0x38 */

/* The quest board packet (kind 6): its two flags.  size: 0x2C */
struct LbNetBoardPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ s32 a_0x24;
    /* +0x28 */ s32 b_0x28;
}; /* size: 0x2C */

/* The meal packet (kind 7): the meal's three skills with their values, the area and the four bonuses.  size: 0x36 */
struct LbNetMealPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u8 skill_0x24[3];
    /* +0x27 */ u8 area_0x27;
    /* +0x28 */ s16 value_0x28[3];
    /* +0x2E */ u16 bonus_0x2E[4];
}; /* size: 0x36 */

/* The position-and-name packet (kind 9): the position, the facing, the area, the latch and the hunter name.
 * size: 0x40 */
struct LbNetPlacePacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ nw4r::math::VEC3 pos_0x24;
    /* +0x30 */ s16 angle_0x30;
    /* +0x32 */ s8 area_0x32;
    /* +0x33 */ u8 latch_0x33;
    /* +0x34 */ char name_0x34[0xA];
    /* +0x3E */ u8 pad_0x3E[0x2];

    LbNetPlacePacket() { VEC3_ctor(&pos_0x24); }
}; /* size: 0x40 */

/* The full-state packet (kinds 13 and 22): the facing and position, the action, the held item, the two names and
 * the three latch bytes.  size: 0x5C */
struct LbNetStatePacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ nw4r::math::VEC3 unused_0x24;
    /* +0x30 */ u16 angle_0x30;
    /* +0x32 */ s16 x_0x32;
    /* +0x34 */ s16 y_0x34;
    /* +0x36 */ s16 z_0x36;
    /* +0x38 */ u16 bits_0x38;
    /* +0x3A */ u16 item_0x3A;
    /* +0x3C */ s16 count_0x3C;
    /* +0x3E */ char partner_0x3E[0xA];
    /* +0x48 */ char name_0x48[0xA];
    /* +0x52 */ u16 act_no_0x52;
    /* +0x54 */ u8 act_kind_0x54;
    /* +0x55 */ u8 latch_0x55;
    /* +0x56 */ u8 board_0x56;
    /* +0x57 */ u8 meal_0x57;
    /* +0x58 */ u8 area_0x58;
    /* +0x59 */ u8 pad_0x59[0x3];

    LbNetStatePacket() { VEC3_ctor(&unused_0x24); }
}; /* size: 0x5C */

/* The trade-offer packet (kind 14): the facing and position, the action, the area, the item and the two names.
 * size: 0x48 */
struct LbNetTradePacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u16 angle_0x24;
    /* +0x26 */ s16 x_0x26;
    /* +0x28 */ s16 y_0x28;
    /* +0x2A */ s16 z_0x2A;
    /* +0x2C */ u16 act_no_0x2C;
    /* +0x2E */ u8 act_kind_0x2E;
    /* +0x2F */ u8 area_0x2F;
    /* +0x30 */ u16 item_0x30;
    /* +0x32 */ s16 count_0x32;
    /* +0x34 */ char partner_0x34[0xA];
    /* +0x3E */ char name_0x3E[0xA];
}; /* size: 0x48 */

/* The item packets (kinds 15 and 16): the result byte of the answer, the item and the count.  size: 0x2A */
struct LbNetItemPacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u8 result_0x24;
    /* +0x25 */ u8 pad_0x25;
    /* +0x26 */ u16 item_0x26;
    /* +0x28 */ u16 count_0x28;
}; /* size: 0x2A */

/* The notice packets (kinds 8 and 17): the two system words and the item list selection.  size: 0x2A */
struct LbNetNoticePacket {
    /* +0x00 */ LbNetPacketHead head;
    /* +0x24 */ u16 a_0x24;
    /* +0x26 */ u16 b_0x26;
    /* +0x28 */ u16 selection_0x28;
}; /* size: 0x2A */

/* The em020 quest-active predicate: whether the shared lobby block's `+0x03` byte is 1.
 * 0x8037583C */
u32 em020_quest_active_ck(void)
{
    return lobby_state_block.quest_active_0x0003 == 1;
}

/* Clears the shared lobby block's `+0x03` quest-active byte.
 * 0x80375858 */
void em020_quest_active_clear(void)
{
    lobby_state_block.quest_active_0x0003 = 0;
}

/* 0x8037586C (0x150): resets the lobby state block: whole outside a session, else everything but the session's
 * own records, then re-syncs the transfer flag and level. */
void lobby_state_reset(void)
{
    LobbyTimerBlock* block = &lobby_state_block;
    u8* row;
    s32 i;

    if ((u32)NetCtrlWk::isSessionFlagSet() != 1 && system_w.field_0x8b1 == 0) {
        memset(block, 0, sizeof(LobbyTimerBlock));
    } else {
        block->step_0x0000 = 0;
        block->substep_0x0001 = 0;
        block->field_0x0002 = 0;
        block->quest_active_0x0003 = 0;
        memset(&block->peer_count_0x0004, 0, 0x484);
        memset(block->block_0x0488, 0, sizeof(block->block_0x0488));
        memset(block->block_0x11D4, 0, sizeof(block->block_0x11D4));
        i = 0;
        row = block->rows_0x13E6[0];
        do {
            memset(row, 0, 4);
            row += 0x1F;
            i++;
        } while (i < 12);
        block->field_0x155A = 0;
        block->request_0x155C = 0;
        block->busy_0x155D = 0;
        memset(block->block_0x1560, 0, sizeof(block->block_0x1560));
        block->card_sent_0x1684 = 0;
        block->field_0x1688 = 0;
        block->move_frames_0x168C = 0;
        block->timeout_0x168E[0] = 0;
        block->timeout_0x168E[1] = 0;
        block->timeout_0x168E[2] = 0;
        block->field_0x1694 = 0;
        block->result_0x1695 = 0;
        block->request_0x1696 = 0;
        block->field_0x1698[0] = 0;
        block->field_0x1698[1] = 0;
        block->field_0x1698[2] = 0;
        block->field_0x1698[3] = 0;
        block->field_0x1698[4] = 0;
        block->field_0x1698[5] = 0;
        block->field_0x1698[6] = 0;
        block->field_0x1698[7] = 0;
        block->field_0x1698[8] = 0;
        block->field_0x16AC = 0;
    }
    syncNetTransferFlag();
    applyTransferLevel();
}

/* Writes the em020 one-byte `.sbss` flag.
 * 0x803759BC */
void em020_unknown_flag_set(s8 value)
{
    lobby_net_err_confirm = value;
}

/* 0x803759C4 (0x1D8): draws the lobby's network error window from `lobby_w`'s error message: the message (or the
 * "ending the network" notice), the confirm button once the window may close, and the layer error's code. */
void lobby_net_err_draw(void)
{
    _MH_VEC2 screen;
    char text[0x20];
    s16 x;
    s32 width = 590;

    if (lobby_w.net_err_0x168 != NULL) {
        get_ScreenSize(&screen);
        x = (screen.x - width) / 2.0f;
        set_zmode(0, 0, 0);
        set_blendmode(4, 5, 1);
        put_frame_dialog(x, 0x28, 590, 400, 0, 0x14120AF0);
        if (lobby_w.net_err_0x168 != NULL) {
            MH3DispErrorString(x + 0x18, 0x44, (s8*)lobby_w.net_err_0x168);
            if (lobby_net_err_confirm == 1) {
                x += 0x20E;
                if (get_option_cfg(7) != 0)
                    put_button_icon(x, 0x178, 0x20, -1, 2, 0);
                else
                    put_button_icon(x, 0x178, 0x20, -1, 0, 0);
            }
            if (net_layer_error_message > 0) {
                flfntSetSize(0x16, 0x16);
                flfntSetColor(-1);
                flfntSetPos((s16)((screen.x - 220.0f) / 2.0f), 0x168);
                sprintf(text, "%s = %d", MH3GetErrorString2(0x33), net_layer_error_message);
                flfntPrintf(text);
            }
        } else {
            MH3DispErrorString(x + 0x18, 0x44,
                               (s8*)"<BODY>なにかのエラー。<BR><BR><BODY>ネットワーク終了処理中です。<BR><BODY>しばらくお待ちください。<END>");
        }
    }
}

/* 0x80375B9C (0x44): leaves the networked lobby: releases the remote players and stops the update. */
void lobby_net_leave(void)
{
    releaseRemotePlayerParts();
    lobby_state_block.quest_active_0x0003 = 0;
    lobby_state_block.step_0x0000 = 0;
    system_w.field_0x8b1 = 1;
}

/* 0x80375BE0 (0x5C): the jump error message of error code `code` (-2..-4), or NULL. */
char* lobby_net_jump_msg_get(s8 code)
{
    switch (code) {
    case -1:
        return NULL;
    case -2:
        return jump_err_msg_str->msg_0x04;
    case -3:
        return jump_err_msg_str->msg_0x08;
    case -4:
        return jump_err_msg_str->msg_0x00;
    default:
        return NULL;
    }
}

/* 0x80375C3C (0xF4): carries out the error window's choice `choice`: 1 leaves the lobby, 2 moves to the offline area
 * entry, 3 to the online one; 1 when handled. */
u32 lobby_net_leave_select(s8 choice)
{
    switch (choice) {
    case 1:
        lobby_w.field_0x0B0 = 3;
        return 1;
    case 2: {
        u8 map;
        u16 angle;

        lobby_area_change_req();
        map = get_now_mapno();
        angle = stage_entry_angle_get(map, 0);
        lobby_player_place(0, stage_entry_pos_get(map, 0), angle);
        lobby_net_leave();
        return 1;
    }
    case 3: {
        u16 angle;
        u8 map;

        lobby_area_change_net_req();
        map = get_now_mapno();
        angle = stage_entry_angle_get(map, 1);
        lobby_player_place_ex(1, stage_entry_pos_get(map, 1), angle, 5);
        lobby_net_leave();
        return 1;
    }
    default:
        return 0;
    }
}

/* Loads this player's own hunter card and the block's own id and name from the network's self record. */
inline void lb_self_card_set(LobbyTimerBlock* block)
{
    s8 slot = my_player_no();
    _PLW* plw = &((_PLW*)get_move_work_adrs(2))[slot];
    const NetPeerRec* self = (const NetPeerRec*)getSelfName();

    if (self != NULL) {
        LbQuestPage* card = &lobby_hunter_cards[slot];

        card->active_0x000 = 1;
        card->slot_0x001 = slot;
        card->loaded_0x002 = 0;
        card->player_0x124 = plw;
        strcpy(card->id_0x03, self->id_0x00.text_0x00);
        strcpy(card->name_0x0D, self->name_0x0A.text_0x00);
        strcpy(block->self_id_0x16B0, self->id_0x00.text_0x00);
        strcpy(block->self_name_0x16BB, self->name_0x0A.text_0x00);
        strcpy(plw->hunter_0x5CA.id_0x5DB, self->id_0x00.text_0x00);
        buildNetUserProfile((NetUserProfile*)card->card_0x024);
    }
}

/* Starts the roster fetch: the lists straight from the network outside a session, else the community news. */
inline void lb_roster_fetch_start(LobbyTimerBlock* block)
{
    lobby_w.field_0x176 = 0;
    if (system_w.field_0x8b1 == 0) {
        NetCtrlWk::copyRosterLists((NetRosterListView*)&block->friends_0x16D0, (NetRecentListView*)&block->recent_0x1DDC);
        lobby_friend_list_sort(&block->friends_0x16D0);
        block->step_0x0000 = 4;
    } else {
        if (getMessageTableState() == 1)
            requestCommunityNews(&block->request_0x1696);
        else
            block->request_0x1696 = 1;
        block->step_0x0000++;
    }
    block->substep_0x0001 = 0;
}

/* Holds the lobby's input while a network window is up. */
inline void lb_net_input_hold(void)
{
    lobby_w.active_0x008 = 1;
    Psw[0].button_0x2C0.pressed_0x04 = 0;
    Psw[0].button_0x2C0.held_0x14 = 0;
    lb_pad_input_flush(&Psw[0]);
}

/* 0x80375D30 (0xBC0): the lobby's network update: steps the lobby state block through the session join (cards,
 * roster, news, big data and notices), raises the lobby notice, then runs the error and sub-error windows and the
 * friend-list refresh. */
void lobby_net_update(void)
{
    LobbyTimerBlock* block;
    NetPeerList* peers;
    void* buf;
    s32 i;
    LbQuestPage* card;

    if (isNetCityMode() != 0) {
        block = &lobby_state_block;
        peers = (NetPeerList*)&block->peer_count_0x0004;
        switch (block->step_0x0000) {
        case 0:
            buf = work_mem_alloc(0x2C);
            NetCtrlWk::copyPeerList(peers);
            i = 0;
            card = lobby_hunter_cards;
            do {
                memset(card, 0, sizeof(LbQuestPage));
                card++;
                i++;
            } while (i < 10);
            if (system_w.field_0x8b1 == 0) {
                lb_self_card_set(block);
            } else {
                _PLW* players = (_PLW*)get_move_work_adrs(2);

                if (isLayerReady() == 0) {
                    lb_self_card_set(block);
                } else {
                    s32 count = importLayerPeers(peers);
                    NetPeerRec* peer;

                    i = 0;
                    card = lobby_hunter_cards;
                    peer = peers->peers_0x004;
                    for (; i < count; i++) {
                        card->active_0x000 = 1;
                        card->slot_0x001 = i;
                        card->loaded_0x002 = 0;
                        card->player_0x124 = players;
                        strcpy(card->id_0x03, peer->id_0x00.text_0x00);
                        strcpy(card->name_0x0D, peer->name_0x0A.text_0x00);
                        if (i == (s8)my_player_no()) {
                            strcpy(block->self_id_0x16B0, peer->id_0x00.text_0x00);
                            strcpy(block->self_name_0x16BB, peer->name_0x0A.text_0x00);
                            strcpy(players->hunter_0x5CA.id_0x5DB, peer->id_0x00.text_0x00);
                            buildNetUserProfile((NetUserProfile*)card->card_0x024);
                        } else {
                            lobby_net_peer_load(players, (NetPeerRec*)getMessageEntry(i), i, 0);
                        }
                        card++;
                        players++;
                        peer++;
                    }
                }
            }
            work_mem_free(buf);
            block->move_frames_0x168C = 0x14;
            block->timeout_0x168E[0] = get_server_big_data_timeout_element(3);
            block->timeout_0x168E[1] = get_server_big_data_timeout_element(4);
            block->timeout_0x168E[2] = get_server_big_data_timeout_element(1);
            block->step_0x0000++;
            if ((s8)lobby_w.field_0x176 != 2)
                lb_roster_fetch_start(block);
            break;
        case 1:
            switch ((s8)lobby_w.field_0x176) {
            case 2:
            case 0:
                break;
            case 1:
                lb_roster_fetch_start(block);
                break;
            default:
                setErrorCode(5);
                break;
            }
            break;
        case 2:
            if (block->request_0x1696 != 0) {
                if (block->request_0x1696 == 1) {
                    if (getMessageTableState() == 1)
                        requestCommunityBlockList(&block->request_0x1696);
                    else
                        block->request_0x1696 = 1;
                    block->step_0x0000++;
                    block->substep_0x0001 = 0;
                } else {
                    setErrorCode(5);
                }
            }
            break;
        case 3:
            if (block->request_0x1696 != 0) {
                if (block->request_0x1696 == 1) {
                    if (getMessageTableState() == 1) {
                        NetCtrlWk::copyRosterLists((NetRosterListView*)&block->friends_0x16D0,
                                                   (NetRecentListView*)&block->recent_0x1DDC);
                        lobby_friend_list_sort(&block->friends_0x16D0);
                    }
                    block->step_0x0000++;
                    block->substep_0x0001 = 0;
                } else {
                    setErrorCode(5);
                }
            }
            break;
        case 4:
            if (getInstance_() != NULL)
                sendReqServerTime(getInstance_());
            requestFriendSync(&block->request_0x1696);
            clearMessageTableState();
            block->step_0x0000++;
            block->substep_0x0001 = 0;
            break;
        case 5:
            if (block->request_0x1696 != 0) {
                if (block->request_0x1696 == 1) {
                    block->step_0x0000++;
                    block->substep_0x0001 = 0;
                    if ((u32)isLayerReady() == 1) {
                        clearBigDataTimeout();
                        NetCtrlWk::pollBigDataFetch();
                    }
                } else {
                    setErrorCode(5);
                }
            }
            break;
        case 6:
            if ((u32)isLayerReady() != 1 || NetCtrlWk::pollBigDataFetch() != 0)
                block->step_0x0000++;
            break;
        case 7:
            if ((u32)isLayerReady() != 1 || NetCtrlWk::pollNoticeFetch() != 0) {
                startRosterFetch(&block->request_0x1696);
                block->step_0x0000++;
            }
            break;
        case 8:
            if (block->request_0x1696 != 0) {
                if (block->request_0x1696 != 1)
                    setErrorCode(5);
                NetCtrlWk::copyPeerList((NetPeerList*)&lobby_state_block.peer_count_0x0004);
                if (lobby_state_block.card_sent_0x1684 != 1) {
                    _PLW* players = (_PLW*)get_move_work_adrs(2);

                    lobby_net_state_send(&players[(s8)my_player_no()], NULL, 1);
                }
                block->quest_active_0x0003 = 1;
                system_w.leave_flag_0x7d6 = 0;
                block->step_0x0000++;
            }
            break;
        case 9:
            if ((u32)isLayerReady() == 1)
                NetCtrlWk::copyPeerList(peers);
            break;
        }
        if (block->request_0x155C != 0 && block->busy_0x155D == 0) {
            lobby_notice_raise(block, 0);
            block->request_0x155C = 0;
        }
        if (system_w.field_0x865 != 1) {
            em020_unknown_flag_set(0);
            lobby_w.field_0x163 = NetCtrlWk::getLobbyReadyByte();
            switch (lobby_w.field_0x163) {
            case 0:
                lobby_w.err_ack_0x16C = 0;
                break;
            case 1:
                lobby_w.active_0x008 = 1;
                lobby_w.net_err_0x168 = NULL;
                subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
                Psw[0].button_0x2C0.pressed_0x04 = 0;
                Psw[0].button_0x2C0.held_0x14 = 0;
                lobby_w.err_ack_0x16C = 0;
                fade_reset(0);
                kbd_close_call();
                menu_work.slot[0].field_0x32D = 0;
                return;
            case 2:
                if (lobby_w.err_ack_0x16C == 0) {
                    lobby_w.active_0x008 = 1;
                    if (get_fade_stat(0) != 2) {
                        if (lobby_w.net_wait_0x164 < 0x3C) {
                            if (file_loading_ck(0, 0) != 1)
                                lobby_w.net_wait_0x164++;
                        } else {
                            em020_unknown_flag_set(1);
                            if ((Psw[0].button_0x2C0.pressed_0x04 & 0x10) != 0) {
                                net_layer_error_message = 0;
                                lobby_w.net_wait_0x164 = 0x14;
                                lobby_w.err_ack_0x16C = 1;
                                sysSE_req(0);
                            }
                        }
                    }
                } else if (lobby_w.net_wait_0x164 > 0) {
                    if (file_loading_ck(0, 0) != 1)
                        lobby_w.net_wait_0x164--;
                } else {
                    lobby_w.field_0x0B0 = 2;
                    system_w.field_0x7ce = 3;
                }
                lobby_w.net_err_0x168 = NetCtrlWk::getErrorMessage();
                if (lobby_w.net_err_0x168 != NULL)
                    subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
                Psw[0].button_0x2C0.pressed_0x04 = 0;
                Psw[0].button_0x2C0.held_0x14 = 0;
                return;
            }
            if (lobby_w.refresh_step_0x177 == 0) {
                switch (lobby_w.sub_err_step_0x16E) {
                case 0:
                    lobby_w.field_0x16F = NetCtrlWk::hasSubError();
                    if (lobby_w.field_0x16F == 0)
                        break;
                    lobby_w.active_0x008 = 1;
                    lobby_w.sub_err_wait_0x170 = 0;
                    lobby_w.sub_err_step_0x16E++;
                    Psw[0].button_0x2C0.pressed_0x04 = 0;
                    Psw[0].button_0x2C0.held_0x14 = 0;
                    fade_reset(0);
                    kbd_close_call();
                    menu_work.slot[0].field_0x32D = 0;
                case 1:
                    if (lobby_w.field_0x16F != 0) {
                        lobby_w.net_err_0x168 = get_network_sub_error_msg();
                        if (lobby_w.net_err_0x168 != NULL)
                            subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
                        lobby_w.active_0x008 = 1;
                        if (get_fade_stat(0) != 2) {
                            if (lobby_w.sub_err_wait_0x170 < 0x3C) {
                                if (file_loading_ck(0, 0) == 1)
                                    break;
                                lobby_w.sub_err_wait_0x170++;
                            } else {
                                em020_unknown_flag_set(1);
                                if ((Psw[0].button_0x2C0.pressed_0x04 & 0x10) != 0) {
                                    net_layer_error_message = 0;
                                    lobby_w.sub_err_step_0x16E++;
                                    lobby_w.sub_err_wait_0x170 = 0x14;
                                    sysSE_req(0);
                                }
                            }
                        }
                        Psw[0].button_0x2C0.pressed_0x04 = 0;
                        Psw[0].button_0x2C0.held_0x14 = 0;
                        lb_pad_input_flush(&Psw[0]);
                        return;
                    }
                    lobby_w.sub_err_step_0x16E = 0;
                    NetCtrlWk::clearSubError();
                    lobby_w.active_0x008 = 0;
                    Psw[0].button_0x2C0.pressed_0x04 = 0;
                    Psw[0].button_0x2C0.held_0x14 = 0;
                    lb_pad_input_flush(&Psw[0]);
                    return;
                case 2:
                    if (lobby_w.sub_err_wait_0x170 > 0) {
                        if (file_loading_ck(0, 0) != 1)
                            lobby_w.sub_err_wait_0x170--;
                        return;
                    }
                    if (NetCtrlWk::getLayerModeByte() == 2) {
                        u8 map;
                        u16 angle;

                        lobby_area_change_req();
                        map = get_now_mapno();
                        angle = stage_entry_angle_get(map, 0);
                        lobby_player_place(0, stage_entry_pos_get(map, 0), angle);
                        lobby_net_leave();
                        NetCtrlWk::setLayerModeByte(0);
                    }
                    lobby_w.sub_err_step_0x16E = 0;
                    NetCtrlWk::clearSubError();
                    lobby_w.active_0x008 = 0;
                    Psw[0].button_0x2C0.pressed_0x04 = 0;
                    Psw[0].button_0x2C0.held_0x14 = 0;
                    lb_pad_input_flush(&Psw[0]);
                    return;
                }
            }
            switch (lobby_w.refresh_step_0x177) {
            case 0: {
                _PLW* players = (_PLW*)get_move_work_adrs(2);

                if (isLayerReady() != 0 && NetCtrlWk::getLayerModeByte() == 1) {
                    if (lobby_w.state_0x000 == 0 && (players->field_0xB01 & 7) == 0) {
                        if (((s32 (*)(s8*))NetCtrlWk::requestListRefreshAlias)(&lobby_w.refresh_result_0x178) == 0)
                            lobby_w.refresh_step_0x177 = 2;
                        else
                            lobby_w.refresh_step_0x177++;
                    } else {
                        setErrorCode(4);
                        NetCtrlWk::setLayerModeByte(0);
                        lobby_w.refresh_step_0x177 = 0;
                    }
                    lb_net_input_hold();
                    return;
                }
                break;
            }
            case 1:
                if (lobby_w.refresh_result_0x178 != 0) {
                    if (lobby_w.refresh_result_0x178 < 0) {
                        lobby_w.refresh_step_0x177 = 2;
                    } else {
                        NetCtrlWk::setSubError(5);
                        NetCtrlWk::setLayerModeByte(2);
                        lobby_w.refresh_step_0x177 = 0;
                    }
                    lb_net_input_hold();
                    return;
                }
                break;
            case 2:
                setErrorCode(4);
                NetCtrlWk::setLayerModeByte(0);
                lobby_w.refresh_step_0x177 = 0;
                lb_net_input_hold();
                return;
            }
            if (getTermsCheckDone() == 1) {
                NetCtrlWk::setSubError(0x1D);
                acknowledgeTermsCheck();
                lb_net_input_hold();
            }
        }
    }
}

/* 0x803768F0 (0x8): the network singleton. */
PatInterface* getInstance_(void)
{
    return PatInterface::mpInstance;
}

/* 0x803768F8 (0x6C): whether the lobby is busy with the network (server select, a lobby-ready byte, a sub error or a
 * pending leave). */
u32 lobby_net_busy_ck(void)
{
    if (isServerSelectState() == 1)
        return 0;
    if (lobby_w.field_0x163 == 0 && lobby_w.field_0x16F == 0 && lobby_w.field_0x0B0 == 0)
        return 0;
    return 1;
}

/* 0x803769D4 (0x64): whether `plw` is the session master: the game is ready, the layer is up and the master test
 * answers 1. */
u32 lobby_net_master_ck(_PLW* plw)
{
    if (game_ready_ck() == 0)
        return 0;
    if (isLayerReady() == 0)
        return 0;
    return Pl_master_ck(plw) == 1;
}

/* 0x80376A38 (0x58): rebuilds this player's profile into its hunter card and submits it. */
void lobby_net_profile_submit(void)
{
    NetUserProfile* profile = (NetUserProfile*)lobby_hunter_cards[(s8)my_player_no()].card_0x024;

    if ((u32)buildNetUserProfile(profile) == 1)
        submitUserProfile(profile);
}

#pragma dont_inline on
/* 0x80376A90 (0xA8): publishes `plw`'s position (action 5 while moving, 0 at rest; 0x80 when hidden) unless the lobby
 * holds the network. */
void lobby_net_position_send(_PLW* plw, u8 moving)
{
    LbNetPositionPacket packet;

    if (isNetCityMode() != 0 && lobby_w.active_0x008 != 1) {
        s32 action = moving == 0 ? 0 : 5;

        if (lobby_hidden_flag == 1)
            action = (u8)(action | 0x80);
        sendUserPosition(action, &plw->vec_0x03C.x, &plw->param_0x54, plw->area_0x16, plw->field_0xB00,
                         plw->field_0xB01, plw->field_0xB02);
    }
}
#pragma dont_inline off

/* 0x80376964 (0x4): raises the lobby notice the update requests (an empty body). */
void lobby_notice_raise(LobbyTimerBlock* block, s32 mode)
{
}

/* The em020 quest-page pointer: the address of the shared quest-page block's fourth record.
 * 0x80376968 */
u8* em020_quest_page_ptr(void)
{
    return lobby_hunter_cards[0].key_0x03;
}

/* Clears the nine quest-page records after the first in the shared quest-page block.
 * 0x80376978 */
void em020_quest_pages_clear(void)
{
    s32 i;

    for (i = 1; i < 10; i++) {
        memset(&lobby_hunter_cards[i], 0, sizeof(LbQuestPage));
    }
}

/* 0x80376B6C (0x1F0): sends `plw`'s action and position as profile part 1 (kind 1); unless `force`, only when the
 * action matters to the other players. */
void lobby_net_action_send(_PLW* plw, u8 force, u16 flags)
{
    LbNetActionPacket packet;

    if (force == 0) {
        if (Pl_act_ck(plw, 0, 0) == 1 && (flags & 0x8000) == 0)
            return;
        if (plw->field_0x00A == 1 && plw->act_no <= 4)
            return;
    }
    memset(&packet, 0, sizeof(packet));
    packet.head.kind_0x00 = 1;
    packet.head.valid_0x01 = 1;
    memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
    packet.act_kind_0x30 = plw->field_0x00A;
    packet.act_no_0x2E = plw->act_no;
    packet.bits_0x2C = plw->field_0x0B6;
    packet.latch_0x35 = plw->field_0xB00;
    packet.area_0x36 = plw->area_0x16;
    packet.angle_0x24 = plw->field_0x0A8;
    packet.x_0x26 = plw->vec_0x03C.x;
    packet.y_0x28 = plw->vec_0x03C.y;
    packet.z_0x2A = plw->vec_0x03C.z;
    packet.act_flags_0x32 = flags & 0x7FFF;
    if (lobby_hidden_flag == 1)
        packet.hidden_0x34 = 1;
    else
        packet.hidden_0x34 = 0;
    if (plw->field_0x00A == 0) {
        switch (plw->act_no) {
        case 3:
        case 6:
            packet.x_0x26 = plw->target_pos_0x090.x;
            packet.z_0x2A = plw->target_pos_0x090.z;
            break;
        case 4:
        case 8:
        case 9:
        case 10:
        case 11:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
            packet.x_0x26 = plw->vec_0x03C.x;
            packet.z_0x2A = plw->vec_0x03C.z;
            break;
        }
    }
    sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
}

/* 0x80376D5C (0x50): sends `plw`'s action (`lobby_net_action_send`) while the layer is up. */
void lobby_net_action_update(_PLW* plw, u16 flags)
{
    if (isLayerReady() != 0)
        lobby_net_action_send(plw, 0, flags);
}

/* 0x80376DAC (0x174): applies a received action packet (kind 1) to the remote player `plw`: its walk target, latch
 * and area, then the action itself through the NPC act setter. */
void lobby_net_action_recv(_PLW* plw, const LbNetActionPacket* packet)
{
    if (packet->hidden_0x34 == 0) {
        plw->field_0x0A8 = packet->angle_0x24;
        plw->field_0xB04 = 1;
        plw->field_0xB00 = packet->latch_0x35;
        plw->target_pos_0x090.x = packet->x_0x26;
        plw->target_pos_0x090.y = packet->y_0x28;
        plw->target_pos_0x090.z = packet->z_0x2A;
        plw->area_0x16 = packet->area_0x36;
        if ((packet->act_flags_0x32 & 4) != 0)
            lb_npc_walk_blend_set(plw, 0);
        else
            lb_npc_walk_blend_set(plw, 0x14);
        if ((packet->act_flags_0x32 & 8) != 0)
            plw->field_0x058 = plw->field_0x0A8;
        if (packet->act_kind_0x30 == 0 && packet->act_no_0x2E == 3 && packet->act_no_0x2E != plw->act_no) {
            lb_npc_act_req(plw, 0, 3, packet->act_flags_0x32);
            return;
        }
        if (lobby_w.field_0x000 != 0x19 || packet->act_kind_0x30 != 0 || (u32)(packet->act_no_0x2E - 0x15) > 3) {
            lb_npc_act_set(plw, packet->act_kind_0x30, packet->act_no_0x2E, packet->act_flags_0x32);
            plw->field_0x0B6 = packet->bits_0x2C;
        }
    }
}

/* 0x80376F20 (0x138): loads the peer record `peer` into hunter card `slot` and its player work `plw` (card, names,
 * equipment and parts), once. */
void lobby_net_peer_load(_PLW* plw, const NetPeerRec* peer, u8 slot, u8 notify)
{
    LbQuestPage* card;
    NetUserProfile* profile;

    if (plw != NULL) {
        card = &lobby_hunter_cards[slot];
        if (card->loaded_0x002 == 0) {
            card->active_0x000 = 1;
            card->slot_0x001 = slot;
            card->player_0x124 = plw;
            strcpy(card->id_0x03, peer->id_0x00.text_0x00);
            strcpy(card->name_0x0D, peer->name_0x0A.text_0x00);
            profile = (NetUserProfile*)card->card_0x024;
            memcpy(profile, &peer->blob_0x20, 0x100);
            decodePlayerCard((const u8*)profile, (NetPlayerCard*)plw);
            strcpy(plw->hunter_0x5CA.name_0x5CA, peer->name_0x0A.text_0x00);
            strcpy(plw->hunter_0x5CA.id_0x5DB, peer->id_0x00.text_0x00);
            plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
            pl_net_slot_bind(plw, slot);
            plw->set_valid |= 1;
            plw->set_valid |= 2;
            plw->set_valid |= 4;
            plw->set_valid |= 8;
            plw->set_valid |= 0x10;
            plw->set_valid |= 0x20;
            plw->set_valid |= 0x40;
            pl_equip_models_set(plw, profile->equip_0x10[0], profile->equip_0x10[1], profile->equip_0x10[2]);
            ((void (*)(_PLW*))pl_parts_refresh)(plw);
            if (plw->physics_0x13C != NULL)
                pl_physics_rebind(plw->physics_0x13C, plw);
            card->loaded_0x002 = 1;
        }
    }
}

/* 0x80377058 (0xB8): releases hunter card `slot` and, when its player work is still bound to it, that player's
 * parts and model. */
void lobby_net_peer_release(u8 slot)
{
    _PLW* plw;

    if (lobby_hunter_cards[slot].active_0x000 != 0) {
        plw = (_PLW*)get_move_work_adrs(2);
        if (plw != NULL) {
            plw = &plw[slot];
            if (plw->slot_active != 0 && plw == lobby_hunter_cards[slot].player_0x124) {
                parts_mdl_release_all(plw);
                pl_model_release(plw);
            }
            memset(&lobby_hunter_cards[slot], 0, sizeof(LbQuestPage));
        }
    }
}

/* 0x80377110 (0xB0): releases the parts and model of remote player slots 1..9 whose hunter card still points at
 * their player work, and clears the cards. */
void releaseRemotePlayerParts(void)
{
    LbQuestPage* card;
    _PLW* plw;
    s32 i;

    plw = (_PLW*)get_move_work_adrs(2);
    if (plw != NULL) {
        plw++;
        i = 1;
        card = &lobby_hunter_cards[1];
        do {
            if (card->active_0x000 != 0) {
                if (plw->slot_active != 0 && plw == card->player_0x124) {
                    parts_mdl_release_all(plw);
                    pl_model_release(plw);
                }
                memset(card, 0, sizeof(LbQuestPage));
            }
            card++;
            i++;
            plw++;
        } while (i < 10);
    }
}

/* 0x803771C0 (0xE8): applies a received position (kind 0 or 5) to the remote player `plw`: a walk for a moving player
 * in its own area, else a jump to the position. */
void lobby_net_position_recv(_PLW* plw, const LbNetPositionPacket* packet, u8 kind)
{
    u8 area;

    plw->field_0xB04 = 1;
    plw->field_0xB00 = packet->latch_0x33;
    plw->field_0xB01 = packet->board_0x34;
    plw->field_0xB02 = packet->meal_0x35;
    area = packet->area_0x32;
    plw->area_0x16 = area;
    if (kind == 0 && area == packet->area_0x32) {
        if (lb_npc_act_busy_ck(plw->field_0x00A, plw->act_no) == 0) {
            copyVec3((nw4r::math::VEC3*)&plw->target_pos_0x090, (const nw4r::math::VEC3*)&packet->pos_0x24);
            lb_npc_walk_blend_set(plw, lobby_state_block.move_frames_0x168C);
        }
    } else {
        copyVec3((nw4r::math::VEC3*)&plw->vec_0x03C, (const nw4r::math::VEC3*)&packet->pos_0x24);
        copyVec3((nw4r::math::VEC3*)&plw->vec_0x048, (const nw4r::math::VEC3*)&plw->vec_0x03C);
        copyVec3((nw4r::math::VEC3*)&plw->target_pos_0x090, (const nw4r::math::VEC3*)&plw->vec_0x03C);
        plw->field_0x0A8 = plw->field_0x058 = packet->angle_0x30;
        lb_npc_walk_blend_set(plw, 0);
    }
}

/* 0x803772A8 (0xA0): sends the quest board's two flags (kind 6). */
void quest_board_flags_send(u8 a, u8 b)
{
    LbNetBoardPacket packet;
    LbNetPacketHead* head;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        head = &packet.head;
        lb_net_head_set(head, 6);
        packet.a_0x24 = a;
        packet.b_0x28 = b;
        sendUserProfilePart(0, (const u8*)head, sizeof(packet));
    }
}

/* 0x80377348 (0xE0): sends the served meal's skills and bonuses to the area `area` (kind 7). */
void meal_result_send(u8 area)
{
    LbNetMealPacket packet;
    LbNetPacketHead* head;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        head = &packet.head;
        lb_net_head_set(head, 7);
        packet.area_0x27 = area;
        packet.skill_0x24[0] = lb_param_w.flag_0x0C[0];
        packet.value_0x28[0] = lb_param_w.value_0x10[0];
        packet.skill_0x24[1] = lb_param_w.flag_0x0C[1];
        packet.value_0x28[1] = lb_param_w.value_0x10[1];
        packet.skill_0x24[2] = lb_param_w.flag_0x0C[2];
        packet.value_0x28[2] = lb_param_w.value_0x10[2];
        packet.bonus_0x2E[0] = lb_param_w.value_0x16;
        packet.bonus_0x2E[1] = lb_param_w.value_0x18;
        packet.bonus_0x2E[2] = lb_param_w.value_0x1A;
        packet.bonus_0x2E[3] = lb_param_w.value_0x1C;
        sendUserProfilePart(0, (const u8*)head, sizeof(packet));
    }
}

/* 0x80377428 (0x80): takes the host's meal (kind 7) when none has arrived yet and it is for this area. */
void lobby_net_meal_recv(const LbNetMealPacket* packet)
{
    if (lobby_w.meal_received_0x14A == 0 && lobby_w.meal_area_0x149 == packet->area_0x27) {
        lobby_w.meal_received_0x14A = 1;
        lobby_w.meal_skill_0x14B[0] = packet->skill_0x24[0];
        lobby_w.meal_value_0x14E[0] = packet->value_0x28[0];
        lobby_w.meal_skill_0x14B[1] = packet->skill_0x24[1];
        lobby_w.meal_value_0x14E[1] = packet->value_0x28[1];
        lobby_w.meal_skill_0x14B[2] = packet->skill_0x24[2];
        lobby_w.meal_value_0x14E[2] = packet->value_0x28[2];
        lobby_w.meal_bonus_0x154[0] = packet->bonus_0x2E[0];
        lobby_w.meal_bonus_0x154[1] = packet->bonus_0x2E[1];
        lobby_w.meal_bonus_0x154[2] = packet->bonus_0x2E[2];
        lobby_w.meal_bonus_0x154[3] = packet->bonus_0x2E[3];
    }
}

/* 0x803774A8 (0x30): takes a notice packet (kind 8): the two system words and the item list selection. */
void lobby_net_notice_recv(const LbNetNoticePacket* packet)
{
    system_w.field_0x18[2] = packet->a_0x24;
    system_w.field_0x18[3] = packet->b_0x26;
    lobby_w.item_list_selection_0x162 = packet->selection_0x28;
}

#pragma dont_inline on
/* 0x803774D8 (0xCC): sends `plw`'s placement (kind 9): the position, the facing, the area, the latch and the name. */
void lobby_net_place_send(_PLW* plw, s8 area, nw4r::math::VEC3* pos, s16 angle)
{
    LbNetPlacePacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        packet.head.kind_0x00 = 9;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        copyVec3(&packet.pos_0x24, pos);
        packet.angle_0x30 = angle;
        packet.area_0x32 = area;
        packet.latch_0x33 = plw->field_0xB00;
        strcpy(packet.name_0x34, (const char*)plw->name_0xB05);
        sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
    }
}
#pragma dont_inline off

/* 0x803775D8 (0x8C): places the remote player `plw` from a placement packet (kind 9). */
void lobby_net_place_recv(_PLW* plw, const LbNetPlacePacket* packet)
{
    plw->field_0xB00 = packet->latch_0x33;
    plw->area_0x16 = packet->area_0x32;
    copyVec3(&plw->vec_0x03C, &packet->pos_0x24);
    copyVec3(&plw->vec_0x048, &plw->vec_0x03C);
    copyVec3(&plw->target_pos_0x090, &plw->vec_0x03C);
    plw->field_0x0A8 = plw->field_0x058 = (u16)packet->angle_0x30;
    strcpy((char*)plw->name_0xB05, packet->name_0x34);
    lb_npc_walk_blend_set(plw, 0);
}

/* 0x80377664 (0x10): sends profile part 1 (the whole profile). */
s32 lobby_net_profile_part1_send(void)
{
    return sendUserProfilePart(1, NULL, 0);
}

/* 0x80377674 (0x154): applies the hunter card of `plw`'s slot to `plw`: the profile bytes, the colour words and the
 * nine equipment records. */
void lobby_net_card_apply(_PLW* plw, const LbNetPacketHead* packet)
{
    NetUserProfile* profile = (NetUserProfile*)lobby_hunter_cards[plw->chunk_ofs].card_0x024;

    plw->user_profile_0x1C = profile->field_0x03;
    plw->field_0x25C[4] = profile->field_0x02;
    plw->field_0x258[0] = (profile->word_0x08 & 0xFF000000) >> 24;
    plw->field_0x258[1] = (profile->word_0x08 & 0xFF0000) >> 16;
    plw->field_0x258[2] = (profile->word_0x08 & 0xFF00) >> 8;
    plw->field_0x258[3] = profile->word_0x08;
    plw->field_0x25C[0] = (profile->word_0x0C & 0xFF000000) >> 24;
    plw->field_0x25C[1] = (profile->word_0x0C & 0xFF0000) >> 16;
    plw->field_0x25C[2] = (profile->word_0x0C & 0xFF00) >> 8;
    plw->field_0x25C[3] = profile->word_0x0C;
    plw->field_0x25C[6] = profile->status_0x04;
    pl_equip_models_set(plw, profile->equip_0x10[0], profile->equip_0x10[1], profile->equip_0x10[2]);
    pl_equip_model_set(plw, profile->equip_0x10[4]);
    pl_equip_model_set(plw, profile->equip_0x10[3]);
    pl_equip_model_set(plw, profile->equip_0x10[6]);
    pl_equip_model_set(plw, profile->equip_0x10[5]);
    pl_equip_model_set(plw, profile->equip_0x10[7]);
    pl_equip_model_set(plw, profile->equip_0x10[8]);
    pl_parts_color_set(plw, 5, plw->field_0x25C[4]);
    ((void (*)(_PLW*))pl_parts_refresh)(plw);
}

/* 0x803777C8 (0x38): sends profile part 2 when online. */
void em020_profile_send(void)
{
    if (isNetCityMode() != 0)
        sendUserProfilePart(2, NULL, 0);
}

/* 0x80377800 (0xD0): copies the Poogie dress of `plw`'s hunter card into the lobby's peer copy and, for this player,
 * dresses the lobby Poogie. */
void lobby_net_dress_recv(_PLW* plw, const LbNetPacketHead* packet)
{
    _PLW* self = (_PLW*)get_move_work_adrs(2);
    LobbyPeerCopy* peer;

    self = &self[(s8)my_player_no()];
    peer = &lobby_state_block.peers_0x0008[plw->chunk_ofs];
    memcpy(((NetUserProfile*)peer->card_0x20)->partner_0x81,
           ((NetUserProfile*)lobby_hunter_cards[plw->chunk_ofs].card_0x024)->partner_0x81, 0x11);
    memcpy(((NetUserProfile*)peer->card_0x20)->field_0x7C,
           ((NetUserProfile*)lobby_hunter_cards[plw->chunk_ofs].card_0x024)->field_0x7C, 5);
    if (strcmp((const char*)self->name_0xB05, (const char*)plw->name_0xB05) == 0) {
        pig_name_set(((NetUserProfile*)peer->card_0x20)->partner_0x81);
        pig_dress_npc_set(((NetUserProfile*)peer->card_0x20)->dress_0x94);
    }
}

/* 0x803778D0 (0x88): asks the other players for their state (kind 12) and refreshes the peer list. */
void lobby_net_state_request_send(void)
{
    LbNetShortPacket packet;
    LbNetPacketHead* head;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(LbNetPacketHead));
        head = &packet.head;
        lb_net_head_set(head, 0xC);
        sendUserProfilePart(0, (const u8*)head, sizeof(LbNetPacketHead));
        NetCtrlWk::copyPeerList((NetPeerList*)&lobby_state_block.peer_count_0x0004);
    }
}

/* 0x80377958 (0x7C): answers a state request with this player's state. */
void lobby_net_state_request_recv(const LbNetPacketHead* packet)
{
    _PLW* plw = (_PLW*)get_move_work_adrs(2);

    if (plw != NULL && plw->slot_active != 0) {
        if (plw->field_0x001 == 0)
            return;
        lobby_net_state_send(&plw[(s8)my_player_no()], packet, 0);
    }
}

#pragma dont_inline on
/* 0x803779D4 (0x1D8): sends `plw`'s full state (kind 13 to the asker of `request`, or kind 22 to everyone). */
void lobby_net_state_send(_PLW* plw, const LbNetPacketHead* request, u8 broadcast)
{
    LbNetStatePacket packet;
    LbNetStatePacket* p;
    u8 kind;
    u16 act;

    if (isNetCityMode() != 0) {
        kind = plw->field_0x00A;
        act = plw->act_no;
        if ((u32)(kind - 0xB) > 1 && kind != 0) {
            kind = 0;
            act = 0;
        }
        p = &packet;
        memset(p, 0, sizeof(packet));
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        if (broadcast == 0) {
            packet.head.kind_0x00 = 0xD;
            memcpy(p->head.to_0x14, request->from_0x04, 0xA);
        } else {
            packet.head.kind_0x00 = 0x16;
            memcpy(p->head.to_0x14, em020_quest_page_ptr(), 0xA);
        }
        packet.act_kind_0x54 = kind;
        packet.act_no_0x52 = act;
        packet.bits_0x38 = plw->field_0x0B6;
        packet.latch_0x55 = plw->field_0xB00;
        packet.board_0x56 = plw->field_0xB01;
        packet.meal_0x57 = plw->field_0xB02;
        packet.angle_0x30 = plw->field_0x0A8;
        packet.x_0x32 = plw->vec_0x03C.x;
        packet.y_0x34 = plw->vec_0x03C.y;
        packet.z_0x36 = plw->vec_0x03C.z;
        packet.area_0x58 = plw->area_0x16;
        strcpy(p->partner_0x3E, plw->partner_0xB0F);
        packet.item_0x3A = plw->field_0x650;
        packet.count_0x3C = plw->field_0x652;
        if ((u8)kind == 0 && ((s32)act == 3 || (s32)act == 6)) {
            packet.x_0x32 = plw->target_pos_0x090.x;
            packet.z_0x36 = plw->target_pos_0x090.z;
        }
        strcpy(p->name_0x48, (const char*)plw->name_0xB05);
        sendUserProfilePart(0, (const u8*)p, sizeof(packet));
    }
}
#pragma dont_inline off

/* 0x80377BE0 (0x114): applies a received full state (kinds 13 and 22) to the remote player `plw`. */
void lobby_net_state_recv(_PLW* plw, const LbNetStatePacket* packet)
{
    plw->field_0xB04 = 1;
    plw->field_0x0A8 = plw->field_0x058 = packet->angle_0x30;
    plw->field_0xB00 = packet->latch_0x55;
    plw->field_0xB01 = packet->board_0x56;
    plw->field_0xB02 = packet->meal_0x57;
    plw->target_pos_0x090.x = packet->x_0x32;
    plw->target_pos_0x090.y = packet->y_0x34;
    plw->target_pos_0x090.z = packet->z_0x36;
    plw->area_0x16 = packet->area_0x58;
    plw->field_0x650 = packet->item_0x3A;
    plw->field_0x652 = packet->count_0x3C;
    strcpy(plw->partner_0xB0F, packet->partner_0x3E);
    strcpy((char*)plw->name_0xB05, packet->name_0x48);
    lb_npc_walk_blend_set(plw, 0);
    lb_npc_act_set(plw, packet->act_kind_0x54, packet->act_no_0x52, 0);
    plw->field_0x0B6 = packet->bits_0x38;
}

/* 0x80377CF4 (0x100): sends `plw`'s trade offer (kind 14): the names, the action, the area, the facing and position,
 * and the item. */
void lobby_net_trade_send(_PLW* plw)
{
    LbNetTradePacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        packet.head.kind_0x00 = 0xE;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        strcpy(packet.partner_0x34, plw->partner_0xB0F);
        strcpy(packet.name_0x3E, (const char*)plw->name_0xB05);
        packet.act_kind_0x2E = plw->field_0x00A;
        packet.act_no_0x2C = plw->act_no;
        packet.area_0x2F = plw->area_0x16;
        packet.angle_0x24 = plw->field_0x0A8;
        packet.x_0x26 = plw->vec_0x03C.x;
        packet.y_0x28 = plw->vec_0x03C.y;
        packet.z_0x2A = plw->vec_0x03C.z;
        packet.item_0x30 = plw->field_0x650;
        packet.count_0x32 = plw->field_0x652;
        sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
    }
}

/* 0x80377DF4 (0xE4): applies a received trade offer (kind 14) to the remote player `plw`. */
void lobby_net_trade_recv(_PLW* plw, const LbNetTradePacket* packet)
{
    plw->field_0x0A8 = packet->angle_0x24;
    plw->target_pos_0x090.x = packet->x_0x26;
    plw->target_pos_0x090.y = packet->y_0x28;
    plw->target_pos_0x090.z = packet->z_0x2A;
    plw->area_0x16 = packet->area_0x2F;
    lb_npc_walk_blend_set(plw, 0xA);
    plw->field_0x650 = packet->item_0x30;
    plw->field_0x652 = packet->count_0x32;
    strcpy((char*)plw->name_0xB05, packet->name_0x3E);
    strcpy(plw->partner_0xB0F, packet->partner_0x34);
    lb_npc_act_set(plw, packet->act_kind_0x2E, packet->act_no_0x2C, 0);
}

/* 0x80377ED8 (0x94): asks the trade partner `plw` for its offered item (kind 15). */
void lobby_net_item_request_send(_PLW* self, _PLW* plw)
{
    LbNetItemPacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        packet.head.kind_0x00 = 0xF;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        strcpy(packet.head.to_0x14, plw->hunter_0x5CA.id_0x5DB);
        packet.item_0x26 = plw->field_0x650;
        packet.count_0x28 = plw->field_0x652;
        sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
    }
}

/* 0x80377F6C (0xE8): answers an item request (kind 15): hands the item over when this player offers it and has it. */
void lobby_net_item_request_recv(_PLW* plw, const LbNetItemPacket* packet)
{
    s32 slots;
    s16 count;

    if (Pl_act_ck(plw, 0, 1) != 0 && plw->field_0x650 == packet->item_0x26) {
        count = plw->field_0x652;
        if (count == packet->count_0x28) {
            slots = ((s32 (*)(void*))userdata_item_slot_count)(lobby_world_block);
            if ((s32)item_count_find(plw->field_0x650, userdata_equip_item_slots_get(lobby_world_block), slots) >= count) {
                userdata_item_give(lobby_world_block, plw->field_0x650, -count, 1);
                plw->act_end_request = 0xFF;
                lobby_net_item_answer_send(plw, packet, 1);
                return;
            }
        }
    }
    lobby_net_item_answer_send(plw, packet, 2);
}

/* 0x80378054 (0xB4): answers the asker of `request` with `result` and this player's item (kind 16). */
void lobby_net_item_answer_send(_PLW* plw, const LbNetItemPacket* request, u8 result)
{
    LbNetItemPacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        packet.head.kind_0x00 = 0x10;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        memcpy(packet.head.to_0x14, request->head.from_0x04, 0xA);
        packet.result_0x24 = result;
        packet.item_0x26 = plw->field_0x650;
        packet.count_0x28 = plw->field_0x652;
        sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
    }
}

/* 0x80378108 (0x7C): takes an item answer (kind 16): receives the item when the trade went through. */
void lobby_net_item_answer_recv(_PLW* plw, const LbNetItemPacket* packet)
{
    if ((packet->result_0x24 & 1) != 0) {
        userdata_item_give(lobby_world_block, packet->item_0x26, packet->count_0x28, 1);
        hud_item_msg_push(2, 0x1D, packet->item_0x26);
        plw->act_end_request = 1;
    } else {
        plw->act_end_request = 2;
    }
}

/* 0x80378184 (0xAC): queues the member-join announcement for a joining member's id text (kind 17). */
void sendMemberJoinNotice(const char* id_text)
{
    LbNetNoticePacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(packet));
        packet.head.kind_0x00 = 0x11;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        strcpy(packet.head.to_0x14, id_text);
        packet.a_0x24 = system_w.field_0x18[2];
        packet.b_0x26 = system_w.field_0x18[3];
        packet.selection_0x28 = lobby_w.item_list_selection_0x162;
        sendUserProfilePart(0, (const u8*)&packet, sizeof(packet));
    }
}

/* 0x80378230 (0x30): takes the member-join notice (kind 17), as `lobby_net_notice_recv` does. */
void lobby_net_join_notice_recv(const LbNetNoticePacket* packet)
{
    system_w.field_0x18[2] = packet->a_0x24;
    system_w.field_0x18[3] = packet->b_0x26;
    lobby_w.item_list_selection_0x162 = packet->selection_0x28;
}

/* 0x80378260 (0x88): sends the two-state flag packet (kind 19 when `on`, 18 when not). */
void lobby_net_flag_send(u16 on)
{
    LbNetShortPacket packet;

    if (isNetCityMode() != 0) {
        memset(&packet, 0, sizeof(LbNetPacketHead));
        packet.head.kind_0x00 = on == 0 ? 0x12 : 0x13;
        packet.head.valid_0x01 = 1;
        memcpy(packet.head.from_0x04, em020_quest_page_ptr(), 0xA);
        sendUserProfilePart(0, (const u8*)&packet, sizeof(LbNetPacketHead));
    }
}

/* 0x803782E8 (0x80): the hunter card whose id is the sender of `packet`, or -1. */
s32 lobby_net_card_find(const LbNetPacketHead* packet)
{
    LbQuestPage* card;
    s32 i;

    i = 0;
    card = lobby_hunter_cards;
    do {
        if (card->active_0x000 != 0 && strcmp(card->id_0x03, packet->from_0x04) == 0)
            return i;
        card++;
        i++;
    } while (i < 10);
    return -1;
}

/* 0x80378368 (0xA8): whether `packet` is addressed to this player (and a hunter card is in use). */
u32 lobby_net_to_self_ck(const LbNetPacketHead* packet)
{
    LbQuestPage* card;
    LobbyTimerBlock* block = &lobby_state_block;
    s32 i;

    if (block->self_id_0x16B0[0] == 0)
        return 0;
    i = 0;
    card = lobby_hunter_cards;
    do {
        if (card->active_0x000 != 0 && strcmp(block->self_id_0x16B0, packet->to_0x14) == 0)
            return 1;
        card++;
        i++;
    } while (i < 10);
    return 0;
}

/* 0x80378410 (0x54): whether `packet` comes from this player. */
u32 lobby_net_from_self_ck(const LbNetPacketHead* packet)
{
    if (lobby_state_block.self_id_0x16B0[0] == 0)
        return 0;
    return strcmp(lobby_state_block.self_id_0x16B0, packet->from_0x04) == 0;
}

/* Whether hunter card `slot` has its player work loaded. */
inline u8 lb_card_loaded(u8 slot)
{
    return lobby_hunter_cards[slot].loaded_0x002;
}

/* 0x80378464 (0x43C): dispatches one 0x30-byte lobby message by kind to the member, roster and quest-page handlers;
 * bit 0x80 of the kind marks a relayed copy, which kinds 0 and 5 ignore. */
void handleLobbyNetMessage(NetLobbyMessage* message)
{
    LbNetPacketHead* packet = (LbNetPacketHead*)message;
    _PLW* players;
    u8 kind;
    u32 relayed;
    s32 index;
    u8 slot;

    if (isNetCityMode() != 0) {
        players = (_PLW*)get_move_work_adrs(2);
        kind = message->kind_0x00;
        relayed = (kind & 0x80) != 0;
        message->kind_0x00 = kind & 0x7F;
        switch (message->kind_0x00) {
        case 0:
        case 5:
            if (relayed != 1) {
                index = lobby_net_card_find(packet);
                if (index > 0) {
                    slot = index;
                    if (lb_card_loaded(slot) != 0)
                        lobby_net_position_recv(&players[slot], (LbNetPositionPacket*)message, message->kind_0x00);
                }
            }
            break;
        case 1:
            index = lobby_net_card_find(packet);
            if (index > 0) {
                slot = index;
                if (lb_card_loaded(slot) != 0)
                    lobby_net_action_recv(&players[slot], (LbNetActionPacket*)message);
            }
            break;
        case 2:
            players = &players[message->slot_0x24];
            NetCtrlWk::copyPeerList((NetPeerList*)&lobby_state_block.peer_count_0x0004);
            lobby_net_peer_load(players, message->peer_0x28, message->slot_0x24, 1);
            break;
        case 3:
            lobby_net_peer_release(message->slot_0x24);
            NetCtrlWk::copyPeerList((NetPeerList*)&lobby_state_block.peer_count_0x0004);
            break;
        case 4:
            lobby_net_profile_submit();
            break;
        case 6:
            index = lobby_net_card_find(packet);
            if (index > 0) {
                slot = index;
                if (lb_card_loaded(slot) != 0) {
                    players[slot].field_0xB01 = ((LbNetBoardPacket*)message)->a_0x24;
                    players[slot].field_0xB02 = ((LbNetBoardPacket*)message)->b_0x28;
                }
            }
            break;
        case 7:
            lobby_net_meal_recv((LbNetMealPacket*)message);
            break;
        case 8:
            lobby_net_notice_recv((LbNetNoticePacket*)message);
            break;
        case 9:
            index = lobby_net_card_find(packet);
            if (index > 0) {
                slot = index;
                if (lb_card_loaded(slot) != 0)
                    lobby_net_place_recv(&players[slot], (LbNetPlacePacket*)message);
            }
            break;
        case 10:
            if (lobby_net_from_self_ck(packet) != 1) {
                u8 slot = message->valid_0x01;

                if (lobby_hunter_cards[slot].loaded_0x002 != 0)
                    lobby_net_card_apply(&players[slot], packet);
            }
            break;
        case 11:
            if (lobby_net_from_self_ck(packet) != 1) {
                index = lobby_net_card_find(packet);
                if (index > 0) {
                    slot = index;
                    if (lb_card_loaded(slot) != 0)
                        lobby_net_dress_recv(&players[slot], packet);
                }
            }
            break;
        case 12:
            lobby_net_state_request_recv(packet);
            break;
        case 13:
            if (lobby_net_to_self_ck(packet) != 0) {
                index = lobby_net_card_find(packet);
                if (index > 0) {
                    slot = index;
                    if (lb_card_loaded(slot) != 0)
                        lobby_net_state_recv(&players[slot], (LbNetStatePacket*)message);
                }
            }
            break;
        case 22:
            if (lobby_net_to_self_ck(packet) != 1) {
                index = lobby_net_card_find(packet);
                if (index > 0) {
                    slot = index;
                    if (lb_card_loaded(slot) != 0)
                        lobby_net_state_recv(&players[slot], (LbNetStatePacket*)message);
                }
            }
            break;
        case 14:
            index = lobby_net_card_find(packet);
            if (index > 0) {
                slot = index;
                if (lb_card_loaded(slot) != 0)
                    lobby_net_trade_recv(&players[slot], (LbNetTradePacket*)message);
            }
            break;
        case 15:
            if (lobby_net_to_self_ck(packet) != 0)
                lobby_net_item_request_recv(&players[(u8)my_player_no()], (LbNetItemPacket*)message);
            break;
        case 16:
            if (lobby_net_to_self_ck(packet) != 0)
                lobby_net_item_answer_recv(&players[(u8)my_player_no()], (LbNetItemPacket*)message);
            break;
        case 17:
            if (lobby_net_to_self_ck(packet) != 0)
                lobby_net_join_notice_recv((LbNetNoticePacket*)message);
            break;
        case 18:
            lb_npc_net_flag_on();
            break;
        case 19:
            lb_npc_net_flag_off();
            break;
        case 20:
            lobby_net_from_self_ck(packet);
            break;
        }
    }
}

/* The em020 "false" program-table stub.
 * 0x803788A0 */
u32 em020_false_ck(void)
{
    return 0;
}

/* 0x803788A8 (0x78): whether the recent-player list holds the id text `id`. */
u32 lobby_recent_has(const char* id)
{
    LobbyRecentList* list = &lobby_state_block.recent_0x1DDC;
    LobbyRecentRow* row;
    s32 count;
    u32 found;
    s32 i;

    found = 0;
    i = 0;
    row = list->rows_0x004;
    count = list->count_0x000;
    for (; i < count; row++, i++) {
        if (strcmp(row->id_0x14.text_0x00, id) == 0)
            found = 1;
    }
    return found;
}

/* 0x80378920 (0x8C): sends a friend request to `who`: 2 when the player already asked, 1 when the list is full, else
 * 0, or 3 when the request could not go out. */
s32 lobby_friend_request_send(const LobbyRecentRow* who)
{
    LobbyTimerBlock* block = &lobby_state_block;

    if (lobby_recent_has(who->id_0x14.text_0x00) == 1)
        return 2;
    if (block->recent_0x1DDC.count_0x000 < 0x10) {
        block->result_0x1695 = 0;
        return (NetCtrlWk::sendFriendRequest((const NetId*)&who->id_0x14, who->name_0x00.text_0x00,
                                             &block->result_0x1695) == 0) ? 3 : 0;
    }
    return 1;
}

/* 0x803789AC (0x80): appends `who` to the recent-player list while it has room. */
void lobby_recent_add(const LobbyRecentRow* who)
{
    LobbyRecentList* list = &lobby_state_block.recent_0x1DDC;

    if (list->count_0x000 < 0x10) {
        LobbyRecentRow* row = &list->rows_0x004[list->count_0x000];

        strcpy(row->id_0x14.text_0x00, who->id_0x14.text_0x00);
        strcpy(row->name_0x00.text_0x00, who->name_0x00.text_0x00);
        list->count_0x000++;
    }
}

/* 0x80378A2C (0x5C): accepts the friend request of recent row `index`: 0 when out of range, 1 sent, 2 failed. */
s32 lobby_friend_accept(s32 index)
{
    LobbyRecentList* list = &lobby_state_block.recent_0x1DDC;

    if (index >= list->count_0x000)
        return 0;
    return (NetCtrlWk::acceptFriendRequest((const NetId*)&list->rows_0x004[index].id_0x14,
                                           &lobby_state_block.result_0x1695) == 0) + 1;
}

/* 0x80378A88 (0xA4): drops recent row `index`, moving the later rows up. */
void lobby_recent_drop(s32 index)
{
    LobbyRecentList* list = &lobby_state_block.recent_0x1DDC;
    s32 i;

    if (index < list->count_0x000) {
        for (i = index; i < list->count_0x000; i++) {
            if (i + 1 < list->count_0x000)
                list->rows_0x004[i].assign(&list->rows_0x004[i + 1]);
        }
        list->count_0x000--;
    }
}

/* 0x80378B2C (0x44): copies a recent-player row field by field. */
void LobbyRecentRow::assign(const LobbyRecentRow* src)
{
    name_0x00 = src->name_0x00;
    id_0x14 = src->id_0x14;
}

/* 0x80378B70 (0xBC): drops lobby mail `index`, moving the later ones up; 1 when dropped. */
u8 dropLobbyMail(s32 index)
{
    LobbyMailList* list = &lobby_state_block.mail_0x1FC4;
    s32 i;

    if (index >= list->count_0x000)
        return 0;
    for (i = index; i < list->count_0x000; i++) {
        if (i + 1 < list->count_0x000) {
            list->rows_0x014[i].assign(&list->rows_0x014[i + 1]);
            list->flags_0x004[i] = list->flags_0x004[i + 1];
        }
    }
    list->count_0x000--;
    return 1;
}

/* 0x80378C2C (0x78): copies a mail row field by field. */
void LobbyMailRow::assign(const LobbyMailRow* src)
{
    name_0x00 = src->name_0x00;
    id_0x14 = src->id_0x14;
    body_0x1F = src->body_0x1F;
}

/* 0x80378CA4 (0x78): whether the friend list holds the id text `id`. */
u32 lobby_friend_has(const char* id)
{
    LobbyFriendList* list = &lobby_state_block.friends_0x16D0;
    LobbyTimerRow* row;
    s32 count;
    u32 found;
    s32 i;

    found = 0;
    i = 0;
    row = list->rows_0x004;
    count = list->count_0x000;
    for (; i < count; row++, i++) {
        if (strcmp(row->id_0x14, id) == 0)
            found = 1;
    }
    return found;
}

/* 0x80378D1C (0x24): whether the friend list is full (50 rows). */
u32 lobby_friend_list_full(void)
{
    return lobby_state_block.friends_0x16D0.count_0x000 >= 50;
}

/* 0x80378D40 (0x8): whether the friend list holds `who`. */
u32 lobby_friend_has_row(const LobbyRecentRow* who)
{
    return lobby_friend_has(who->id_0x14.text_0x00);
}

/* 0x80378D48 (0xB0): adds a friend row (id text, name) to the friend list unless it is there or the list is full. */
void addFriendNotice(const char* id_text, const char* name)
{
    LobbyFriendList* list = &lobby_state_block.friends_0x16D0;

    if (lobby_friend_has(id_text) != 1 && list->count_0x000 < 50) {
        LobbyTimerRow* row = &list->rows_0x004[list->count_0x000];

        strcpy(row->id_0x14, id_text);
        strcpy(row->name_0x00, name);
        row->state_0x1E = 1;
        row->flag_0x1F = 0;
        list->count_0x000++;
        lobby_friend_list_sort(list);
    }
}

/* 0x80378DF8 (0xB0): removes `who` from the friend list: 0 when not there, 1 sent, 2 failed. */
s32 lobby_friend_remove(const LobbyRecentRow* who)
{
    LobbyTimerRow* row;
    s32 count;
    LobbyFriendList* list;
    s32 found;
    s32 i;

    list = &lobby_state_block.friends_0x16D0;
    found = -1;
    i = 0;
    row = list->rows_0x004;
    count = list->count_0x000;

    for (; i < count; row++, i++) {
        if (strcmp(row->id_0x14, who->id_0x14.text_0x00) == 0)
            found = i;
    }
    if (found < 0)
        return 0;
    return (NetCtrlWk::removeFriend((const NetId*)list->rows_0x004[found].id_0x14, &lobby_state_block.result_0x1695) == 0) + 1;
}

/* 0x80378EA8 (0xD4): drops `who` from the friend list, moving the later rows up, and re-sorts it. */
void lobby_friend_drop(const LobbyRecentRow* who)
{
    s32 count;
    LobbyTimerRow* row;
    LobbyFriendList* list;
    s32 found;
    s32 i;

    list = &lobby_state_block.friends_0x16D0;
    found = -1;
    i = 0;
    row = list->rows_0x004;
    count = list->count_0x000;

    for (; i < count; row++, i++) {
        if (strcmp(row->id_0x14, who->id_0x14.text_0x00) == 0)
            found = i;
    }
    if (found >= 0) {
        for (; found < list->count_0x000; found++) {
            if (found + 1 < list->count_0x000)
                lobby_friend_row_copy(&list->rows_0x004[found], &list->rows_0x004[found + 1]);
        }
        list->count_0x000--;
        lobby_friend_list_sort(list);
    }
}

/* The address of the em020 twelve-entry 31-byte row array at the shared lobby block's `+0x13E6`.
 * 0x80378F7C */
u8* em020_row_ptr(u8 index)
{
    return lobby_state_block.rows_0x13E6[index];
}

#ifdef __cplusplus
}
#endif
