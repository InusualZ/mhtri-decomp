/*
 * menu/menu_placeinfo.cpp - the lobby's city and community select screens (screens 28/33/37) and the city move
 *   confirm (screen 32), em003's call-SE timeline, the award and place-info menu pages (12/13) and the placed interior
 *   models of the lobby rooms (60 functions, all written).
 * RANGE. .text 0x80437270-0x8043D524 (60 functions); extab 0x8001DA94-0x8001DBEC, extabindex 0x8003E514-0x8003E718,
 *   .ctors 0x8056F3C8, .data 0x80603E90-0x806047D0, .bss 0x806E1B38-0x806E26E0, .sdata 0x80793A10-0x80793A80, .sdata2
 *   0x8079C8A8-0x8079C8D0.  The right edge is where the `.ctors` closure ends, the left is `Network/net_session_close.cpp`'s
 *   end.  One TU: the `__FILE__` string "menu_placeinfo.cpp" (0x80604780) has one copy.
 * FLAGS. `cflags_menu` (configure.py); `#pragma dont_inline on` at the foot keeps `__sinit` calling the two constructors.
 * NAMES. Module and file from the `__FILE__` string.  `draw_search_city_dialog`, `placeinfo_page12_*`/`placeinfo_page13_*`,
 *   `lb_city_select_open`, `lb_city_select_screen_step`, `lb_city_screen_open`, `lb_city_move_confirm_step`,
 *   `lb_city_area_ck`, `em003_call_se_*` are the map's names.
 *   GUESS (from each body and its callers): placeinfo_model_create, `place_info_set`, `pig_name_set` (it sets the
 *   placed interior models from a peer's eight layout ids).
 *   GUESS (the placed interior models, from the stage code that calls them): `PlaceModel`, `PlaceModelWork`,
 *   `place_model_work`, `place_model_work_init`, `place_model_area_load`, `place_model_create_pending`,
 *   `place_model_group_load`, `place_model_load`, `place_model_release`, `place_model_release_all`,
 *   `place_model_step`, `place_model_ready_ck`, `place_interior_flags`.
 *   GUESS (the select screens): `LbCitySelectWork`, `LbCityMoveWork`, `lb_city_select_init`, `lb_community_select_open`,
 *   `lb_city_select_close`, `lb_city_member_card_open`, `lb_city_member_scroll_sync`, `lb_city_filter_list_build`,
 *   `lb_city_tabs_init`, `lb_city_member_page_set`, `lb_city_page_load`, `lb_city_event_name_get`,
 *   `lb_city_second_filter_name_get`, `lb_city_first_filter_name_get`, `lb_city_help_draw`, `lb_city_list_draw`,
 *   `lb_city_list_ex_draw`, `lb_city_member_list_draw`, `lb_community_info_draw`, `lb_community_list_draw`,
 *   `lb_community_list_ex_draw`, `lb_city_filter_list_draw`, `lb_city_timer_draw`, `lb_city_select_draw`,
 *   `lb_city_move_close`, `lb_city_move_confirm_draw`, `lb_city_filters_collect`, `lb_community_select_step`.
 *   GUESS (pages and em003): `placeinfo_award_ck`, `placeinfo_world_draw`, `placeinfo_net_draw`,
 *   `placeinfo_server_info_draw`, `placeinfo_layer_info_draw`, `placeinfo_award_icons`, `em003_call_se_frames`,
 *   `em003_call_se_ids`.
 * RESIDUALS.
 *  - `lb_community_select_step`: retail keeps its page-count tail (pages = total / 8, cursor clamp) in state 8 step 2
 *    and reaches it by backward jumps from seven later places (no case label can mark it, playbook 241); the source
 *    calls the static helper `lb_community_page_count_set`, an extra function in our object.
 *    The tail after the switch (idea 183) is 3360 B against 3312 and scores lower; a scratch `goto` spelling 99.24.
 *  - `lb_city_select_screen_step`: state 5 step 3 repeats state 4 step 1's enter-city tail, which retail reaches by a
 *    backward jump (rule 8; idea 213: MWCC has no pass that merges the identical copy). The tail after the switch
 *    (idea 183) is retail's 3704 B but scores 96.30; a scratch `goto` spelling 100.
 *  - `lb_city_select_draw`: state 8 re-enters state 5's search-dialog body through a `for (;;)` re-dispatch (playbook
 *    203); retail branches straight to case 5, ours loops back to the compare chain (`li r3,5 ; b`), hoists the
 *    `lobby_w.state_0x000` load and colours `work` in r29 where retail has r31.
 *  - `place_model_release`, `place_model_load`: retail keeps `index * 0x174` in a saved register across the calls and
 *    stores through `work + offset`, plus a `mr r6,r30` copy of the slot pointer; not reproduced by an inline
 *    member, a byte-offset pointer or a global-based `model`.
 *  - `lb_city_member_list_draw`, `lb_city_list_draw`, `lb_city_list_ex_draw`: retail updates the `s32` icon/name x in
 *    place (`addi r25,r25,0xa`); ours folds the `+= 10` into its later uses (`+0x3a`) and recolours the loop.
 *  - `lb_city_timer_draw`: retail extends both `font_locate` coordinates after the second `fctiwz`; ours extends the
 *    first at once.
 *  - `lb_city_filter_list_draw`: retail clears the title id with `clrlwi r4,r4,16` after the ternary; ours omits it.
 *  - The other draws (`lb_community_*_draw`, `draw_search_city_dialog`, `placeinfo_world_draw`) and
 *    `lb_community_select_step`: register colouring and the odd `li` placement.
 *  - Data: `.data` 0x80603EA0-0x80604638 (the six combo tables `lobby/lb_companion_ui.cpp` reads: 0x80603EA0,
 *    0x806042B8, 0x806043E8, 0x806045F8, 0x8060461C, 0x80604628) and `.sdata` 0x80793A28-0x80793A67 (the small tables
 *    0x806045F8 points at) are not emitted: `lobby/lb_companion_ui.h` declares two of them under their map names.
 *    `.sdata2`: retail keeps a second 0.0f (0x8079C8CC) for `placeinfo_model_create`; ours shares the first.
 *  - `lb_area_view_pos_tbl` (0x805BA540) and `jump_err_msg_str` (0x80794B10) are unclaimed shared data (band headers).
 * SHAPES. The select screens' coordinates are `s32` locals read through `(s16)` views where retail extends before the
 *   arithmetic, `s16` locals where retail stores them extended; the colour flags are `bool`.  The NetCtrlWk calls whose
 *   header spells a `void`/`s32` return retail tests or stores as a byte go through signature casts
 *   (`requestListRefreshAlias`, `requestCommand17`, `getSelectedServerTypeIndex`).  The asserts sit on retail's lines
 *   288 and 292 (`#line`).
 */

#include "types.h"
#include "pl.h"   /* MHchar */
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_interior_load.h"
#include "lobby/mhchar_construct.h"
#include "menu/get_option_pair.h"
#include "menu/pig_name_set.h"
#include "menu/placeinfo_model_create.h"
#include "stage/get_now_mapno.h"
#include "stage/stg_w.h"   /* get_now_areano */
#include "sound/mhchar.h"   /* mhchar_reset */
#include "sound/mhchar_model_release.h"
#include "sound/mhchar_model_create.h"
#include "ef/fn_800CDB2C.h"   /* push_g3d_wk */
#include "ef/pRoot.h"
#include "g3d/g3d_root_model_bind.h"
#include "g3d/g3d_scnmdlsmpl.h"   /* nw4r::g3d::ScnMdlSimple::GetResMdl */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMdl */
#include "g3d/g3d_obj.h"   /* nw4r::g3d::AnmObj */
#include "g3d/res_mdl_copy_ctor.h"
#include "nw_resource.h"
#include "draw_shape.h"   /* res_file_ctor, res_file_assign */
#include "fn_8004CAD8.h"   /* vec_to_mh_vec3 */
#include "unsplit/lobby_area_view.h"
#include "MSL_C/printf.h"
#include "nw4r/db_assert.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em003_call_act_type_ck.h"
#include "menu/em003_call_se.h"
#include "sound/se.h"
#include "sound/get_em_se_bank.h"
#include "sound/fn_800D7F54.h"   /* sysSE_req */
#include "fn_80047398/lobby_world_block.h"
#include "menu/menu_item.h"   /* MenuSlot, ItemName, get_menu_lsp_tbl, game_ready_ck */
#include "menu/placeinfo_page12_open.h"
#include "menu/placeinfo_page12_step.h"
#include "menu/placeinfo_page12_draw.h"
#include "menu/placeinfo_page13_open.h"
#include "menu/placeinfo_page13_step.h"
#include "menu/placeinfo_page13_draw.h"
#include "hud/layout.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/draw_award_icon_idx.h"
#include "hud/cockpit.h"   /* chk_pointer */
#include "ef/system_core.h"   /* get_str_tbl */
#include "lobby/LbStr.h"
#include "lobby/lb_font_print.h"
#include "lobby/lb_font_puts.h"
#include "lobby/lb_font_center_print.h"
#include "sound/se_frame_req.h"
#include "lobby/lb_unlock_cond_ck.h"   /* kujira_event_over_ck */
#include "lobby/lb_world_submode_get.h"
#include "userdata_progress_flag_ck.h"
#include "Network/network_pat_control.h"
#include "Network/net_session_close.h"   /* NetPeerCard, copyPeerProfileCard, requestPeerProfileById */
#include "menu/menu_message.h"   /* menu_cursor_step*, menu_page_count, menu_scroll_*, put_frame_dialog, put_message */
#include "menu/put_frame_dialog_title.h"
#include "menu/put_frame_side_arrows.h"
#include "menu/menu_friend_list_init.h"
#include "menu/lb_city_select_open.h"
#include "menu/lb_city_select_screen_step.h"
#include "menu/lb_city_screen_open.h"
#include "menu/lb_city_move_confirm_step.h"
#include "menu/lb_city_area_ck.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_panel_close.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_menu_pos_tbl.h"   /* lobby_player_place, lobby_player_place_ex */
#include "lobby/lb_cursor_prompt_draw.h"
#include "enemy/em020_prog.h"   /* lobby_net_*, releaseRemotePlayerParts, em020_quest_pages_clear */
#include "enemy/lobby_state_block.h"
#include "menu/multi_result.h"   /* lobby_area_change_net_req */
#include "userdata_hunter_rank_info_get.h"
#include "get_userdata.h"
#include "fn_80047398.h"   /* subTransSet */
#include "font/flfnt.h"
#include "mh3_pad/disp_beta.h"
#include "menu/get_pop_dat_ptr.h"   /* get_option_cfg */
#include "unsplit/unknown.h"   /* jump_err_msg_str */


/* ---------------------------------------------------------------------------------------------------------------- */
/* The placed interior models. */

/* One interior model placed in a lobby room: its UV animation and the two g3d works it runs on, the model, and the
 * slot state.  size: 0x174 */
class PlaceModel {
public:
    PlaceModel() { mhchar_construct(&chr); }

    /* +0x000 */ nw4r::g3d::AnmObj* anm;   /* the UV animation `nw_res_anm_create` built (NULL: none) */
    /* +0x004 */ struct _g3d_work* works[2];   /* the g3d works the animation runs on */
    /* +0x00C */ union {
        MHchar chr;                 /* the model */
        u8 chr_bytes[0x164];        /* its whole record, which the init clears */
    };
    /* +0x170 */ u8 used;           /* 1 while the slot holds a model */
    /* +0x171 */ u8 interior_id;    /* the interior resource (`lb_interior_load`'s id; 0xFF: none) */
    /* +0x172 */ u8 flags;          /* `place_interior_flags[kind]`: 1 starts the motion, 2 builds the UV animation */
    /* +0x173 */ u8 res_index;      /* the loaded resource's table index (0xFF: not built yet) */
};

/* The eight placed models and the map/area they were loaded for.  size: 0xBA8 */
struct PlaceModelWork {
    /* +0x000 */ PlaceModel models[8];
    /* +0xBA0 */ u8 map;    /* 0xFF: nothing loaded */
    /* +0xBA1 */ u8 area;
    /* +0xBA2 */ u8 unused_0xBA2[6];
};

static PlaceModelWork place_model_work;

/* One farm plot record of the save block: its state (0 empty, 1 growing nothing, 2 planted), the crop and its
 * stage.  size: 0xC */
struct PlaceFarmPlot {
    /* +0x0 */ u8 state_0x0;
    /* +0x1 */ u8 pad_0x1[2];
    /* +0x3 */ u8 crop_0x3;     /* 0xFF: none */
    /* +0x4 */ s8 stage_0x4;
    /* +0x5 */ u8 pad_0x5[7];
};

/* One trade ship record of the save block: the item it brings and the count it has of the maximum.  size: 0x8 */
struct PlaceShip {
    /* +0x0 */ u16 item_0x0;    /* 0: no trade */
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ u8 count_0x4;
    /* +0x5 */ u8 max_0x5;
    /* +0x6 */ u8 pad_0x6[2];
};

/* The fields of the save block (`lobby_world_block`, `get_userdata()`) the place info and the city select read.
 * size: 0x4835 (approximation: the last byte read) */
struct PlaceUserData {
    /* +0x0000 */ u8 pad_0x0000[0x39C0];
    /* +0x39C0 */ u16 second_filter_low_0x39C0[0x80];    /* the bounds `NetCtrlWk::collectSecondFilters` checks */
    /* +0x3AC0 */ u16 second_filter_high_0x3AC0[0x80];
    /* +0x3BC0 */ u8 pad_0x3BC0[0x224];
    /* +0x3DE4 */ u16 hunter_rank_0x3DE4;
    /* +0x3DE6 */ u8 pad_0x3DE6[0x11E];
    /* +0x3F04 */ s32 resource_points_0x3F04;   /* the village's resource points */
    /* +0x3F08 */ u8 pad_0x3F08[0x30];
    /* +0x3F38 */ PlaceFarmPlot farm_plots_0x3F38[8];
    /* +0x3F98 */ u8 pad_0x3F98[0x160];
    /* +0x40F8 */ u16 award_bits_0x40F8[2];     /* the owned awards, 12 bits a halfword */
    /* +0x40FC */ u8 pad_0x40FC[0x55C];
    /* +0x4658 */ PlaceShip ships_0x4658[3];
    /* +0x4670 */ u8 ship_count_0x4670;
    /* +0x4671 */ u8 pad_0x4671[0x1C3];
    /* +0x4834 */ s8 kujira_days_0x4834;        /* the kujira event's day counter (counts down below zero) */
};

/* The city select's work (`lobby_w.menu_0xAC` while screens 28/33/37 are open): the state, the request result, the
 * city or community page, the member list of the selected one, the search tabs and their value list.  size: 0x2000 */
struct LbCitySelectWork {
    /* +0x0000 */ u8 state_0x000;          /* 1 open, 2 city list, 3 member list, 4 join confirm, 5 search, 6 value list,
                                            * 8 community search/list, 12 back to talk, 13 nothing found */
    /* +0x0001 */ u8 step_0x001;
    /* +0x0002 */ u8 sub_0x002;
    /* +0x0003 */ u8 server_type_0x003;    /* `NetCtrlWk::getSelectedServerTypeIndex` */
    /* +0x0004 */ s8 result_0x004;         /* the pending network request's result (0 pending, < 0 failed) */
    /* +0x0005 */ u8 community_0x005;      /* 1 on the community screens */
    /* +0x0006 */ u8 pad_0x006[2];
    /* +0x0008 */ u16 arrow_moved_0x008;   /* the page cursor's last move (the page arrows' pulse) */
    /* +0x000A */ s16 cursor_0x00A;
    /* +0x000C */ s16 count_0x00C;         /* the rows of the page */
    /* +0x000E */ s16 page_0x00E;
    /* +0x0010 */ s16 pages_0x010;
    /* +0x0012 */ s16 total_0x012;
    /* +0x0014 */ s16 marks_0x014[8];
    /* +0x0024 */ NetCityView cities_0x024[8];
    /* +0x01A4 */ NetCommunityView communities_0x1A4[8];
    /* +0x0544 */ s16 member_total_0x544;
    /* +0x0546 */ s16 member_count_0x546;  /* the rows of the member page */
    /* +0x0548 */ s16 member_page_0x548;
    /* +0x054A */ s16 member_row_0x54A;
    /* +0x054C */ u8 member_busy_0x54C[5];
    /* +0x0551 */ u8 member_status_0x551[5];
    /* +0x0556 */ u16 member_area_0x556[5];
    /* +0x0560 */ NetFriendCard members_0x560[10];
    /* +0x1140 */ MenuScroll scroll_0x1140;  /* the member list's cursor */
    /* +0x114C */ u8 card_view_0x114C[0x148];  /* the member card view (`FriendCardView`, `menu/movie.cpp`'s layout) */
    /* +0x1294 */ u8 member_cards_0x1294;
    /* +0x1295 */ s8 filter_ids_0x1295[0x40];  /* the events or second filters the server lists (-1 ends) */
    /* +0x12D5 */ u8 pad_0x12D5[3];
    /* +0x12D8 */ s32 filter_count_0x12D8;
    /* +0x12DC */ s8 first_filters_0x12DC[8];
    /* +0x12E4 */ s16 first_cursor_0x12E4;
    /* +0x12E6 */ u8 pad_0x12E6[2];
    /* +0x12E8 */ s32 first_count_0x12E8;
    /* +0x12EC */ s32 tab_kinds_0x12EC[4];    /* the search tabs: 0 first filter, 1 second filter, 2 event, -1 confirm */
    /* +0x12FC */ s32 tab_values_0x12FC[4];   /* the value each tab picked */
    /* +0x130C */ s16 tab_cursor_0x130C;
    /* +0x130E */ s16 tab_count_0x130E;
    /* +0x1310 */ u8 pad_0x1310[6];
    /* +0x1316 */ s16 timer_0x1316;           /* the frames the server keeps the search open */
    /* +0x1318 */ s16 list_page_0x1318;
    /* +0x131A */ s16 list_pages_0x131A;
    /* +0x131C */ s16 list_cursor_0x131C;
    /* +0x131E */ s16 list_count_0x131E;
    /* +0x1320 */ s16 list_0x1320[8];         /* the value list's page */
    /* +0x1330 */ u8 pad_0x1330[0x10];
    /* +0x1340 */ s32 yes_no_0x1340;          /* the join confirm's answer */
    /* +0x1344 */ u8 pad_0x1344[0x20];
    /* +0x1364 */ u32 caller_0x1364;          /* the two words the opener passes */
    /* +0x1368 */ u32 caller_arg_0x1368;
    /* +0x136C */ u16 hunter_rank_0x136C;
    /* +0x136E */ s16 msg_timer_0x136E;       /* frames before a message can be closed */
    /* +0x1370 */ u8 pad_0x1370[0xC90];
};

/* The city move confirm's work (`lobby_w.menu_0xAC` while screen 32 is open).  size: 0x10 (approximation: the last
 * field used) */
struct LbCityMoveWork {
    /* +0x0 */ s32 step_0x000;
    /* +0x4 */ s32 delay_0x004;
    /* +0x8 */ s32 yes_no_0x008;
    /* +0xC */ s8 result_0x00C;
    /* +0xD */ u8 pad_0x00D[3];
};

extern "C" {

void lb_city_select_close(void);
s32 lb_city_filters_collect(u8 kind);
void lb_community_select_step(LbCitySelectWork* work);
void lb_city_select_draw(void);
void lb_city_move_close(void);
void lb_city_move_confirm_draw(void);
void lb_city_member_card_open(LbCitySelectWork* work);
void lb_city_member_scroll_sync(MenuScroll* scroll, s16* page, s16* row);
void lb_city_filter_list_build(LbCitySelectWork* work, s32 kind, s16 page);
void lb_city_tabs_init(LbCitySelectWork* work);
void lb_city_member_page_set(LbCitySelectWork* work, s16 page);
void lb_city_page_load(LbCitySelectWork* work, s16 page);
s8* lb_city_event_name_get(s32 index);
s8* lb_city_second_filter_name_get(s32 index);
s8* lb_city_first_filter_name_get(s32 index);
void lb_city_help_draw(u16 id);
void lb_city_list_draw(LbCitySelectWork* work, bool active);
void lb_city_list_ex_draw(LbCitySelectWork* work, bool active);
void lb_city_member_list_draw(LbCitySelectWork* work, bool active);
void lb_community_info_draw(LbCitySelectWork* work);
void lb_community_list_draw(LbCitySelectWork* work, bool active);
void lb_community_list_ex_draw(LbCitySelectWork* work, bool active);
void draw_search_city_dialog(LbCitySelectWork* work, bool active);
void lb_city_filter_list_draw(LbCitySelectWork* work, bool active);
void lb_city_timer_draw(LbCitySelectWork* work);
void placeinfo_server_rows_draw(u16 id, u32 framed, NetPeerCard* card);
void placeinfo_settings_rows_draw(u16 id, u32 framed, NetPeerCard* card);

/* ---------------------------------------------------------------------------------------------------------------- */
/* The city select (screens 28/33/37) and the city move confirm (screen 32). */

/* 0x80437270 (0xCC): clears the city select work, records the two caller words, opens lobby screen `screen` and,
 * for the community select (37), asks for the slot list first. */
void lb_city_select_init(u32 caller, u32 caller_arg, u8 screen) {
    LbLobbyWork* lobby = &lobby_w;
    LbCitySelectWork* work = (LbCitySelectWork*)lobby->menu_0xAC;
    s32 rank_next;
    s32 rank_points;
    NetSlotIds ids;

    memset(work, 0, sizeof(LbCitySelectWork));
    work->caller_0x1364 = caller;
    work->caller_arg_0x1368 = caller_arg;
    lobby_w.state_0x000 = screen;
    lobby->active_0x008 = 1;
    work->server_type_0x003 = ((u8 (*)(void))NetCtrlWk::getSelectedServerTypeIndex)();
    userdata_hunter_rank_info_get(&work->hunter_rank_0x136C, &rank_points, &rank_next);
    switch (lobby_w.state_0x000) {
    case 37:
        ids.slots_0x00[0] = -1;
        ids.slots_0x00[1] = -1;
        ids.slots_0x00[2] = -1;
        ids.slots_0x00[3] = 7;
        if (NetCtrlWk::submitSlotRequest(&ids, &work->result_0x004) == 0) {
            lb_city_select_close();
        } else {
            work->state_0x000 = 1;
        }
        break;
    default:
        work->state_0x000 = 1;
        break;
    }
}

/* 0x8043733C (0x24): opens the city select (screen 28 for kind 11, else 33) */
void lb_city_select_open(struct _PLW* self, u8 kind) {
    if (kind == 11) {
        lb_city_select_init((u32)self, 0, 28);
    } else {
        lb_city_select_init((u32)self, 0, 33);
    }
}

/* 0x80437360 (0x8): opens the community select (screen 37). */
void lb_community_select_open(u32 caller, u32 caller_arg) {
    lb_city_select_init(caller, caller_arg, 37);
}

/* 0x80437368 (0x4): closes the city select. */
void lb_city_select_close(void) {
    lb_panel_close();
}

/* 0x8043736C (0x68): copies the requested peer's card and opens the card view on it. */
void lb_city_member_card_open(LbCitySelectWork* work) {
    NetPeerCard* card = (NetPeerCard*)lobby_state_block.block_0x11D4;

    memset(card, 0, sizeof(NetPeerCard));
    copyPeerProfileCard(card);
    menu_friend_list_init((FriendCardView*)work->card_view_0x114C, card, 3, 1);
}

/* 0x804373D4 (0x78): clamps the member list's page and row to the scroll and copies them into it. */
void lb_city_member_scroll_sync(MenuScroll* scroll, s16* page, s16* row) {
    u8 rows;

    if (*page >= scroll->col_count) {
        *page = 0;
    }
    if (*page >= scroll->col_count - 1) {
        rows = scroll->field_0x007;
    } else {
        rows = scroll->rows_per_page;
    }
    scroll->row_limit = rows;
    if (*row >= rows) {
        *row = rows - 1;
        scroll->row = *row;
    } else {
        scroll->row = *row;
    }
    scroll->col = *page;
}

/* 0x8043744C (0xE78): the city select screen step (NetCtrlWk::requestEnterCity), screens 28/33/37 */
void lb_city_select_screen_step(void) {
    LbCitySelectWork* work = (LbCitySelectWork*)lobby_w.menu_0xAC;
    MenuScroll* scroll;
    u32 last_row;
    s32 state;
    NetSettingsWords settings;
    const char* peer_id;
    u8 map;
    s16 city_id;

    switch (lobby_w.state_0x000) {
    case 33:
    case 37:
        lb_community_select_step(work);
        return;
    }
    work->arrow_moved_0x008 = 0;
    switch (work->state_0x000) {
    case 1:
        work->state_0x000 = 2;
        work->step_0x001 = 0;
        work->cursor_0x00A = 0;
        work->community_0x005 = 0;
        break;
    case 2:
        switch (work->step_0x001) {
        case 0:
            if (NetCtrlWk::requestCommand07(&work->result_0x004) == 0) {
                work->step_0x001 = 99;
            } else {
                work->step_0x001++;
                sysSE_stop(0x20);
            }
            break;
        case 1:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    work->step_0x001 = 99;
                } else {
                    lb_city_page_load(work, 0);
                    work->step_0x001++;
                }
            }
            break;
        case 2:
            NetCtrlWk::copyCityViews(work->cities_0x024, work->page_0x00E * 8, 8, 0);
            if (lb_cmd_pressed_ck(0x10) != 0) {
                switch (work->cities_0x024[work->cursor_0x00A].state_0x0C) {
                case 2:
                    sysSE_req(5);
                    work->member_page_0x548 = 0;
                    work->member_row_0x54A = 0;
                    work->state_0x000 = 3;
                    work->step_0x001 = 0;
                    work->member_count_0x546 = 0;
                    menu_scroll_init(&work->scroll_0x1140, 0, 0, 6);
                    NetCtrlWk::requestCommand0E(&work->result_0x004,
                                                work->cities_0x024[work->cursor_0x00A].id_0x08);
                    break;
                case 0:
                    sysSE_req(0);
                    work->state_0x000 = 5;
                    work->step_0x001 = 0;
                    lb_city_filters_collect(2);
                    em020_quest_pages_clear();
                    NetCtrlWk::requestEnterCity(&work->result_0x004,
                                                work->cities_0x024[work->cursor_0x00A].id_0x08, 0);
                    lb_city_tabs_init(work);
                    break;
                default:
                    sysSE_req(2);
                    break;
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                lb_city_select_close();
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                work->cursor_0x00A = menu_cursor_step(work->cursor_0x00A, work->count_0x00C, lb_cmd_repeat_get(), 1, 2);
            } else if (lb_cmd_repeat_ck(0xC) != 0) {
                work->page_0x00E = menu_cursor_wrap(work->page_0x00E, work->pages_0x010, lb_cmd_repeat_get(), 4, 8, 6,
                                                    &work->arrow_moved_0x008);
                lb_city_page_load(work, work->page_0x00E);
            }
            break;
        case 99:
            lb_city_select_close();
            break;
        }
        break;
    case 3:
        NetCtrlWk::copyCityViews(work->cities_0x024, work->page_0x00E * 8, 8, 1);
        switch (work->step_0x001) {
        case 0:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    work->state_0x000 = 2;
                    work->step_0x001 = 2;
                } else {
                    work->member_cards_0x1294 = readFriendCards(work->members_0x560, 0, 10);
                    work->cities_0x024[work->cursor_0x00A].population_0x00 = work->member_cards_0x1294;
                    if (work->member_cards_0x1294 == 0) {
                        work->state_0x000 = 2;
                        work->step_0x001 = 2;
                    } else {
                        lb_city_member_page_set(work, work->member_page_0x548);
                        menu_scroll_init(&work->scroll_0x1140, 0,
                                         work->member_total_0x544 + menu_page_count(work->member_total_0x544, 5), 6);
                        lb_city_member_scroll_sync(&work->scroll_0x1140, &work->member_page_0x548,
                                                   &work->member_row_0x54A);
                        work->scroll_0x1140.row = work->scroll_0x1140.row_limit - 1;
                        work->step_0x001 = 1;
                        work->sub_0x002 = 0;
                    }
                }
            }
            break;
        case 1:
            state = work->cities_0x024[work->cursor_0x00A].state_0x0C;
            if (state != 2 && state != 3) {
                sysSE_req(1);
                work->state_0x000 = 2;
                work->step_0x001 = 2;
                NetCtrlWk::setSubError(9);
                break;
            }
            switch (work->sub_0x002) {
            case 0:
                if (lb_cmd_pressed_ck(0x10) != 0) {
                    if (work->scroll_0x1140.row + 1 == work->scroll_0x1140.row_limit) {
                        if ((u32)work->cities_0x024[work->cursor_0x00A].values_0x20[1] != 0xFFFFFFFF &&
                            NetCtrlWk::passesFirstFilter(work->cities_0x024[work->cursor_0x00A].values_0x20[1],
                                                         get_hunter_rank_max(lobby_world_block)) == 0) {
                            sysSE_req(2);
                            work->sub_0x002 = 1;
                            work->msg_timer_0x136E = 15;
                        } else {
                            sysSE_req(0);
                            work->state_0x000 = 4;
                            work->step_0x001 = 0;
                            work->yes_no_0x1340 = 0;
                        }
                    } else {
                        sysSE_req(0);
                        work->step_0x001 = 2;
                        requestPeerProfileById(
                            work->members_0x560[(s8)(work->scroll_0x1140.row + work->scroll_0x1140.col * 5)].id_0x003,
                            &work->result_0x004, 0, 0);
                    }
                } else if (lb_cmd_pressed_ck(0x20) != 0) {
                    sysSE_req(1);
                    work->state_0x000 = 2;
                    work->step_0x001 = 2;
                } else {
                    scroll = &work->scroll_0x1140;
                    last_row = 0;
                    if (scroll->row_limit == scroll->row + 1) {
                        last_row = 1;
                    }
                    menu_scroll_step(scroll, lb_cmd_repeat_get(), 6);
                    if (scroll->move_flags & 0xC) {
                        lb_city_member_page_set(work, scroll->col);
                        work->member_page_0x548 = scroll->col;
                        if (last_row == 1) {
                            scroll->row = scroll->row_limit - 1;
                            work->member_row_0x54A = scroll->row;
                        } else if (scroll->row == scroll->row_limit - 1 && scroll->row != 0) {
                            scroll->row--;
                            work->member_row_0x54A = scroll->row;
                        }
                    } else if (scroll->move_flags & 3) {
                        work->member_row_0x54A = scroll->row;
                    }
                }
                break;
            case 1:
                if (work->msg_timer_0x136E > 0) {
                    work->msg_timer_0x136E--;
                } else if (lb_cmd_pressed_ck(0x10) != 0) {
                    work->sub_0x002 = 0;
                    sysSE_req(0);
                }
                break;
            }
            break;
        case 2:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    menu_scroll_init(&work->scroll_0x1140, 0,
                                     work->member_total_0x544 + menu_page_count(work->member_total_0x544, 5), 6);
                    lb_city_member_scroll_sync(&work->scroll_0x1140, &work->member_page_0x548, &work->member_row_0x54A);
                    work->step_0x001 = 1;
                } else {
                    lb_city_member_card_open(work);
                    work->step_0x001 = 3;
                }
            }
            break;
        case 3:
            state = work->cities_0x024[work->cursor_0x00A].state_0x0C;
            if (state != 2 && state != 3) {
                sysSE_req(1);
                work->state_0x000 = 2;
                work->step_0x001 = 2;
                NetCtrlWk::setSubError(9);
                break;
            }
            switch (menu_friend_list_step((FriendCardView*)work->card_view_0x114C)) {
            case 0:
                break;
            case 2:
                peer_id = ((NetPeerCard*)lobby_state_block.block_0x11D4)->id_0x014;
                if (NetCtrlWk::requestCommand18((const s32*)&peer_id, &work->result_0x004) == 0) {
                    work->step_0x001 = 1;
                    work->sub_0x002 = 0;
                } else {
                    work->step_0x001 = 4;
                    work->sub_0x002 = 0;
                }
                break;
            default:
                work->step_0x001 = 1;
                work->sub_0x002 = 0;
                break;
            }
            break;
        case 4:
            switch (work->sub_0x002) {
            case 0:
                if (work->result_0x004 != 0) {
                    if (lobby_net_leave_select(work->result_0x004) == 0 && lobby_net_jump_msg_get(work->result_0x004) != 0) {
                        work->sub_0x002 = 1;
                        work->msg_timer_0x136E = 15;
                    } else {
                        lb_city_select_close();
                    }
                }
                break;
            case 1:
                if (work->msg_timer_0x136E > 0) {
                    work->msg_timer_0x136E--;
                } else if (lb_cmd_pressed_ck(0x10) != 0) {
                    sysSE_req(0);
                    lb_city_select_close();
                }
                break;
            }
            break;
        }
        break;
    case 4:
        NetCtrlWk::copyCityViews(work->cities_0x024, work->page_0x00E * 8, 8, 1);
        state = work->cities_0x024[work->cursor_0x00A].state_0x0C;
        if (state != 2 && state != 3 && work->step_0x001 == 0) {
            sysSE_req(1);
            work->state_0x000 = 2;
            work->step_0x001 = 2;
            NetCtrlWk::setSubError(9);
            break;
        }
        switch (work->step_0x001) {
        case 0:
            switch (lb_yes_no_step(&work->yes_no_0x1340)) {
            case 1:
                if ((u32)work->cities_0x024[work->cursor_0x00A].population_0x00 >=
                    (u32)work->cities_0x024[work->cursor_0x00A].capacity_0x04) {
                    NetCtrlWk::setSubError(8);
                    work->step_0x001 = 99;
                } else {
                    em020_quest_pages_clear();
                    if (NetCtrlWk::requestEnterCity(&work->result_0x004,
                                                    work->cities_0x024[work->cursor_0x00A].id_0x08, 1) == 0) {
                        work->step_0x001 = 99;
                    } else {
                        work->step_0x001++;
                    }
                }
                break;
            case 2:
                work->state_0x000 = 3;
                work->step_0x001 = 0;
                break;
            }
            break;
        case 1:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    work->step_0x001 = 99;
                    break;
                }
                lb_city_select_close();
                lobby_area_change_net_req();
                map = get_now_mapno();
                lobby_player_place_ex(1, stage_entry_pos_get(map, 1), stage_entry_angle_get(map, 1), 5);
                return;
            }
            break;
        case 99:
            lb_city_select_close();
            break;
        }
        break;
    case 5:
        switch (work->step_0x001) {
        case 0:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    work->state_0x000 = 2;
                    work->step_0x001 = 2;
                } else {
                    work->step_0x001++;
                    work->timer_0x1316 = lobby_state_block.timeout_0x168E[2];
                    if (work->server_type_0x003 == 3) {
                        work->first_count_0x12E8 = NetCtrlWk::collectFirstFilters(
                            work->first_filters_0x12DC, ((PlaceUserData*)get_userdata())->hunter_rank_0x3DE4, 8);
                        work->first_cursor_0x12E4 = 0;
                        work->tab_values_0x12FC[0] = work->first_filters_0x12DC[work->first_cursor_0x12E4];
                    }
                }
            }
            break;
        case 1:
            if (NetCtrlWk::isAccountLinked() == 0) {
                work->step_0x001 = 2;
                break;
            }
            if (work->timer_0x1316 > 0) {
                work->timer_0x1316--;
            } else {
                NetCtrlWk::requestCommand12(&work->result_0x004, work->cities_0x024[work->cursor_0x00A].id_0x08);
                work->step_0x001 = 2;
                sysSE_req(1);
                break;
            }
            {
                if (lb_cmd_pressed_ck(0x10) != 0) {
                    if (work->tab_cursor_0x130C == work->tab_count_0x130E - 1) {
                        sysSE_req(0);
                        city_id = work->cities_0x024[work->cursor_0x00A].id_0x08;
                        if (work->server_type_0x003 == 3) {
                            settings.word_0x00 = -1;
                            settings.word_0x04 = work->tab_values_0x12FC[0];
                            settings.word_0x08 = work->tab_values_0x12FC[1];
                            settings.word_0x0C = work->tab_values_0x12FC[2];
                        } else {
                            settings.word_0x00 = -1;
                            settings.word_0x04 = -1;
                            settings.word_0x08 = -1;
                            settings.word_0x0C = work->tab_values_0x12FC[0];
                        }
                        NetCtrlWk::requestSettingsCommand(&work->result_0x004, city_id, &settings);
                        work->step_0x001 = 3;
                    } else if ((u32)(work->tab_kinds_0x12EC[work->tab_cursor_0x130C] - 1) <= 1) {
                        work->state_0x000 = 6;
                        work->list_cursor_0x131C = 0;
                        lb_city_filter_list_build(work, work->tab_kinds_0x12EC[work->tab_cursor_0x130C], 0);
                        sysSE_req(5);
                    }
                } else if (lb_cmd_pressed_ck(0x20) != 0) {
                    NetCtrlWk::requestCommand12(&work->result_0x004, work->cities_0x024[work->cursor_0x00A].id_0x08);
                    work->step_0x001 = 2;
                    sysSE_req(1);
                } else if (lb_cmd_repeat_ck(3) != 0) {
                    work->tab_cursor_0x130C = menu_cursor_step(work->tab_cursor_0x130C, work->tab_count_0x130E,
                                                               lb_cmd_repeat_get(), 1, 2);
                } else if (lb_cmd_repeat_ck(0xC) != 0 && work->tab_kinds_0x12EC[work->tab_cursor_0x130C] == 0) {
                    work->first_cursor_0x12E4 = menu_cursor_step_fixed_tail(work->first_cursor_0x12E4,
                                                                            work->first_count_0x12E8,
                                                                            lb_cmd_repeat_get(), 4, 8,
                                                                            &work->arrow_moved_0x008);
                    work->tab_values_0x12FC[work->tab_cursor_0x130C] =
                        work->first_filters_0x12DC[work->first_cursor_0x12E4];
                }
            }
            break;
        case 2:
            if (work->result_0x004 != 0) {
                work->state_0x000 = 2;
                work->step_0x001 = 2;
            }
            break;
        case 3:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    work->state_0x000 = 2;
                    work->step_0x001 = 2;
                }
                lb_city_select_close();
                lobby_area_change_net_req();
                map = get_now_mapno();
                lobby_player_place_ex(1, stage_entry_pos_get(map, 1), stage_entry_angle_get(map, 1), 5);
                return;
            }
            break;
        }
        break;
    case 6:
        if (NetCtrlWk::isAccountLinked() == 0) {
            work->state_0x000 = 5;
            sysSE_req(1);
            break;
        }
        if (work->timer_0x1316 > 0) {
            work->timer_0x1316--;
        } else {
            NetCtrlWk::requestCommand12(&work->result_0x004, work->cities_0x024[work->cursor_0x00A].id_0x08);
            work->state_0x000 = 5;
            work->step_0x001 = 2;
            sysSE_req(1);
            break;
        }
        {
            if (lb_cmd_pressed_ck(0x10) != 0) {
                work->state_0x000 = 5;
                work->tab_values_0x12FC[work->tab_cursor_0x130C] = work->list_0x1320[work->list_cursor_0x131C];
                sysSE_req(0);
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                work->state_0x000 = 5;
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                work->list_cursor_0x131C = menu_cursor_step(work->list_cursor_0x131C, work->list_count_0x131E,
                                                            lb_cmd_repeat_get(), 1, 2);
            } else if (lb_cmd_repeat_ck(0xC) != 0) {
                work->list_page_0x1318 = menu_cursor_wrap(work->list_page_0x1318, work->list_pages_0x131A,
                                                          lb_cmd_repeat_get(), 4, 8, 6, &work->arrow_moved_0x008);
                lb_city_filter_list_build(work, work->tab_kinds_0x12EC[work->tab_cursor_0x130C], work->list_page_0x1318);
            }
        }
        break;
    }
    subTransSet((u32)lb_city_select_draw, 0, NULL);
}

/* 0x804382C4 (0x1D8): builds the page of the selected tab's filter list (kind 0 events, 1 second filters, 2 first
 * filters) that the list dialog shows. */
void lb_city_filter_list_build(LbCitySelectWork* work, s32 kind, s16 page) {
    s32 count;
    s32 index;
    s32 i;
    s8 id;

    memset(work->list_0x1320, 0, sizeof(work->list_0x1320));
    work->list_page_0x1318 = 0;
    work->list_pages_0x131A = 0;
    work->list_count_0x131E = 0;
    count = 0;
    switch (kind) {
    case 0:
        count = lb_city_filters_collect(2);
        break;
    case 1:
        count = lb_city_filters_collect(1);
        break;
    case 2:
        count = lb_city_filters_collect(0);
        break;
    }
    if (kind == 0 && work->community_0x005 == 0) {
        return;
    }
    if (count != 0) {
        index = page * 8;
        if (count >= index) {
            for (i = 0; i < 8; i++) {
                id = work->filter_ids_0x1295[index];
                if (id == -1) {
                    break;
                }
                index++;
                work->list_0x1320[work->list_count_0x131E++] = id;
            }
        }
    }
    if (kind != 0) {
        work->list_page_0x1318 = page;
        work->list_pages_0x131A = menu_page_count(count, 8);
        if (work->list_cursor_0x131C >= work->list_count_0x131E) {
            work->list_cursor_0x131C = work->list_count_0x131E - 1;
        }
    }
}

/* 0x8043849C (0x68): sets the search dialog's tabs: event, second and first filter for server type 3, else the
 * first filter alone, plus the confirm row. */
void lb_city_tabs_init(LbCitySelectWork* work) {
    work->tab_cursor_0x130C = 0;
    work->tab_kinds_0x12EC[0] = -1;
    work->tab_values_0x12FC[0] = 0;
    work->tab_kinds_0x12EC[1] = -1;
    work->tab_values_0x12FC[1] = 0;
    work->tab_kinds_0x12EC[2] = -1;
    work->tab_values_0x12FC[2] = 0;
    work->tab_kinds_0x12EC[3] = -1;
    work->tab_values_0x12FC[3] = 0;
    switch (work->server_type_0x003) {
    case 3:
        work->tab_kinds_0x12EC[0] = 0;
        work->tab_kinds_0x12EC[1] = 1;
        work->tab_kinds_0x12EC[2] = 2;
        work->tab_count_0x130E = 4;
        break;
    default:
        work->tab_kinds_0x12EC[0] = 2;
        work->tab_count_0x130E = 2;
        break;
    }
}

/* 0x80438504 (0xB0): copies member page `page` (five cards) of the selected city into the member list rows. */
void lb_city_member_page_set(LbCitySelectWork* work, s16 page) {
    NetFriendCard* card;
    s16 row;
    s32 i;
    s16 index;

    work->member_total_0x544 = work->cities_0x024[work->cursor_0x00A].population_0x00;
    work->member_count_0x546 = 0;
    index = page * 5;
    row = 0;
    for (i = 0; i < 5; i++) {
        if (index >= work->member_total_0x544) {
            return;
        }
        card = &work->members_0x560[index];
        card->valid_0x000 = 1;
        work->member_status_0x551[row] = card->status_0x029;
        work->member_busy_0x54C[row] = card->status_low_0x11D != 0;
        work->member_area_0x556[i] = card->area_0x024;
        work->member_count_0x546++;
        index++;
        row++;
    }
}

/* 0x804385B4 (0xF4): loads city page `page` (eight cities) and its page counts. */
void lb_city_page_load(LbCitySelectWork* work, s16 page) {
    s32 i;
    s32 last;

    for (i = 0; i < 8; i++) {
        work->marks_0x014[i] = 0;
        memset(&work->cities_0x024[i], 0, sizeof(NetCityView));
    }
    work->pages_0x010 = NetCtrlWk::getCityPageCount(8);
    work->total_0x012 = NetCtrlWk::getCityCount();
    NetCtrlWk::copyCityViews(work->cities_0x024, page * 8, 8, 0);
    last = work->pages_0x010 - 1;
    if (page == last) {
        work->count_0x00C = work->total_0x012 - last * 8;
    } else {
        work->count_0x00C = 8;
    }
    if (work->cursor_0x00A >= work->count_0x00C) {
        work->cursor_0x00A = work->count_0x00C - 1;
    }
    work->page_0x00E = page;
}

/* 0x804386A8 / 0x804386AC / 0x804386B0 (0x4 each): the display name of event, second filter and first filter
 * `index`. */
s8* lb_city_event_name_get(s32 index) {
    return (s8*)NetCtrlWk::getEvent(index);
}

s8* lb_city_second_filter_name_get(s32 index) {
    return (s8*)NetCtrlWk::getSecondFilter(index);
}

s8* lb_city_first_filter_name_get(s32 index) {
    return (s8*)NetCtrlWk::getFirstFilter(index);
}

/* 0x804386B4 (0x8C): the help window at the bottom with lobby help text `id`. */
void lb_city_help_draw(u16 id) {
    s16 x = get_wide_offset(4) + 0x170;
    s8* text;

    draw_window_frame_preset(x, 0x14A, 0);
    font_set_size(16, 16);
    text = (s8*)LbStr(3, id);
    if (text != NULL) {
        font_print_ex(x + 18, 0x15A, 0, text);
    }
}

/* 0x80438740 (0x308): the city list dialog: name, event and population of each city of the page. */
void lb_city_list_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s32 name_x;
    s16 count_x;
    s32 bar_x;
    s16 free_x;
    s32 count_text_x;
    s32 y;
    s32 text_y;
    s16 i;
    s16 event_x;
    bool cursor;
    u32 flags;
    s32 color;
    NetCityView* city;
    char text[0x20];

    flfntFlush();
    flfntStackReset();
    x = put_frame_dialog_title(0x10, 0x48, 0x260, 0x10A, 0, 0, 0, (s8*)LbStr(0, 0x264), 3);
    lb_page_arrow_put((s16)(x + 0x1EC), 0x3E, work->page_0x00E, work->pages_0x010, work->arrow_moved_0x008, active);
    name_x = x + 0x14;
    event_x = (s16)name_x + 0x5A;
    count_x = name_x + 0x1C0;
    free_x = name_x + 0x1FC;
    put_frame_label(name_x, 0x5C, 0x56, 0, 0, (s8*)LbStr(0, 0x270));
    put_frame_label(event_x, 0x5C, 0x162, 0, 0, (s8*)LbStr(0, 0x271));
    put_frame_label(count_x, 0x5C, 0x7A, 0, 0, (s8*)LbStr(0, 0x272));
    name_x += 10;
    event_x += 8;
    y = 0x74;
    font_set_size(18, 18);
    city = work->cities_0x024;
    bar_x = (s16)name_x - 10;
    count_text_x = count_x + 8;
    for (i = 0; i < work->count_0x00C; i++) {
        put_frame_separator(bar_x, y, 0x238, 0x1A, 0);
        if (i == work->cursor_0x00A) {
            cursor = 1;
            flags = 6;
            put_frame_cursor_bar(bar_x, y, 0x238, 0x1A, 0, 0);
        } else {
            cursor = 0;
            flags = 2;
        }
        color = GetMenuFontColor(1, cursor, active, 0);
        switch (city->state_0x0C) {
        case 0:
            color = 0xFFFF00FF;
            break;
        case 3:
        case 1:
            color = 0x646464FF;
            break;
        }
        text_y = (s16)y + 3;
        lb_font_print(name_x, text_y, (s8*)city->name_0x10, color, flags);
        if (city->state_0x0C == 0) {
            lb_font_print(event_x, text_y, (s8*)LbStr(0, 0x27C), color, flags);
        } else {
            lb_font_print(event_x, text_y, lb_city_event_name_get(city->values_0x20[3]), color, flags);
        }
        sprintf(text, "%2d/%2d", city->population_0x00, city->capacity_0x04);
        lb_font_print(count_text_x, text_y, (s8*)text, color, flags);
        sprintf(text, "(%2d)", city->population_0x00 - city->order_0x0A);
        lb_font_print(free_x, text_y, (s8*)text, color, flags);
        y += 0x1A;
        city++;
    }
    flfntFlush();
    flfntStackReset();
    lb_city_help_draw(0x1EC);
}

/* 0x80438A48 (0x374): the city list dialog of server type 3: name, first and second filter, event and population. */
void lb_city_list_ex_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s16 count_x;
    s32 name_x;
    s32 bar_x;
    s16 event_x;
    s32 count_text_x;
    s32 y;
    s32 text_y;
    s16 first_x;
    s16 i;
    bool cursor;
    u32 flags;
    s16 second_x;
    s32 color;
    NetCityView* city;
    char text[0x30];

    flfntFlush();
    flfntStackReset();
    x = put_frame_dialog_title(8, 0x48, 0x270, 0x10A, 0, 0, 0, (s8*)LbStr(0, 0x264), 3);
    lb_page_arrow_put((s16)(x + 0x1EC), 0x3E, work->page_0x00E, work->pages_0x010, work->arrow_moved_0x008, active);
    name_x = x + 0x14;
    first_x = name_x + 0x54;
    second_x = first_x + 0x36;
    event_x = second_x + 0x9E;
    count_x = event_x + 0xA8;
    put_frame_label(name_x, 0x5C, 0x52, 0, 0, (s8*)LbStr(0, 0x270));
    put_frame_label(first_x, 0x5C, 0x34, 0, 0, (s8*)LbStr(0, 0x277));
    put_frame_label(second_x, 0x5C, 0x9C, 0, 0, (s8*)LbStr(0, 0x278));
    put_frame_label(event_x, 0x5C, 0xA6, 0, 0, (s8*)LbStr(0, 0x271));
    put_frame_label(count_x, 0x5C, 0x78, 0, 0, (s8*)LbStr(0, 0x272));
    name_x += 10;
    first_x += 8;
    y = 0x74;
    font_set_size(18, 18);
    city = work->cities_0x024;
    bar_x = (s16)name_x - 10;
    count_text_x = count_x + 0x16;
    for (i = 0; i < work->count_0x00C; i++) {
        put_frame_separator(bar_x, y, 0x248, 0x1A, 0);
        if (i == work->cursor_0x00A) {
            cursor = 1;
            flags = 6;
            put_frame_cursor_bar(bar_x, y, 0x248, 0x1A, 0, 0);
        } else {
            cursor = 0;
            flags = 2;
        }
        color = GetMenuFontColor(1, cursor, active, 0);
        switch (city->state_0x0C) {
        case 0:
            color = 0xFFFF00FF;
            break;
        case 3:
        case 1:
            color = 0x646464FF;
            break;
        }
        text_y = (s16)y + 3;
        lb_font_print(name_x, text_y, (s8*)city->name_0x10, color, flags);
        if (city->state_0x0C == 0) {
            lb_font_print(first_x, text_y, (s8*)LbStr(0, 0x27C), color, flags);
        } else {
            lb_font_print(first_x, text_y, lb_city_first_filter_name_get(city->values_0x20[1]), color, flags);
            lb_font_print(second_x, text_y, lb_city_second_filter_name_get(city->values_0x20[2]), color, flags);
            lb_font_print(event_x, text_y, lb_city_event_name_get(city->values_0x20[3]), color, flags);
        }
        sprintf(text, "%2d/%2d (%2d)", city->population_0x00, city->capacity_0x04,
                city->population_0x00 - city->order_0x0A);
        lb_font_print(count_text_x, text_y, (s8*)text, color, flags);
        y += 0x1A;
        city++;
    }
    flfntFlush();
    flfntStackReset();
    lb_city_help_draw(0x1EC);
}

/* 0x80438DBC (0x39C): the member list dialog of the selected city: status icon, name, area and state of the five
 * members of the page, and the cursor prompt. */
void lb_city_member_list_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s16 name_x;
    s16 area_x;
    s16 state_x;
    s32 icon_x;
    s32 bar_x;
    s16 y;
    s32 text_y;
    s16 first;
    s32 i;
    bool cursor;
    s32 color;
    s8* name;
    NetFriendCard* card;
    _SPR_DATA_ spr;
    char text[0x20];
    _mh_ivec2_ pos;

    x = put_frame_dialog_title(0x104, 0x40, 0x168, 0xE6, 0, 0, 0, (s8*)LbStr(0, 0x269), 4);
    lb_page_arrow_put((s16)(x + 0xE6), 0x36, work->scroll_0x1140.col, work->scroll_0x1140.col_count,
                      work->scroll_0x1140.move_flags, active);
    icon_x = x + 0x14;
    name_x = icon_x + 0x3C;
    area_x = name_x + 0x6E;
    state_x = area_x + 0x3C;
    put_frame_label(icon_x, 0x54, 0x3A, 0, 0, (s8*)LbStr(0, 0x273));
    put_frame_label(name_x, 0x54, 0x6C, 0, 0, (s8*)LbStr(0, 0x274));
    put_frame_label(area_x, 0x54, 0x3A, 0, 0, (s8*)LbStr(0, 0x275));
    put_frame_label(state_x, 0x54, 0x58, 0, 0, (s8*)LbStr(0, 0x276));
    icon_x += 10;
    name_x += 8;
    area_x += 4;
    state_x += 8;
    y = 0x6C;
    first = work->scroll_0x1140.col * 5;
    font_set_size(18, 18);
    card = &work->members_0x560[first];
    bar_x = (s16)icon_x - 10;
    for (i = 0; i < 5; i++) {
        if ((s16)i < work->member_count_0x546) {
            put_frame_separator(bar_x, y, 0x140, 0x1A, 0);
            cursor = (s16)i == work->scroll_0x1140.row;
            spr_data_copy(&spr, get_lsp_data(0x18B6, NULL));
            pos.x = bar_x;
            pos.y = y;
            draw_sprite_uv_color_idx(0x18B6, work->member_status_0x551[(s16)i], GetMenuIconColor(1, cursor, active, 0),
                                     &pos);
            color = GetMenuFontColor(1, cursor, active, 0);
            if (get_option_cfg(12) == 0) {
                name = (s8*)card->name_0x00D;
            } else {
                name = (s8*)card->id_0x003;
            }
            font_set_size(24, 18);
            text_y = y + 3;
            lb_font_puts(name_x, text_y, name, color, 2);
            sprintf(text, "%3d", work->member_area_0x556[i]);
            lb_font_print(area_x, text_y, (s8*)text, color, 2);
            font_set_size(18, 18);
            lb_font_print(state_x, text_y, (s8*)LbStr(0, work->member_busy_0x54C[(s16)i] + 0x27D), color, 2);
            if (cursor == 1) {
                put_frame_cursor_bar(bar_x, y, 0x140, 0x1A, 0, 0);
            }
        }
        y += 0x1A;
        card++;
    }
    pos.x = icon_x + 0x30;
    pos.y = y + 2;
    if (active != 0) {
        if (work->scroll_0x1140.row + 1 == work->scroll_0x1140.row_limit) {
            lb_cursor_prompt_draw(1, (s16*)&pos);
        } else {
            lb_cursor_prompt_draw(0, (s16*)&pos);
        }
    } else {
        lb_cursor_prompt_draw(2, (s16*)&pos);
    }
    lb_city_help_draw(0x1ED);
}

/* 0x80439158 (0x138): the selected community's panel: its name and its leader. */
void lb_community_info_draw(LbCitySelectWork* work) {
    NetCommunityView* view = &work->communities_0x1A4[work->cursor_0x00A];
    s32 bar_x;
    s32 label_x;
    s32 x = get_wide_offset(3) + 0x10;
    s32 text_x;

    put_frame_dialog(x, 0x154, 0x13C, 0x50, 0, 0);
    label_x = (s16)x + 0xC;
    put_frame_label(label_x, 0x164, 0x6E, 0, 0, (s8*)LbStr(0, 0x26D));
    bar_x = (s16)x + 0x7C;
    put_frame_separator(bar_x, 0x161, 0xB4, 0x1A, 0);
    text_x = (s16)x + 0x80;
    lb_font_print(text_x, 0x164, (s8*)view->name_0x01, -1, 2);
    put_frame_separator(bar_x, 0x17B, 0xB4, 0x1A, 0);
    put_frame_label(label_x, 0x17E, 0x6E, 0, 0, (s8*)LbStr(0, 0x26F));
    lb_font_print(text_x, 0x17E, (s8*)view->leader_0x21, -1, 2);
}

/* 0x80439290 (0x2D8): the community list dialog: comment, event and population of each community of the page. */
void lb_community_list_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s32 name_x;
    s16 free_x;
    s16 event_x;
    s16 count_x;
    s32 text_y;
    s16 i;
    s32 y;
    bool cursor;
    u32 flags;
    s32 color;
    NetCommunityView* view;
    char text[0x20];

    flfntFlush();
    flfntStackReset();
    x = put_frame_dialog_title(0x10, 0x48, 0x260, 0x10A, 0, 0, 0, (s8*)LbStr(0, 0x264), 3);
    lb_page_arrow_put((s16)(x + 0x1EC), 0x3E, work->page_0x00E, work->pages_0x010, work->arrow_moved_0x008, active);
    name_x = x + 0x14;
    event_x = (s16)name_x + 0x5A;
    count_x = (s16)name_x + 0x1C0;
    free_x = name_x + 0x1FC;
    put_frame_label(name_x, 0x5C, 0x56, 0, 0, (s8*)LbStr(0, 0x270));
    put_frame_label(event_x, 0x5C, 0x162, 0, 0, (s8*)LbStr(0, 0x271));
    put_frame_label(count_x, 0x5C, 0x7A, 0, 0, (s8*)LbStr(0, 0x272));
    name_x += 10;
    event_x += 8;
    count_x += 8;
    y = 0x74;
    font_set_size(18, 18);
    for (i = 0; i < work->count_0x00C; i++) {
        view = &work->communities_0x1A4[i];
        put_frame_separator((s16)name_x - 10, y, 0x238, 0x1A, 0);
        if (i == work->cursor_0x00A) {
            cursor = 1;
            flags = 6;
            put_frame_cursor_bar((s16)name_x - 10, y, 0x238, 0x1A, 0, 0);
        } else {
            cursor = 0;
            flags = 2;
        }
        GetMenuFontColor(1, cursor, active, 0);
        color = -1;
        if ((u32)view->value_0x54 == (u32)view->value_0x5C) {
            color = 0x646464FF;
        }
        text_y = (s16)y + 3;
        lb_font_print(name_x, text_y, (s8*)view->comment_0x41, color, flags);
        lb_font_print(event_x, text_y, lb_city_event_name_get(view->slots_0x64[3]), color, flags);
        sprintf(text, "%2d/%2d", view->value_0x54, view->value_0x5C);
        lb_font_print(count_x, text_y, (s8*)text, color, flags);
        sprintf(text, "(%2d)", view->value_0x54 - view->value_0x58);
        lb_font_print(free_x, text_y, (s8*)text, color, flags);
        y += 0x1A;
    }
    flfntFlush();
    flfntStackReset();
    lb_community_info_draw(work);
    if (work->community_0x005 == 0) {
        lb_city_help_draw(0x1EC);
    } else {
        lb_city_help_draw(0x1F2);
    }
}

/* 0x80439568 (0x32C): the community list dialog of server type 3: comment, first and second filter, event and
 * population. */
void lb_community_list_ex_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s16 event_x;
    s16 first_x;
    s16 second_x;
    s32 name_x;
    s16 count_x;
    s32 text_y;
    s16 i;
    s32 y;
    bool cursor;
    u32 flags;
    s32 color;
    NetCommunityView* view;
    char text[0x30];

    flfntFlush();
    flfntStackReset();
    x = put_frame_dialog_title(8, 0x48, 0x270, 0x10A, 0, 0, 0, (s8*)LbStr(0, 0x264), 3);
    lb_page_arrow_put((s16)(x + 0x1EC), 0x3E, work->page_0x00E, work->pages_0x010, work->arrow_moved_0x008, active);
    name_x = x + 0x14;
    first_x = name_x + 0x54;
    second_x = first_x + 0x36;
    event_x = second_x + 0x9E;
    count_x = event_x + 0xA8;
    put_frame_label(name_x, 0x5C, 0x52, 0, 0, (s8*)LbStr(0, 0x270));
    put_frame_label(first_x, 0x5C, 0x34, 0, 0, (s8*)LbStr(0, 0x277));
    put_frame_label(second_x, 0x5C, 0x9C, 0, 0, (s8*)LbStr(0, 0x278));
    put_frame_label(event_x, 0x5C, 0xA6, 0, 0, (s8*)LbStr(0, 0x271));
    put_frame_label(count_x, 0x5C, 0x78, 0, 0, (s8*)LbStr(0, 0x272));
    name_x += 10;
    first_x += 8;
    count_x += 0x16;
    y = 0x74;
    font_set_size(18, 18);
    for (i = 0; i < work->count_0x00C; i++) {
        view = &work->communities_0x1A4[i];
        put_frame_separator((s16)name_x - 10, y, 0x248, 0x1A, 0);
        if (i == work->cursor_0x00A) {
            cursor = 1;
            flags = 6;
            put_frame_cursor_bar((s16)name_x - 10, y, 0x248, 0x1A, 0, 0);
        } else {
            cursor = 0;
            flags = 2;
        }
        GetMenuFontColor(1, cursor, active, 0);
        color = -1;
        if ((u32)view->value_0x54 == (u32)view->value_0x5C) {
            color = 0x646464FF;
        }
        text_y = (s16)y + 3;
        lb_font_print(name_x, text_y, (s8*)view->comment_0x41, color, flags);
        lb_font_print(first_x, text_y, lb_city_first_filter_name_get(view->slots_0x64[1]), color, flags);
        lb_font_print(second_x, text_y, lb_city_second_filter_name_get(view->slots_0x64[2]), color, flags);
        lb_font_print(event_x, text_y, lb_city_event_name_get(view->slots_0x64[3]), color, flags);
        sprintf(text, "%2d/%2d (%2d)", view->value_0x54, view->value_0x5C, view->value_0x54 - view->value_0x58);
        lb_font_print(count_x, text_y, (s8*)text, color, flags);
        y += 0x1A;
    }
    flfntFlush();
    flfntStackReset();
    lb_community_info_draw(work);
    lb_city_help_draw(0x1F2);
}

/* 0x80439894 (0x35C): the city search dialog: one row per filter tab with its current value, the confirm row and
 * the help text of the selected tab. */
void draw_search_city_dialog(LbCitySelectWork* work, bool active) {
    char title[0x60];
    _mh_ivec2_ pos;
    s16 height = (work->tab_count_0x130E - 1) * 0x44 + 0x46;
    s32 x;
    s16 center_x;
    s16 prompt_y;
    s16 bar_x;
    s16 y;
    s16 i;
    s32 width;
    s32 arrows;
    s32 flags;
    bool cursor;
    s8* text;
    u16 label;
    s32 help;

    if (work->community_0x005 == 0) {
        sprintf(title, (char*)LbStr(0, 0x26A), work->cities_0x024[work->cursor_0x00A].name_0x10);
    } else {
        sprintf(title, (char*)LbStr(0, 0x284), work->cities_0x024[work->cursor_0x00A].name_0x10);
    }
    x = put_frame_dialog_title(0x140, 0x40, 0x12C, height, 0, 0, 0, (s8*)title, 4);
    center_x = x + 0x96;
    prompt_y = height + 0xA;
    x += 0x14;
    y = 0x54;
    for (i = 0; i < work->tab_count_0x130E - 1; i++) {
        width = 0x104;
        arrows = 0;
        if (work->tab_kinds_0x12EC[i] != 1) {
            if (work->tab_kinds_0x12EC[i] != 0) {
                label = 0x271;
            } else {
                label = 0x279;
                width = 0x64;
                arrows = 1;
            }
        } else {
            label = 0x278;
        }
        put_frame_label(x, y, 0x6E, 0, 0, (s8*)LbStr(0, label));
        bar_x = center_x - (width >> 1);
        y += 0x1A;
        put_frame_separator(bar_x, y, width, 0x1A, 0);
        if (i == work->tab_cursor_0x130C) {
            flags = 6;
            cursor = 1;
            if (active == 1) {
                put_frame_cursor_bar(bar_x, y, width, 0x1A, 0, 0);
                if (arrows != 0) {
                    put_frame_side_arrows(bar_x, y, width, 0x1A, work->arrow_moved_0x008, 0, 0);
                }
            }
        } else {
            flags = 0;
            cursor = 0;
        }
        if (work->tab_kinds_0x12EC[i] != 0) {
            if (work->tab_kinds_0x12EC[i] != 1) {
                text = lb_city_event_name_get(work->tab_values_0x12FC[i]);
            } else {
                text = lb_city_second_filter_name_get(work->tab_values_0x12FC[i]);
            }
        } else {
            text = lb_city_first_filter_name_get(work->tab_values_0x12FC[i]);
        }
        lb_font_center_print(center_x, y + 0xD, text, 18, GetMenuFontColor(1, cursor, active, 0), flags);
        y += 0x24;
    }
    pos.x = x + 0x1E;
    pos.y = prompt_y;
    lb_cursor_prompt_draw(work->tab_kinds_0x12EC[work->tab_cursor_0x130C] == -1, (s16*)&pos);
    switch (work->tab_kinds_0x12EC[work->tab_cursor_0x130C]) {
    case 0:
        if (work->community_0x005 == 0) {
            help = 0x1EE;
        } else {
            help = 0x1F7;
        }
        break;
    case 1:
        if (work->community_0x005 == 0) {
            help = 0x1EF;
        } else {
            help = 0x1F8;
        }
        break;
    case 2:
        if (work->community_0x005 == 0) {
            help = 0x1F0;
        } else {
            help = 0x1F9;
        }
        break;
    default:
        help = 0x1FA;
        if (work->community_0x005 == 0) {
            help = 0x1F1;
        }
        break;
    }
    lb_city_help_draw(help);
}

/* 0x80439BF0 (0x1A8): the value list dialog of the selected tab (events or second filters). */
void lb_city_filter_list_draw(LbCitySelectWork* work, bool active) {
    s32 x;
    s32 text_x;
    s16 y;
    u8 kind = work->tab_kinds_0x12EC[work->tab_cursor_0x130C];
    s16 i;
    bool cursor;
    u32 flags;
    s32 color;
    s8* text;

    x = put_frame_dialog_title(0x14, 0x68, 0x12C, 0x114, 0x505070FF, 0, 0, (s8*)LbStr(0, kind == 1 ? 0x278 : 0x271), 4);
    lb_page_arrow_put((s16)(x + 0xAA), 0x5E, work->list_page_0x1318, work->list_pages_0x131A,
                      work->arrow_moved_0x008, active);
    text_x = x + 0x1E;
    y = 0x80;
    font_set_size(18, 18);
    for (i = 0; i < work->list_count_0x131E; i++) {
        put_frame_separator((s16)text_x - 10, y, 0x104, 0x1A, 0);
        if (i == work->list_cursor_0x131C) {
            cursor = 1;
            flags = 6;
            put_frame_cursor_bar((s16)text_x - 10, y, 0x104, 0x1A, 0, 0);
        } else {
            cursor = 0;
            flags = 2;
        }
        color = GetMenuFontColor(1, cursor, active, 0);
        if (kind == 1) {
            text = lb_city_second_filter_name_get(work->list_0x1320[i]);
        } else {
            text = lb_city_event_name_get(work->list_0x1320[i]);
        }
        lb_font_print(text_x, y + 3, text, color, flags);
        y += 0x1A;
    }
}

/* 0x80439D98 (0x160): the city search's countdown band (screen 28): the seconds left of the search. */
void lb_city_timer_draw(LbCitySelectWork* work) {
    Vec pos;
    f32 size[2];
    _GXColor color;
    char text[0x100];

    if (lobby_w.state_0x000 == 28) {
        pos.x = (s16)get_wide_offset(4) + 0xA0;
        pos.y = 50.0f;
        pos.z = 0.0f;
        size[0] = 140.0f;
        size[1] = 28.0f;
        color.r = 0x24;
        color.g = 0x23;
        color.b = 0x1F;
        color.a = 0xDC;
        disp_beta(&pos, (Vec2*)size, color);
        flfntSetColor(0xC0C0C0FF);
        font_set_size(18, 18);
        font_locate(4.0f + pos.x, 5.0f + pos.y);
        sprintf(text, (char*)LbStr(0, 0x299), (work->timer_0x1316 + 29) / 30);
        font_print((s8*)text);
    }
}

/* 0x80439EF8 (0x414): draws the city select's current state. */
void lb_city_select_draw(void) {
    LbCitySelectWork* work = (LbCitySelectWork*)lobby_w.menu_0xAC;
    s32 state = work->state_0x000;

    for (;;) {
        switch (state) {
        case 2:
            put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x264), 0);
            if (work->server_type_0x003 == 3) {
                lb_city_list_ex_draw(work, 1);
            } else {
                lb_city_list_draw(work, 1);
            }
            break;
        case 3:
            put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x265), 0);
            switch (work->step_0x001) {
            case 0:
                lb_city_member_list_draw(work, 1);
                break;
            case 1:
                lb_city_member_list_draw(work, 1);
                if (work->sub_0x002 == 1) {
                    put_message((s8*)jump_err_msg_str->msg_0x08, 20, 0xC0C0C0FF);
                }
                break;
            case 2:
                lb_city_member_list_draw(work, 1);
                break;
            case 3:
            case 4:
                menu_friend_list_draw((FriendCardView*)work->card_view_0x114C);
                if (work->step_0x001 == 4 && work->sub_0x002 == 1) {
                    put_message((s8*)lobby_net_jump_msg_get(work->result_0x004), 20, 0xC0C0C0FF);
                }
                break;
            }
            break;
        case 4:
            switch (work->step_0x001) {
            case 0:
                put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x265), 0);
                lb_city_member_list_draw(work, 0);
                put_message_window((s8*)LbStr(0, 0x27F), (s8*)LbStr(1, 0x2D), (s8*)LbStr(1, 0x2E), work->yes_no_0x1340, 0,
                                   -1);
                break;
            case 1:
                if (work->sub_0x002 == 1) {
                    put_message((s8*)lobby_net_jump_msg_get(work->result_0x004), 20, 0xC0C0C0FF);
                }
                break;
            }
            break;
        case 5:
            if (lobby_w.state_0x000 == 33) {
                put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x267), 0);
            } else {
                put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x266), 0);
            }
            draw_search_city_dialog(work, 1);
            switch (work->step_0x001) {
            case 1:
                lb_city_timer_draw(work);
                break;
            }
            break;
        case 6:
            if (lobby_w.state_0x000 == 33) {
                put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x267), 0);
            } else {
                put_frame_header(0, 0, 0, (s8*)LbStr(0, 0x266), 0);
            }
            draw_search_city_dialog(work, 0);
            lb_city_filter_list_draw(work, 1);
            lb_city_timer_draw(work);
            break;
        case 8:
            if (work->step_0x001 == 0 || work->step_0x001 >= 3 || lobby_w.state_0x000 == 37) {
                if (work->step_0x001 == 3) {
                    if (work->server_type_0x003 == 3) {
                        lb_community_list_ex_draw(work, 1);
                    } else {
                        lb_community_list_draw(work, 1);
                    }
                }
                if (work->server_type_0x003 == 3 && work->step_0x001 == 3 && work->sub_0x002 == 1) {
                    put_message((s8*)jump_err_msg_str->msg_0x08, 20, 0xC0C0C0FF);
                }
                break;
            }
            state = 5;
            continue;
        case 13:
            put_message((s8*)LbStr(3, 0x1F6), 18, -1);
            break;
        }
        break;
    }
}

/* 0x8043A30C (0x54): clears the lobby screen work and opens the city screen for player `self` */
void lb_city_screen_open(struct _PLW* self) {
    memset(lobby_w.menu_0xAC, 0, sizeof(LbCitySelectWork));
    lobby_w.state_0x000 = 32;
    lobby_w.active_0x008 = 1;
}

/* 0x8043A360 (0x4): closes the city move confirm. */
void lb_city_move_close(void) {
    lb_panel_close();
}

/* 0x8043A364 (0x7C): draws the city move confirm window. */
void lb_city_move_confirm_draw(void) {
    put_message_window((s8*)LbStr(3, 0x1EA), (s8*)LbStr(1, 0x2D), (s8*)LbStr(1, 0x2E),
                       ((LbCityMoveWork*)lobby_w.menu_0xAC)->yes_no_0x008, 0, 0);
}

/* 0x8043A3E0 (0x188): the city move confirm step (lb_yes_no_step, lobby_player_place), screen 32 */
void lb_city_move_confirm_step(void) {
    u8 map;
    LbCityMoveWork* work = (LbCityMoveWork*)lobby_w.menu_0xAC;
    s8* result = &work->result_0x00C;

    switch (work->step_0x000) {
    case 0:
        work->step_0x000++;
        work->delay_0x004 = 10;
        work->yes_no_0x008 = 1;
        sysSE_req(5);
        break;
    case 1:
        if (work->delay_0x004 > 0) {
            work->delay_0x004--;
        } else {
            switch (lb_yes_no_step(&work->yes_no_0x008)) {
            case 1:
                work->step_0x000++;
                break;
            case 2:
                lb_city_move_close();
                break;
            }
        }
        break;
    case 2:
        if (((s32 (*)(s8*))NetCtrlWk::requestListRefreshAlias)(result) == 0) {
            lb_city_move_close();
        } else {
            work->step_0x000++;
        }
        break;
    case 3:
        switch (*result) {
        case 1:
            releaseRemotePlayerParts();
            work->step_0x000++;
            break;
        case -1:
            lb_city_move_close();
            break;
        }
        break;
    case 4:
        lb_city_move_close();
        map = get_now_mapno();
        lobby_player_place(0, stage_entry_pos_get(map, 0), stage_entry_angle_get(map, 0));
        return;
    }
    subTransSet((u32)lb_city_move_confirm_draw, 0, NULL);
}

/* 0x8043A568 (0x1B8): collects the selectable values of list `kind` (0 events, 1 second filters, 2 first filters) for
 * the selected server type and this hunter's rank; returns the count. */
s32 lb_city_filters_collect(u8 kind) {
    LbCitySelectWork* work = (LbCitySelectWork*)lobby_w.menu_0xAC;
    PlaceUserData* user;
    s32 i;

    switch (kind) {
    case 0:
    case 1:
        for (i = 0; i < 0x40; i++) {
            work->filter_ids_0x1295[i] = -1;
        }
        work->filter_count_0x12D8 = 0;
        break;
    case 2:
        for (i = 0; i < 8; i++) {
            work->first_filters_0x12DC[i] = -1;
        }
        work->first_count_0x12E8 = 0;
        break;
    }
    switch (kind) {
    case 0:
        work->filter_count_0x12D8 = NetCtrlWk::collectEvents(work->filter_ids_0x1295, work->server_type_0x003, 0x20);
        break;
    case 1:
        user = (PlaceUserData*)get_userdata();
        work->filter_count_0x12D8 = NetCtrlWk::collectSecondFilters(work->filter_ids_0x1295, user->hunter_rank_0x3DE4,
                                                                    user->second_filter_low_0x39C0,
                                                                    user->second_filter_high_0x3AC0, 0x29, 0x40);
        break;
    case 2:
        work->first_count_0x12E8 = NetCtrlWk::collectFirstFilters(work->first_filters_0x12DC,
                                                                  ((PlaceUserData*)get_userdata())->hunter_rank_0x3DE4,
                                                                  8);
        break;
    }
    return work->filter_count_0x12D8;
}

/* Recomputes the community page count after a page load and keeps the cursor inside the page. */
static void lb_community_page_count_set(LbCitySelectWork* work) {
    work->pages_0x010 = work->total_0x012 / 8;
    if (work->total_0x012 % 8 > 0) {
        work->pages_0x010 = work->total_0x012 / 8 + 1;
    }
    if (work->cursor_0x00A >= work->count_0x00C) {
        work->cursor_0x00A = 0;
    }
}

/* 0x8043A720 (0xCF0): the community select step (screens 33 and 37): the search filters, the community list, its
 * member list and the join request. */
void lb_community_select_step(LbCitySelectWork* work) {
    MenuScroll* scroll;
    u32 last_row;
    NetSlotIds ids;
    const char* peer_id;
    NetCommunityView* view;
    s32 cards;

    work->arrow_moved_0x008 = 0;
    switch (work->state_0x000) {
    case 1:
        switch (lobby_w.state_0x000) {
        case 33:
            work->state_0x000 = 8;
            work->step_0x001 = 0;
            break;
        case 37:
            work->state_0x000 = 8;
            work->step_0x001 = 2;
            break;
        }
        work->cursor_0x00A = 0;
        work->community_0x005 = 1;
        break;
    case 8:
        switch (work->step_0x001) {
        case 0:
            lb_city_filters_collect(0);
            if (work->server_type_0x003 == 3) {
                work->first_count_0x12E8 = NetCtrlWk::collectFirstFilters(
                    work->first_filters_0x12DC, ((PlaceUserData*)get_userdata())->hunter_rank_0x3DE4, 8);
                work->first_cursor_0x12E4 = 0;
                work->tab_values_0x12FC[0] = work->first_filters_0x12DC[0];
            }
            lb_city_tabs_init(work);
            sysSE_req(5);
            work->step_0x001++;
            break;
        case 1:
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (work->tab_cursor_0x130C == work->tab_count_0x130E - 1) {
                    sysSE_req(0);
                    if (work->server_type_0x003 == 3) {
                        ids.slots_0x00[0] = -1;
                        ids.slots_0x00[1] = work->tab_values_0x12FC[0];
                        ids.slots_0x00[2] = work->tab_values_0x12FC[1];
                        ids.slots_0x00[3] = work->tab_values_0x12FC[2];
                    } else {
                        ids.slots_0x00[0] = -1;
                        ids.slots_0x00[1] = -1;
                        ids.slots_0x00[2] = -1;
                        ids.slots_0x00[3] = work->tab_values_0x12FC[0];
                    }
                    if (NetCtrlWk::submitSlotRequest(&ids, &work->result_0x004) != 0) {
                        work->step_0x001 = 2;
                    }
                } else {
                    if ((u32)(work->tab_kinds_0x12EC[work->tab_cursor_0x130C] - 1) <= 1) {
                        work->state_0x000 = 6;
                        work->list_cursor_0x131C = 0;
                        lb_city_filter_list_build(work, work->tab_kinds_0x12EC[work->tab_cursor_0x130C], 0);
                        sysSE_req(5);
                    }
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                lb_city_select_close();
                sysSE_req(1);
                return;
            } else if (lb_cmd_repeat_ck(3) != 0) {
                work->tab_cursor_0x130C = menu_cursor_step(work->tab_cursor_0x130C, work->tab_count_0x130E,
                                                           lb_cmd_repeat_get(), 1, 2);
            } else if (lb_cmd_repeat_ck(0xC) != 0 && work->tab_kinds_0x12EC[work->tab_cursor_0x130C] == 0) {
                work->first_cursor_0x12E4 = menu_cursor_step_fixed_tail(work->first_cursor_0x12E4,
                                                                        work->first_count_0x12E8, lb_cmd_repeat_get(),
                                                                        4, 8, &work->arrow_moved_0x008);
                work->tab_values_0x12FC[work->tab_cursor_0x130C] = work->first_filters_0x12DC[work->first_cursor_0x12E4];
            }
            break;
        case 2:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    lb_city_select_close();
                    sysSE_req(1);
                    return;
                }
                memset(work->communities_0x1A4, 0, sizeof(work->communities_0x1A4));
                work->count_0x00C = NetCtrlWk::copyCommunityViews(work->communities_0x1A4, work->page_0x00E * 8, 8,
                                                                  &work->total_0x012);
                if (work->total_0x012 <= 0) {
                    work->state_0x000 = 13;
                    work->msg_timer_0x136E = 30;
                    sysSE_req(5);
                } else {
                    work->step_0x001 = 3;
                    work->sub_0x002 = 0;
                    work->page_0x00E = 0;
                    work->cursor_0x00A = 0;
                    sysSE_stop(0x20);
                    lb_community_page_count_set(work);
                }
            }
            break;
        case 3:
            switch (work->sub_0x002) {
            case 0:
                if (lb_cmd_pressed_ck(0x10) != 0) {
                    view = &work->communities_0x1A4[work->cursor_0x00A];
                    if (view->valid_0x00 != 0) {
                        if ((u32)view->slots_0x64[1] != 0xFFFFFFFF &&
                            NetCtrlWk::passesFirstFilter(work->communities_0x1A4[work->cursor_0x00A].slots_0x64[1],
                                                         get_hunter_rank_max(lobby_world_block)) == 0) {
                            sysSE_req(2);
                            work->sub_0x002 = 1;
                            work->msg_timer_0x136E = 15;
                        } else {
                            sysSE_req(5);
                            work->member_count_0x546 = 0;
                            menu_scroll_init(&work->scroll_0x1140, 0, 0, 6);
                            work->state_0x000 = 3;
                            work->step_0x001 = 0;
                            work->member_page_0x548 = 0;
                            work->member_row_0x54A = 0;
                        }
                    }
                } else if (lb_cmd_pressed_ck(0x20) != 0) {
                    sysSE_req(1);
                    if (lobby_w.state_0x000 == 37) {
                        lb_talk_page_open(1);
                        work->state_0x000 = 12;
                    } else {
                        lb_city_select_close();
                        return;
                    }
                } else if (lb_cmd_repeat_ck(3) != 0) {
                    work->cursor_0x00A = menu_cursor_step(work->cursor_0x00A, work->count_0x00C, lb_cmd_repeat_get(), 1,
                                                          2);
                } else if (lb_cmd_repeat_ck(0xC) != 0) {
                    work->page_0x00E = menu_cursor_wrap(work->page_0x00E, work->pages_0x010, lb_cmd_repeat_get(), 4, 8,
                                                        6, &work->arrow_moved_0x008);
                    work->count_0x00C = NetCtrlWk::copyCommunityViews(work->communities_0x1A4, work->page_0x00E * 8, 8,
                                                                      &work->total_0x012);
                    lb_community_page_count_set(work);
                }
                break;
            case 1:
                if (work->msg_timer_0x136E > 0) {
                    work->msg_timer_0x136E--;
                } else if (lb_cmd_pressed_ck(0x10) != 0) {
                    sysSE_req(0);
                    work->sub_0x002 = 0;
                }
                break;
            }
            break;
        }
        break;
    case 4:
        switch (work->step_0x001) {
        case 0:
            switch (lb_yes_no_step(&work->yes_no_0x1340)) {
            case 1:
                view = &work->communities_0x1A4[work->cursor_0x00A];
                if ((u32)view->value_0x54 >= (u32)view->value_0x5C) {
                    NetCtrlWk::setSubError(8);
                    work->state_0x000 = 8;
                    work->step_0x001 = 3;
                    work->sub_0x002 = 0;
                } else if (((s32 (*)(s8*, s32))NetCtrlWk::requestCommand17)(&work->result_0x004, view->index_0x60) == 0) {
                    work->state_0x000 = 8;
                    work->step_0x001 = 3;
                    work->sub_0x002 = 0;
                } else {
                    work->step_0x001 = 1;
                    work->sub_0x002 = 0;
                }
                break;
            case 2:
                work->state_0x000 = 3;
                work->step_0x001 = 0;
                break;
            }
            break;
        case 1:
            switch (work->sub_0x002) {
            case 0:
                if (work->result_0x004 != 0) {
                    if (lobby_net_leave_select(work->result_0x004) == 0) {
                        if (lobby_net_jump_msg_get(work->result_0x004) != 0) {
                            work->sub_0x002 = 1;
                            work->msg_timer_0x136E = 15;
                        } else {
                            work->state_0x000 = 8;
                            work->step_0x001 = 3;
                            work->sub_0x002 = 0;
                            lb_community_page_count_set(work);
                        }
                    } else {
                        lb_city_select_close();
                    }
                }
                break;
            case 1:
                if (work->msg_timer_0x136E > 0) {
                    work->msg_timer_0x136E--;
                } else if (lb_cmd_pressed_ck(0x10) != 0) {
                    sysSE_req(0);
                    work->state_0x000 = 8;
                    work->step_0x001 = 3;
                    work->sub_0x002 = 0;
                    lb_community_page_count_set(work);
                }
                break;
            }
            break;
        case 99:
            lb_city_select_close();
            break;
        }
        break;
    case 6:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            work->state_0x000 = 8;
            work->tab_values_0x12FC[work->tab_cursor_0x130C] = work->list_0x1320[work->list_cursor_0x131C];
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            work->state_0x000 = 8;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->list_cursor_0x131C = menu_cursor_step(work->list_cursor_0x131C, work->list_count_0x131E,
                                                        lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->list_page_0x1318 = menu_cursor_wrap(work->list_page_0x1318, work->list_pages_0x131A,
                                                      lb_cmd_repeat_get(), 4, 8, 6, &work->arrow_moved_0x008);
            lb_city_filter_list_build(work, work->tab_kinds_0x12EC[work->tab_cursor_0x130C], work->list_page_0x1318);
        }
        break;
    case 3:
        switch (work->step_0x001) {
        case 0:
            work->member_cards_0x1294 = work->communities_0x1A4[work->cursor_0x00A].value_0x54;
            work->cities_0x024[work->cursor_0x00A].population_0x00 = work->member_cards_0x1294;
            cards = readCommunityMemberCards((NetCommunitySelection*)&work->communities_0x1A4[work->cursor_0x00A],
                                             work->members_0x560, 0, 10);
            if (work->member_cards_0x1294 == 0 || cards == 0) {
                work->state_0x000 = 8;
                work->step_0x001 = 3;
                lb_community_page_count_set(work);
                break;
            }
            lb_city_member_page_set(work, work->member_page_0x548);
            menu_scroll_init(&work->scroll_0x1140, 0,
                             work->member_total_0x544 + menu_page_count(work->member_total_0x544, 5), 6);
            lb_city_member_scroll_sync(&work->scroll_0x1140, &work->member_page_0x548, &work->member_row_0x54A);
            work->scroll_0x1140.row = work->scroll_0x1140.row_limit - 1;
            work->step_0x001 = 1;
            break;
        case 1:
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (work->scroll_0x1140.row + 1 == work->scroll_0x1140.row_limit) {
                    work->state_0x000 = 4;
                    work->step_0x001 = 0;
                    work->yes_no_0x1340 = 0;
                    sysSE_req(0);
                } else {
                    work->step_0x001 = 2;
                    requestPeerProfileById(
                        work->members_0x560[(s8)(work->scroll_0x1140.row + work->scroll_0x1140.col * 5)].id_0x003,
                        &work->result_0x004, 0, 0);
                    sysSE_req(5);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                work->state_0x000 = 8;
                work->step_0x001 = 3;
                sysSE_req(1);
                lb_community_page_count_set(work);
            } else {
                scroll = &work->scroll_0x1140;
                last_row = 0;
                if (scroll->row_limit == scroll->row + 1) {
                    last_row = 1;
                }
                menu_scroll_step(scroll, lb_cmd_repeat_get(), 6);
                if (scroll->move_flags & 0xC) {
                    lb_city_member_page_set(work, scroll->col);
                    work->member_page_0x548 = scroll->col;
                    if (last_row == 1) {
                        scroll->row = scroll->row_limit - 1;
                        work->member_row_0x54A = scroll->row;
                    } else if (scroll->row == scroll->row_limit - 1 && scroll->row != 0) {
                        scroll->row--;
                        work->member_row_0x54A = scroll->row;
                    }
                } else if (scroll->move_flags & 3) {
                    work->member_row_0x54A = scroll->row;
                }
            }
            break;
        case 2:
            if (work->result_0x004 != 0) {
                if (work->result_0x004 < 0) {
                    menu_scroll_init(&work->scroll_0x1140, 0,
                                     work->member_total_0x544 + menu_page_count(work->member_total_0x544, 5), 6);
                    lb_city_member_scroll_sync(&work->scroll_0x1140, &work->member_page_0x548, &work->member_row_0x54A);
                    work->step_0x001 = 1;
                } else {
                    lb_city_member_card_open(work);
                    work->step_0x001 = 3;
                }
            }
            break;
        case 3:
            switch (menu_friend_list_step((FriendCardView*)work->card_view_0x114C)) {
            case 0:
                break;
            case 2:
                peer_id = ((NetPeerCard*)lobby_state_block.block_0x11D4)->id_0x014;
                if (NetCtrlWk::requestCommand18((const s32*)&peer_id, &work->result_0x004) == 0) {
                    work->step_0x001 = 1;
                } else {
                    work->step_0x001 = 4;
                    work->sub_0x002 = 0;
                }
                break;
            default:
                work->step_0x001 = 1;
                break;
            }
            break;
        case 4:
            switch (work->sub_0x002) {
            case 0:
                if (work->result_0x004 != 0) {
                    if (lobby_net_leave_select(work->result_0x004) == 0) {
                        if (lobby_net_jump_msg_get(work->result_0x004) != 0) {
                            work->sub_0x002 = 1;
                            work->msg_timer_0x136E = 15;
                        } else {
                            work->state_0x000 = 8;
                            work->step_0x001 = 3;
                            work->sub_0x002 = 0;
                            lb_community_page_count_set(work);
                        }
                    } else {
                        lb_city_select_close();
                    }
                }
                break;
            case 1:
                if (work->msg_timer_0x136E > 0) {
                    work->msg_timer_0x136E--;
                } else if (lb_cmd_pressed_ck(0x10) != 0) {
                    sysSE_req(0);
                    work->state_0x000 = 8;
                    work->step_0x001 = 3;
                    work->sub_0x002 = 0;
                    lb_community_page_count_set(work);
                }
                break;
            }
            break;
        }
        break;
    case 12:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_city_select_close();
        }
        break;
    case 13:
        if (work->msg_timer_0x136E > 0) {
            work->msg_timer_0x136E--;
        } else if (lb_cmd_pressed_ck(0x10) != 0) {
            if (lobby_w.state_0x000 == 37) {
                work->state_0x000 = 12;
                lb_talk_page_open(1);
            } else {
                work->state_0x000 = 8;
                work->step_0x001 = 1;
                work->cursor_0x00A = 0;
            }
            sysSE_req(0);
        }
        break;
    }
    subTransSet((u32)lb_city_select_draw, 0, NULL);
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* em003's call-SE timeline. */

/* 0x80793A68 / 0x80604638 ... 0x80604714: em003's call-SE timelines, the frames and the SE ids, by call row and
 * variant. */
static s16 em003_call_se_frames0[] = {0x5E, 0x88, -1};
static s16 em003_call_se_ids0[6] = {0x2E, 0x2F, 0};
static s16 em003_call_se_frames1[6] = {0x5E, 0x88, 0xB0, 0xD8, 0x100, -1};
static s16 em003_call_se_ids1[6] = {0x60, 0x61, 0x5F, 0x60, 0x63, 0};
static s16 em003_call_se_frames2[6] = {0x48, 0x5E, 0x74, 0x88, 0x9C, -1};
static s16 em003_call_se_ids2[6] = {0x2D, 0x2E, 0x2F, 0x30, 0x31, 0};
static s16 em003_call_se_frames3[11] = {0x48, 0x5E, 0x74, 0x88, 0x9C, 0xB0, 0xC4, 0xD8, 0xEC, 0x100, -1};
static s16 em003_call_se_ids3[10] = {0x5F, 0x60, 0x61, 0x62, 0x63, 0x5F, 0x60, 0x61, 0x62, 0x63};
static s16 em003_call_se_frames4[6] = {0x48, 0x5E, 0x74, 0x88, 0x9C, -1};
static s16 em003_call_se_ids4[6] = {0x2D, 0x2E, 0x2F, 0x30, 0x31, 0};
static s16 em003_call_se_frames5[11] = {0x48, 0x5E, 0x74, 0x88, 0x9C, 0xB0, 0xC4, 0xD8, 0xEC, 0x100, -1};
static s16 em003_call_se_ids5[10] = {0x5F, 0x60, 0x61, 0x62, 0x63, 0x5F, 0x60, 0x61, 0x62, 0x63};
static s16 em003_call_se_frames6[6] = {0x48, 0x5E, 0x74, 0x88, 0x9C, -1};
static s16 em003_call_se_ids6[6] = {0x2D, 0x2E, 0x2F, 0x30, 0x31, 0};
static s16 em003_call_se_frames7[11] = {0x48, 0x5E, 0x74, 0x88, 0x9C, 0xB0, 0xC4, 0xD8, 0xEC, 0x100, -1};
static s16 em003_call_se_ids7[10] = {0x5F, 0x60, 0x61, 0x62, 0x63, 0x5F, 0x60, 0x61, 0x62, 0x63};

/* 0x80604728 / 0x80604748 (0x20 each). */
static s16* em003_call_se_frames[8] = {
    em003_call_se_frames0, em003_call_se_frames1, em003_call_se_frames2, em003_call_se_frames3,
    em003_call_se_frames4, em003_call_se_frames5, em003_call_se_frames6, em003_call_se_frames7,
};
static s16* em003_call_se_ids[8] = {
    em003_call_se_ids0, em003_call_se_ids1, em003_call_se_ids2, em003_call_se_ids3,
    em003_call_se_ids4, em003_call_se_ids5, em003_call_se_ids6, em003_call_se_ids7,
};

/* 0x8043B410 (0x14): rewinds em003's call-SE timeline. */
void em003_call_se_reset(_ENEMY_WORK* self) {
    _se_w* se = self->se_0xB14;

    se->call_se_step_0x003 = 0;
    se->call_se_timer_0xA42 = 0;
}

/* 0x8043B424 (0x150): advances em003's call-SE timeline for call `call_kind` (`variant` picks the table pair) and
 * plays the next SE when its frame comes. */
void em003_call_se_step(_ENEMY_WORK* self, u8 call_kind, u8 variant) {
    _se_w* se = self->se_0xB14;
    s32 bank;
    s32 row;
    const s16* frames;
    const s16* ids;

    switch (em003_call_act_type_ck(self)) {
    case 0:
    case 1:
        return;
    case 2:
        if (self->em003_0x328.call_kind != 0) {
            bank = get_em_se_bank(self->em003_0x328.call_kind);
            break;
        }
        return;
    }
    row = 0;
    switch (call_kind) {
    case 10:
        row = 0;
        break;
    case 31:
        row = 2;
        break;
    case 17:
        row = 4;
        break;
    case 29:
        row = 6;
        break;
    }
    row += variant;
    frames = em003_call_se_frames[row];
    ids = em003_call_se_ids[row];
    if (frames[se->call_se_step_0x003] != -1) {
        se->call_se_timer_0xA42++;
        if (se->call_se_timer_0xA42 >= frames[se->call_se_step_0x003]) {
            se_frame_req(se, ids[se->call_se_step_0x003], (bank << 24) | 3, 24);
            se->call_se_step_0x003++;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* Menu page 12 (the award list) and page 13 (the place info). */

/* 0x8043B574 (0x60): 1 when award `index` (0..23) is owned. */
u32 placeinfo_award_ck(s32 index) {
    u32 owned = 0;

    if ((1 << (index % 12)) & ((PlaceUserData*)lobby_world_block)->award_bits_0x40F8[index / 12]) {
        owned = 1;
    }
    return owned;
}

/* 0x8043B5D4 (0x40): opens page 12: 12 awards, 24 once the second set is unlocked. */
void placeinfo_page12_open(MenuSlot* slot) {
    slot->field_0x014 = 12;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = 0;
    slot->field_0x1A2 = 0;
    slot->field_0x19E = 0;
    if (((PlaceUserData*)lobby_world_block)->award_bits_0x40F8[1] != 0) {
        slot->field_0x1A3 = 24;
    } else {
        slot->field_0x1A3 = 12;
    }
}

/* 0x8043B614 (0x17C): moves the award cursor over the 6-column grid; 2 when the page closes. */
s32 placeinfo_page12_step(MenuSlot* slot) {
    s32 ret = 0;
    u16 input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    s8 rows = slot->field_0x1A3 / 6;
    s32 cursor;
    s8 col = slot->field_0x1A2 % 6;
    s8 row = slot->field_0x1A2 / 6;

    if (input & 0x20) {
        ret = 2;
        sysSE_req(1);
    } else if (input & 1) {
        if (--row < 0) {
            row = rows - 1;
        }
    } else if (input & 2) {
        if (rows <= ++row) {
            row = 0;
        }
    } else if (input & 4) {
        if (--col < 0) {
            col = 5;
        }
    } else if (input & 8) {
        if (++col >= 6) {
            col = 0;
        }
    }
    cursor = col + row * 6;
    if (slot->field_0x1A2 != cursor) {
        sysSE_req(3);
        slot->field_0x1A2 = (s8)cursor;
    }
    return ret;
}

/* 0x80604768 (0x18): the icon of each award slot (1-based). */
static u8 placeinfo_award_icons[24] = {
    1, 2, 3, 4, 5, 6, 8, 22, 9, 7, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 12,
};

/* 0x8043B790 (0x1FC): draws page 12: the award grid with the owned icons and the cursor, and the selected award's
 * name and description. */
void placeinfo_page12_draw(MenuSlot* slot) {
    s8 cursor = slot->field_0x1A2;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u8** names;
    u16* cells;
    s32 offset;
    s32 i;

    get_lsp_data(0xB49, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x144), &base);
    get_lsp_data(0xB60, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x145), &base);
    names = get_str_tbl(0x51);
    if (placeinfo_award_ck(cursor) == 1) {
        draw_font_idx(0xB62, (s8*)names[cursor], 1, &base);
    } else {
        draw_font_idx(0xB62, (s8*)names[24], 1, &base);
    }
    offset = ((PlaceUserData*)lobby_world_block)->award_bits_0x40F8[1] != 0 ? 0 : 6;
    for (i = 0; i < slot->field_0x1A3; i++) {
        cells = (u16*)get_menu_lsp_tbl(0x146);
        put_lsp_anchor_offset(cells[offset + i], &pos, base);
        draw_sprite_idx(0xB5D, &pos);
        if (cursor == i && chk_pointer() == 0) {
            draw_sprite_idx(0xB5F, &pos);
        }
        if (placeinfo_award_ck(i) == 1) {
            draw_award_icon_idx(0xB5C, placeinfo_award_icons[i] - 1, &pos);
        }
        if (cursor == i && chk_pointer() == 0) {
            draw_sprite_idx(0xB5E, &pos);
        }
    }
    get_lsp_data(0xB7E, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x147), &base);
    if (placeinfo_award_ck(cursor) == 1) {
        names = get_str_tbl(0x50);
        draw_font_idx(0xB7F, (s8*)names[cursor], 0, &base);
    }
}

/* 0x8043B98C (0xC): opens page 13. */
void placeinfo_page13_open(MenuSlot* slot) {
    slot->field_0x014 = 13;
}

/* 0x8043B998 (0x58): 2 when page 13 closes. */
s32 placeinfo_page13_step(MenuSlot* slot) {
    s32 ret = 0;

    if ((u16)(slot->buttons_trig | (slot->field_0x008 & 0xF)) & 0x20) {
        ret = 2;
        sysSE_req(1);
    }
    return ret;
}

void placeinfo_world_draw(MenuSlot* slot);
void placeinfo_net_draw(MenuSlot* slot);

/* 0x8043B9F0 (0x48): draws page 13: the network place info online, the world progress offline. */
void placeinfo_page13_draw(MenuSlot* slot) {
    if (game_ready_ck() == 1) {
        placeinfo_net_draw(slot);
    } else {
        placeinfo_world_draw(slot);
    }
}

/* 0x8043BA38 (0x624): draws the offline place info: the village's resource points, the kujira event, the farm
 * plots and the trade ships. */
void placeinfo_world_draw(MenuSlot* slot) {
    char text[0x10];
    _mh_ivec2_ base;
    _mh_ivec2_ frame;
    _mh_ivec2_ row;
    u32 kujira_open = userdata_progress_flag_ck(0x1C);
    u32 farm_open = userdata_progress_flag_ck(0x1A);
    u32 ships_open = userdata_progress_flag_ck(0x19);
    u16* frames;
    u16* rows;
    PlaceUserData* ud;
    PlaceFarmPlot* plot;
    PlaceShip* ship;
    u8 plots;
    u8 days;
    s32 i;

    get_lsp_data(0x28D1, &base);
    draw_sprite_idx(0x28DE, &base);
    frames = (u16*)get_menu_lsp_tbl(0x149);
    rows = (u16*)get_menu_lsp_tbl(0x151);
    put_lsp_anchor_offset(frames[0], &frame, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x150), &frame);
    draw_sprite_idx(0x290F, &frame);
    put_lsp_anchor_offset(rows[0], &row, frame);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x152), &row);
    draw_font_idx(0x291A, (s8*)LbStr(0, 6), 0, &row);
    sprintf(text, "%d", ((PlaceUserData*)lobby_world_block)->resource_points_0x3F04);
    draw_font_idx(0x291B, (s8*)text, 0, &row);
    if (kujira_open == 1) {
        ud = (PlaceUserData*)lobby_world_block;
        put_lsp_anchor_offset(rows[1], &row, frame);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x152), &row);
        draw_font_idx(0x291A, (s8*)LbStr(0, 0x285), 0, &row);
        if (kujira_event_over_ck() == 1) {
            if (-(ud->kujira_days_0x4834 / 2) == 0) {
                sprintf(text, (char*)LbStr(0, 0x198));
                draw_font_idx(0x291B, (s8*)text, 0, &row);
            } else {
                draw_font_idx(0x291B, (s8*)LbStr(0, 0x286), 0, &row);
                days = ud->kujira_days_0x4834;
                if (-(s8)days & 1) {
                    sprintf(text, (char*)LbStr(0, 0x197), -((s8)days / 2));
                } else {
                    sprintf(text, (char*)LbStr(0, 0x196), -((s8)days / 2));
                }
                draw_font_idx(0x291C, (s8*)text, 2, &row);
            }
        } else {
            draw_font_idx(0x291B, (s8*)LbStr(0, 0x287), 0, &row);
        }
    }
    plots = 0;
    if (farm_open == 1) {
        plots = lb_world_submode_get();
        if (plots != 0) {
            put_lsp_anchor_offset(frames[1], &frame, base);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x150), &frame);
            draw_sprite_idx(0x2915, &frame);
            draw_sprite_idx(0x2910, &frame);
            for (i = 0; i < plots; i++) {
                plot = &((PlaceUserData*)lobby_world_block)->farm_plots_0x3F38[i];
                put_lsp_anchor_offset(rows[i], &row, frame);
                draw_sprite_ary((u16*)get_menu_lsp_tbl(0x152), &row);
                draw_font_idx(0x291A, (s8*)LbStr(0, i + 0x141), 0, &row);
                switch (plot->state_0x0) {
                case 0:
                    draw_font_idx(0x291B, (s8*)LbStr(0, 0x286), 0, &row);
                    break;
                case 1:
                    draw_font_idx(0x291B, (s8*)LbStr(0, 0x288), 0, &row);
                    break;
                case 2:
                    if (plot->crop_0x3 != 0xFF) {
                        draw_font_idx(0x291B, (s8*)LbStr(0, plot->crop_0x3 + 0x15C), 0, &row);
                        draw_font_idx(0x291C, (s8*)LbStr(0, plot->stage_0x4 + 0x148), 2, &row);
                    } else {
#line 288
                        nw4r::db::Panic(__FILE__, __LINE__, "NW4R:Failed assertion 0");
                    }
                    break;
                default:
#line 292
                    nw4r::db::Panic(__FILE__, __LINE__, "NW4R:Failed assertion 0");
                    break;
                }
            }
        }
    }
    if (ships_open == 1) {
        put_lsp_anchor_offset((frames + plots)[1], &frame, base);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x150), &frame);
        draw_sprite_idx(0x2915, &frame);
        draw_sprite_idx(0x2911, &frame);
        ud = (PlaceUserData*)lobby_world_block;
        for (i = 0; i < ud->ship_count_0x4670; i++) {
            ship = &ud->ships_0x4658[i];
            put_lsp_anchor_offset(rows[i], &row, frame);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x152), &row);
            draw_font_idx(0x291A, (s8*)lb_speaker_name_get(i + 20), 0, &row);
            if (ship->item_0x0 != 0) {
                if (ship->count_0x4 != 0) {
                    draw_font_idx(0x291B, (s8*)ItemName(ship->item_0x0), 0, &row);
                    sprintf(text, (char*)LbStr(0, 0x28B), ship->count_0x4, ship->max_0x5);
                    draw_font_idx(0x291C, (s8*)text, 2, &row);
                } else {
                    draw_font_idx(0x291B, (s8*)LbStr(0, 0x289), 0, &row);
                }
            } else {
                draw_font_idx(0x291B, (s8*)LbStr(0, 0x28A), 0, &row);
            }
        }
    }
    get_lsp_data(0x28D1, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x148), &base);
    draw_sprite_idx(0x28E0, &base);
}

/* 0x8043C05C (0x88): draws the online place info: the server and layer panels of this player. */
void placeinfo_net_draw(MenuSlot* slot) {
    _mh_ivec2_ base;

    get_lsp_data(0x28D1, &base);
    draw_sprite_idx(0x28DC, &base);
    draw_sprite_idx(0x28DD, &base);
    placeinfo_server_rows_draw(0x28E9, 0, NULL);
    placeinfo_settings_rows_draw(0x28FB, 0, NULL);
    get_lsp_data(0x28D1, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x148), &base);
    draw_sprite_idx(0x28DF, &base);
}

/* 0x8043C0E4 (0x2D0): draws the server panel at sprite `id`: server type, server, account and nickname, of this
 * player or of the peer `card`; `framed` adds the row frames. */
void placeinfo_server_rows_draw(u16 id, u32 framed, NetPeerCard* card) {
    _mh_ivec2_ base;
    _mh_ivec2_ row;
    u16* rows;

    get_lsp_data(id, &base);
    if (framed == 1) {
        draw_sprite_idx(0x28EE, &base);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14A), &base);
    rows = (u16*)get_menu_lsp_tbl(0x14B);
    put_lsp_anchor_offset(rows[0], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14C), &row);
    if (framed == 1) {
        draw_sprite_idx(0x28FA, &row);
    }
    draw_font_idx(0x28F5, (s8*)LbStr(0, 0x281), 4, &row);
    if (card == NULL) {
        draw_font_idx(0x28F6, (s8*)NetCtrlWk::getSelectedServerTypeName(), 0, &row);
    } else {
        draw_font_idx(0x28F6, (s8*)get_server_type_name(card->server_0x20A), 0, &row);
    }
    put_lsp_anchor_offset(rows[1], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14C), &row);
    if (framed == 1) {
        draw_sprite_idx(0x28FA, &row);
    }
    draw_font_idx(0x28F4, (s8*)LbStr(0, 0x26D), 4, &row);
    if (card == NULL) {
        draw_font_idx(0x28F6, (s8*)NetCtrlWk::getSelectedServerName(), 0, &row);
    } else {
        draw_font_idx(0x28F6, (s8*)card->names_0x14A[0], 0, &row);
    }
    put_lsp_anchor_offset(rows[2], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14C), &row);
    if (framed == 1) {
        draw_sprite_idx(0x28FA, &row);
    }
    draw_font_idx(0x28F4, (s8*)LbStr(0, 0x26F), 4, &row);
    if (card == NULL) {
        draw_font_idx(0x28F6, (s8*)NetCtrlWk::getAccountNameOrBlank(), 0, &row);
    } else {
        draw_font_idx(0x28F6, (s8*)card->names_0x14A[1], 0, &row);
    }
    put_lsp_anchor_offset(rows[3], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14C), &row);
    if (framed == 1) {
        draw_sprite_idx(0x28FA, &row);
    }
    draw_font_idx(0x28F4, (s8*)LbStr(0, 0x270), 4, &row);
    if (card == NULL) {
        draw_font_idx(0x28F6, (s8*)NetCtrlWk::getNicknameOrBlank(), 0, &row);
    } else {
        draw_font_idx(0x28F6, (s8*)card->names_0x14A[2], 0, &row);
    }
}

/* 0x8043C3B4 (0x34C): draws the layer panel at sprite `id`: the event, the two filters and the city settings, of
 * this player or of the peer `card`; `framed` adds the row frames. */
void placeinfo_settings_rows_draw(u16 id, u32 framed, NetPeerCard* card) {
    char text[0x10];
    char card_text[0x10];
    _mh_ivec2_ base;
    _mh_ivec2_ row;
    u16* rows;
    u8 value;
    s32 high;

    get_lsp_data(id, &base);
    if (framed == 1) {
        draw_sprite_idx(0x2900, &base);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14D), &base);
    if (framed == 1) {
        draw_sprite_idx(0x2902, &base);
    }
    rows = (u16*)get_menu_lsp_tbl(0x14E);
    put_lsp_anchor_offset(rows[0], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14F), &row);
    if (framed == 1) {
        draw_sprite_idx(0x290D, &row);
    }
    draw_font_idx(0x2908, (s8*)LbStr(0, 0x271), 4, &row);
    if (card == NULL) {
        value = NetCtrlWk::getLayerSetting(3);
    } else {
        value = card->settings_0x20B[3];
    }
    if (value != 0xFF) {
        draw_font_idx(0x2909, (s8*)NetCtrlWk::getEvent(value)->name_0x00, 0, &row);
    }
    put_lsp_anchor_offset(rows[1], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14F), &row);
    if (framed == 1) {
        draw_sprite_idx(0x290D, &row);
    }
    draw_font_idx(0x2908, (s8*)LbStr(0, 0x279), 4, &row);
    if (card == NULL) {
        value = NetCtrlWk::getLayerSetting(1);
    } else {
        value = card->settings_0x20B[1];
    }
    if (value != 0xFF) {
        draw_font_idx(0x2909, (s8*)NetCtrlWk::getFirstFilter(value)->name_0x00, 0, &row);
    }
    put_lsp_anchor_offset(rows[2], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14F), &row);
    if (framed == 1) {
        draw_sprite_idx(0x290D, &row);
    }
    draw_font_idx(0x2908, (s8*)LbStr(0, 0x278), 4, &row);
    if (card == NULL) {
        value = NetCtrlWk::getLayerSetting(2);
    } else {
        value = card->settings_0x20B[2];
    }
    if (value != 0xFF) {
        draw_font_idx(0x2909, (s8*)NetCtrlWk::getSecondFilter(value)->name_0x00, 0, &row);
    }
    put_lsp_anchor_offset(rows[3], &row, base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x14F), &row);
    if (framed == 1) {
        draw_sprite_idx(0x290D, &row);
    }
    draw_font_idx(0x2908, (s8*)LbStr(0, 0x282), 4, &row);
    if (card == NULL) {
        if (NetCtrlWk::getLayerSetting(5) != 0) {
            high = NetCtrlWk::getLayerSetting(5);
            sprintf(text, "%d/%d", NetCtrlWk::getLayerSetting(4), high);
            draw_font_idx(0x2909, (s8*)text, 0, &row);
        }
    } else if ((u8)card->value_0x210 != 0) {
        sprintf(card_text, "%d/%d", card->value_0x20F, card->value_0x210);
        draw_font_idx(0x2909, (s8*)card_text, 0, &row);
    }
}

/* 0x806047B0 (0x20): the placed-model flags by interior kind (1: start the motion, 2: build the UV animation). */
static u8 place_interior_flags[0x20] = {
    1, 1, 1, 0, 0, 0, 2, 1, 1, 0, 0, 0, 2, 2, 2, 0, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

/* 0x8043C700 (0x38): copies the 0x11 place ids into the lobby work, or clears them when `info` is NULL. */
void place_info_set(u8* info) {
    if (info == NULL) {
        memset(lobby_w.place_ids_0x134, 0xFF, sizeof(lobby_w.place_ids_0x134));
        return;
    }
    memcpy(lobby_w.place_ids_0x134, info, sizeof(lobby_w.place_ids_0x134));
}

/* 0x8043C738 (0x74): 1 for areas 5..7 of map 21 and area 1 of map 22. */
u32 lb_city_area_ck(u8 area) {
    switch (get_now_mapno()) {
    case 21:
        if ((u32)(area - 5) <= 2) {
            return 1;
        }
        break;
    case 22:
        if (area == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8043C7AC (0x94): clears the eight placed models and the loaded map/area. */
void place_model_work_init(void) {
    PlaceModel* model;
    s32 i;

    PlaceModelWork* work = &place_model_work;

    work->map = 0xFF;
    work->area = 0xFF;
    model = work->models;
    for (i = 0; i < 8; i++) {
        model->anm = NULL;
        model->works[0] = NULL;
        model->works[1] = NULL;
        memset(model->chr_bytes, 0, sizeof(model->chr_bytes));
        model->used = 0;
        model->interior_id = 0xFF;
        model->flags = 0;
        model->res_index = 0xFF;
        model++;
    }
}

void place_model_release_all(void);
void place_model_load(u8 mode, u8 index, u8 id, u8 map, u8 area);
void place_model_release(u8 index);
void place_model_group_load(u8 mask, u8 id);

/* 0x8043C840 (0x160): loads the placed models of the lobby rooms (map 21 areas 5..7, map 22 area 1). */
void place_model_area_load(u8 map, u8 area) {
    LbLobbyWork* lobby;
    PlaceModelWork* work = &place_model_work;
    u8 first = 0;
    u8 count = 0;
    s32 i;

    switch (map) {
    case 21:
        switch (area) {
        case 5:
            get_option_pair(area, &first, &count);
            break;
        case 6:
            get_option_pair(area, &first, &count);
            break;
        case 7:
            get_option_pair(area, &first, &count);
            break;
        default:
            return;
        }
        break;
    case 22:
        switch (area) {
        case 1:
            get_option_pair(area, &first, &count);
            break;
        default:
            return;
        }
        break;
    default:
        return;
    }
    if (work->map != 0xFF || work->area != 0xFF) {
        place_model_release_all();
    }
    for (i = 0, lobby = &lobby_w; i < count; i++) {
        if (lobby->place_ids_0x134[i + first] != 0xFF) {
            place_model_load(0, i, lobby->place_ids_0x134[i + first], map, area);
        }
    }
    if (work->map == 0xFF) {
        work->map = map;
    }
    if (work->area == 0xFF) {
        work->area = area;
    }
}

/* 0x8043C9A0 (0x7C): builds every loaded model whose resource has not been built yet. */
void place_model_create_pending(void) {
    PlaceModel* model;
    u8 i;

    for (i = 0, model = place_model_work.models; i < 8; model++, i++) {
        if (model->used != 0 && model->interior_id != 0xFF && model->res_index == 0xFF) {
            placeinfo_model_create(i, lb_interior_path_get(model->interior_id));
        }
    }
}

/* 0x8043CA1C (0x1C0): replaces the placed models whose ids changed by the peer's eight ids, then stores them. */
void pig_name_set(const char* name) {
    u8* ids = (u8*)name;
    u8 first = 0;
    u8 count = 0;
    u8 map = lobby_w.field_0x001;
    u8 area = lobby_w.field_0x002;
    u8 changed[8];
    s32 i;
    s32 j;
    u8* entry;
    u8* other;
    u8 mask;

    memset(changed, 0xFF, sizeof(changed));
    switch (map) {
    case 21:
        switch (area) {
        case 5:
            get_option_pair(area, &first, &count);
            break;
        case 6:
            get_option_pair(area, &first, &count);
            break;
        case 7:
            get_option_pair(area, &first, &count);
            break;
        default:
            return;
        }
        break;
    default:
        return;
    }
    for (i = 0, entry = changed; i < count; i++, entry++) {
        if (lobby_w.place_ids_0x134[i + first] != ids[first + (i)]) {
            place_model_release(i);
            *entry = ids[first + (i)];
        }
    }
    for (i = 0, entry = changed; i < 8; i++, entry++) {
        if (*entry != 0xFF) {
            mask = 1 << i;
            for (j = i + 1, other = &changed[j]; j < 8; j++, other++) {
                if (*entry == *other) {
                    *other = 0xFF;
                    mask |= (u8)(1 << j);
                }
            }
            *entry = 0xFF;
            place_model_group_load(mask, ids[first + (i)]);
        }
    }
    place_info_set(ids);
}

/* 0x8043CBDC (0xE0): loads interior `id` into every free slot of `mask` (within the room's model count). */
void place_model_group_load(u8 mask, u8 id) {
    u8 first;
    u8 count;
    PlaceModel* model;
    s32 i;
    u8 kind = lb_interior_kind_get(id);

    for (i = 0, model = place_model_work.models; i < 8; model++, i++) {
        if (mask & (1 << i)) {
            if (model->used == 1) {
                return;
            }
            get_option_pair(lobby_w.field_0x002, &first, &count);
            if (i < 0 || i >= count) {
                return;
            }
            model->used = 1;
            model->interior_id = kind;
            model->flags = place_interior_flags[kind];
            model->anm = NULL;
        }
    }
    lb_interior_load(2, mask, kind);
}

/* 0x8043CCBC (0xDC): loads interior `id` into slot `index` and records the map/area of the first load. */
void place_model_load(u8 mode, u8 index, u8 id, u8 map, u8 area) {
    u8 first;
    u8 count;
    PlaceModelWork* work = &place_model_work;
    u8 kind = lb_interior_kind_get(id);
    PlaceModel* model;

    if (work->map == 0xFF) {
        work->map = map;
    }
    if (work->area == 0xFF) {
        work->area = area;
    }
    model = work->models + index;
    if (model->used != 1) {
        get_option_pair(area, &first, &count);
        if (index < count) {
            model->used = 1;
            model->interior_id = kind;
            model->flags = place_interior_flags[kind];
            work->models[index].anm = NULL;
            lb_interior_load(mode, index, kind);
        }
    }
}

/* 0x8043CD98 (0x1D0): releases slot `index`: its animation works, model and interior, and the resource when no
 * other slot uses it. */
void place_model_release(u8 index) {
    PlaceModelWork* work = &place_model_work;
    u32 last = 1;
    MHchar* chr;
    s32 i;
    PlaceModel* slot;
    PlaceModel* model = &work->models[index];
    u8 res;

    if (model->used != 0) {
        model->used = 0;
        for (i = 1; i >= 0; i--) {
            if (model->works[i] != NULL) {
                push_g3d_wk(model->works[i]);
                model->works[i] = NULL;
            }
        }
        chr = &model->chr;
        mhchar_model_release(chr);
        if (work->models[index].chr.g3d_0x10C != NULL) {
            push_g3d_wk(work->models[index].chr.g3d_0x10C);
            work->models[index].chr.g3d_0x10C = NULL;
        }
        mhchar_reset(chr);
        work->models[index].chr.field_0x34 = 0;
        lb_interior_release(work->models[index].interior_id);
        slot = &work->models[index];
        res = slot->res_index;
        if (res != 0xFF) {
            for (i = 0; i < 8; i++) {
                if (work->models[i].used != 0 && i != index && res == work->models[i].res_index) {
                    last = 0;
                    break;
                }
            }
            if (last == 1) {
                nwDelResource(res);
            }
        }
        work->models[index].interior_id = 0xFF;
        work->models[index].flags = 0;
        model->anm = NULL;
        slot->res_index = 0xFF;
    }
}

/* 0x8043CF68 (0x58): releases all eight slots and forgets the loaded map/area. */
void place_model_release_all(void) {
    PlaceModelWork* work = &place_model_work;
    s32 i;

    for (i = 0; i < 8; i++) {
        place_model_release(i);
    }
    work->map = 0xFF;
    work->area = 0xFF;
}

/* 0x8043CFC0 (0x138): moves and binds the built models while the player is in the rooms they were loaded for. */
void place_model_step(void) {
    PlaceModel* model = place_model_work.models;
    s32 i;

    if (place_model_work.map == 0xFF || place_model_work.area == 0xFF) {
        return;
    }
    if (place_model_work.map != get_now_mapno() || place_model_work.area != get_now_areano()) {
        place_model_release_all();
    }
    switch (get_now_mapno()) {
    case 22:
        switch (get_now_areano()) {
        case 1:
            break;
        default:
            return;
        }
        break;
    case 21:
        if ((u32)(get_now_areano() - 5) > 2) {
            return;
        }
        break;
    default:
        return;
    }
    for (i = 0; i < 8; model++, i++) {
        if (model->used != 0 && model->chr.field_0x34 != 0) {
            model->chr.field_0xF2 = 1;
            model->chr.move(0);
            if (model->anm != NULL) {
                model->anm->UpdateFrame();
            }
            g3d_root_model_bind(pRoot, model->chr.field_0x118);
        }
    }
}

/* 0x8043D0F8 (0x284): builds slot `index`'s model from the loaded file `name`, places it and builds its UV animation
 * when the interior asks for one; 1 on success. */
s32 placeinfo_model_create(u8 index, char* name) {
    EftResFileHandle res;
    s32 base;
    PlaceModel* model;
    MHchar* chr;
    struct _g3d_work** works;
    u8 flags;
    u8 map;
    u8 area;
    ResEntry* entry;
    u32 file;
    nw4r::g3d::AnmObj* anm;

    res_file_ctor(&res.data_0x00, 0);
    nw4r::g3d::ResMdl mdl((void*)0);
    base = 0;
    map = place_model_work.map;
    area = place_model_work.area;
    model = &place_model_work.models[index];
    chr = &model->chr;
    works = model->works;
    flags = model->flags;
    if (chr == NULL) {
        return 0;
    }
    entry = ckResourceName(name);
    if (entry == NULL) {
        return 0;
    }
    place_model_work.models[index].res_index = entry->index;
    res_file_assign(&res.data_0x00, res_file_ctor(&file, (u32)entry->data));
    mhchar_reset(chr);
    chr->g3d_0x10C = NULL;
    mhchar_model_create(chr, &res.data_0x00, 3, 2);
    chr->field_0x40 = 0;
    chr->ready = 1;
    switch (map) {
    case 22:
        switch (area) {
        case 1:
            base = 0;
            break;
        }
        break;
    case 21:
        switch (area) {
        case 5:
            base = 3;
            break;
        case 6:
            base = 5;
            break;
        case 7:
            base = 9;
            break;
        }
        break;
    }
    vec_to_mh_vec3(&chr->pos_0x04, &(lb_area_view_pos_tbl + base)[index].pos);
    chr->field_0x28 = 0;
    chr->field_0x2C = (lb_area_view_pos_tbl + base)[index].angle;
    chr->field_0x30 = 0;
    setVector3(&chr->scale_0x1C, 1.0f, 1.0f, 1.0f);
    if (flags & 1) {
        chr->frame_init(0, 0, 0.0f, 0, 1.0f);
    }
    if (!(flags & 2)) {
        return 1;
    }
    nw4r::g3d::ResMdl scn_mdl = ((nw4r::g3d::ScnMdlSimple*)chr->field_0x118)->GetResMdl();
    res_mdl_copy_ctor((ResHandle*)&mdl, (ResHandle*)&scn_mdl);
    if (nw_res_anm_work_num(res, *(EftResFileHandle*)&mdl, 5, works) <= 1) {
        anm = (nw4r::g3d::AnmObj*)nw_res_anm_create(res, *(EftResFileHandle*)&mdl,
                                                    (nw4r::g3d::ScnMdlSimple*)chr->field_0x118, 5, works[0], &works[1],
                                                    1.0f);
        if (anm != NULL) {
            model->anm = anm;
            return 1;
        }
    }
    if (*works != NULL) {
        push_g3d_wk(*works);
    }
    *works = NULL;
    return 0;
}

/* 0x8043D37C (0x110): 0 while a loaded slot's model is not built yet. */
u32 place_model_ready_ck(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (place_model_work.models[i].used != 0 && place_model_work.models[i].res_index == 0xFF) {
            return 0;
        }
    }
    return 1;
}

} /* extern "C" */

#pragma dont_inline on
