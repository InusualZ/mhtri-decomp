/*
 * lobby/lb_server_sel_trans.cpp - the demo/event work, the lobby player's SE frame callback, the combine/trade counter
 *   (screen 0x1A), the NPC talk window and the server-select screen (screen 0x1B).
 * RANGE. .text 0x803C3A5C-0x803C987C (87 functions); extab 0x8001936C-0x80019554, extabindex 0x80039B88-0x80039E64,
 *   .data 0x805F8920-0x805F8DB0, .bss 0x806D2B20-0x806D2C60 (`demo_work`, `npc_talk_work`), .sdata 0x80793738-0x80793900,
 *   .sbss 0x80794C90-0x80794C98 (`qnpc_talk_data`), .sdata2 0x8079C680-0x8079C690.  The head 0x803C3A5C-0x803C4B74
 *   follows `menu/get_pop_dat_ptr.cpp`'s pop-data/option code; the right edge 0x803C987C is the left edge of
 *   `Network/NetworkStreamSink.cpp` (its header holds the seam evidence).
 * FLAGS. `cflags_menu` (the row sits in the menu lib); `#pragma pool_data off` below (the retail code addresses each
 *   table by its own symbol, never through one data base register).
 * NAMES. Module `lobby` and `lb_server_sel_trans` are a GUESS from the band's server-selection transition code; no
 *   `__FILE__` string covers it.  Every own row below is a GUESS from its body and callers:
 *   GUESS (from `menu/multi_result.cpp`'s NPC talk callers): npc_talk_start, npc_talk_active_ck, talk_msg_speaker_get,
 *   talk_msg_choice_ck, npc_talk_end, talk_msg_index_get, npc_talk_flag_set.
 *   GUESS (demo work): demo_work_start, demo_work_update, demo_sound_update, demo_work_start_ex, demo_slot_set,
 *   demo_slot_queue, demo_map_slot_set, demo_slot_ready_ck, demo_slot_play, demo_fade_update, demo_effect_update;
 *   data demo_def_<flag> (the `.sdata` demo records, named by progress flag), demo_fade_keys, demo23_break_keys,
 *   demo23_break_pos, demo27_stage_frames.
 *   GUESS (screen 0x1A): lb_screen26_* (open/close/step, trade_*, pick_*, combo_*, the draws), lb_trade_state_get;
 *   data lb_combo_price_tbl, lb_s26_* sprite runs, lb_trade_rewards_N and lb_trade_offers_N with their tables,
 *   lb_trade_reward_odds.  The existing row name `userdata_server_rows_reset` is kept (it counts the trade wait down).
 *   GUESS (talk window): npc_talk_init/reset/update/draw/flag_get/text_expand/input_step, npc_talk_work,
 *   qnpc_talk_data, npc_talk_item_fmt, npc_talk_str_fmt.
 *   GUESS (screen 0x1B): lb_server_select_close/rooms_open/rooms_build/type_info_draw/types_draw/title_draw/
 *   help_draw/servers_draw/rooms_draw/draw.
 * RESIDUALS. 17 rows below 100 %, register/stack colouring or one scheduling pair each (hill-climbed declaration
 *   orders are applied): lb_screen26_trade_list_draw, lb_screen26_combo_list_draw, lb_screen26_trade_tabs_draw,
 *   npc_talk_start, lb_server_select_types_draw, lb_screen26_trade_commit, lb_server_select_rooms_build,
 *   lb_server_select_rooms_draw, lb_screen26_status_draw, lb_server_select_help_draw, lb_screen26_trade_build,
 *   lb_screen26_picks_draw, lb_server_select_servers_draw, lb_server_select_screen_step, lb_screen26_box_draw,
 *   lb_screen26_step, lb_screen26_combo_items_draw.
 *   npc_talk_start is 8 bytes long (msg * 8 is recomputed after the conditional `choice` store; retail keeps it), so
 *   `.text` is 0x5E28 against 0x5E20.  lb_screen26_step: the one-case compare is `cmplwi 2` where retail has `cmpwi 2`.
 *   The server-select list draws keep `x` sign-extended at its assignment where retail extends it at each use.
 *   `.sdata` ends at +0x1C3 against +0x1C8 and `.sbss` is 4 bytes against 8: unreferenced tail words, contents unknown.
 *   flipcheck: the registry callee names (sound/AI/light/stage demo hooks, SE bank requests, font typewriter, combo
 *   yield and talk name tables, lb_list_dialog_draw, lb_item_cell_draw_layout11, userdata_rank_band_get) are not map
 *   names until the registry lands.  Cast views at the call sites: toggle_word_step_c1, lb_item_cell_draw_layout1_c1,
 *   pl_step_se_kind_get_c1 (the owner returns a byte, `Pl/pl_act.h` spells u32),
 *   request_list_refresh_c1, request_command_0f_c1 (requests lobby-server-sel#1-4), demo_bgm_load as `s8`,
 *   and the eft052_page_put/take leaves spell the player-first argument list the retail callers pass.
 * SHAPES. The tables are defined where the retail emission order puts them: MWCC emits a function's string literals
 *   and switch tables when it generates the function, after every global defined above it, so the counter's tables
 *   follow the SE callback (its jump table) and the trade tables follow the talk window (its 16-byte format buffer);
 *   the 16-byte `npc_talk_item_fmt` and 8-byte `npc_talk_str_fmt` buffers reproduce the retail padding.
 */


#include "types.h"

/* The retail functions address each table of this unit by its own symbol (no shared data base register). */
#pragma pool_data off

#include "nw4r/math.h"       /* nw4r::math::VEC3, the 3-float record VEC3_ctor takes */
#include "mh3_pad/vec3.h"    /* VEC3_ctor (owner src/mh3_pad.cpp) */
#include "fn_8004CAD8.h"     /* userdata_progress_flag_ck/_set, unlock_bit_raise (owner src/fn_8004CAD8.cpp) */
#include "hud/layout.h"     /* _mh_ivec2_, which the band header's prototypes name */
#include "Runtime.PPCEABI.H/memset.h" /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "unsplit/menu.h"    /* option_w, this band's own tables, its unowned callees */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "lobby/lb_server_sel_trans.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/lb_server_leave_ck.h"
#include "lobby/lb_server_ready_ck.h"
#include "ef/GameMode_ck.h"
#include "ef/fn_800CDB2C.h"          /* PlayMode_ck */
#include "ef/get_move_work_adrs.h"
#include "ef/eft028_set_koware.h"
#include "stage/get_now_mapno.h"
#include "stage/stg_w.h"             /* stage_map_kind_get, get_now_areano */
#include "stage/stage_demo_start.h"
#include "stage/stage_demo_flags_set.h"
#include "menu/demo_play_ck.h"
#include "menu/gallery_demo_running_ck.h"
#include "enemy/enemy_control.h"     /* emc_event_* */
#include "enemy/ENEMY_WORK.h"        /* _ENEMY_WORK, move work 3's records */
#include "camera/camera.h"           /* camera_work_ck */
#include "camera/camera_lobby_mode_offset_req.h"
#include "camera/camera_event_length_get.h"
#include "camera/stage_dcm_buffer_tbl.h"
#include "sound/demo_bgm_load.h"
#include "sound/demo_bgm_ready_ck.h"
#include "sound/stage_bgm_reload.h"
#include "sound/set_BGM_volume.h"
#include "sound/set_SE_volume.h"
#include "sound/set_str_bgm_main_vol.h"
#include "sound/set_str_se_main_vol.h"
#include "sound/demo_stream_play.h"
#include "sound/demo_stream_stop_ck.h"
#include "sound/demo_stream_stop.h"
#include "fn_80056F24.h"             /* fade_set */
#include "Pl/pl_act.h"               /* Pl_motion_input_ck */
#include "Pl/root_wait_frames_set.h"
#include "ai/ainpc_demo_start.h"
#include "ai/ainpc_demo_end.h"
#include "light/light_demo_start.h"
#include "light/light_demo_end.h"
#include "light/light_demo_update.h"
#include "lobby/lb_npc_area_light_set.h"
#include "lobby/lb_npc_area_light_clear.h"
#include "lobby/lb_event_request.h"
#include "lobby/lobby_w.h"
#include "Network/network_pat_control.h" /* isServerSelectState */
#include "Pl/plw.h"                  /* _PLW */
#include "Pl/fn_802693C4.h"          /* Get_motion_no */
#include "Pl/pl_now_area_ck.h"          /* pl_now_area_ck */
#include "sound/fn_800D7F54.h"       /* se_req_frame_set, SE_Code_Make */
#include "sound/se_req_frame_set_bank.h"
#include "lobby/se_req_frame_set_parts3.h"
#include "lobby/se_req_frame_set_part4.h"
#include "lobby/lb_pl_se_frame_callback.h"
#include "fn_80047398/lobby_world_block.h"
#include "id_value.h"                /* IdValue */
#include "lobby/lb_list.h"           /* LbList */
#include "lobby/lb_list_init.h"      /* lb_list_init, lb_choice_step, lb_panel_close */
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_page_row_pair_get.h"
#include "lobby/lb_combo_recipe_known_ck.h"
#include "lobby/lb_combo_exec.h"
#include "lobby/lb_npc.h"            /* lb_item_get_data, lb_npc_talk_mode_set */
#include "ef/eft052_box.h"           /* Eft052ItemGetReq */
#include "ef/eft052_item_get_open.h"
#include "ef/eft052.h"               /* eft052_page_count_add, eft052_page_count_ck */
#include "ef/eft052_page_put.h"
#include "ef/eft052_page_take.h"
#include "ef/system_core.h"          /* rand_lcg_step */
#include "hud/cockpit.h"             /* ainpc_page_hold_set */
#include "hud/ainpc_page_hold_clear.h"
#include "menu/menu_item.h"          /* GetItemData, ItemDataRecord */
#include "menu/menu_message.h"       /* menu_cursor_*, menu_page_count, toggle_word_step */
#include "menu/lb_combo_yield_tbl.h"
#include "userdata_item.h"           /* item_slots_free_count, userdata_item_count_total, subTransSet */
#include "fn_80047398.h"             /* userdata_box_capacity, userdata_equip_item_slots_get */
#include "userdata_rank_band_get.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Pl/pl_item_page2_ck.h"
#include "main.h"                    /* ck_WideMode */
#include "lobby/LbStr.h"
#include "lobby/spr_data_copy.h"
#include "lobby/lb_item_cell_draw_layout1.h"
#include "lobby/lb_item_cell_draw_layout11.h"
#include "lobby/lb_text_center_print.h"
#include "lobby/draw_font_idx_num.h"
#include "lobby/lb_item_detail_panel_draw.h"
#include "lobby/lb_item_box_count_draw.h"
#include "menu/menu_item_cell_draw.h"
#include "menu/ItemExp.h"
#include "sound/set_zmode.h"
#include "MSL_C/printf.h"                /* sprintf */
#include "Pl/pl_act_stage_latch_set.h"   /* my_player_work_get */
#include "font/flfnt.h"                  /* font_set_size, font_print_ex, flfntStrLen */
#include "font/font_str_char_count.h"
#include "font/font_print_typed.h"
#include "hud/draw_help_window.h"
#include "hud/draw_spin_icon.h"
#include "hud/draw_window_box.h"
#include "menu/npc_talk_name_tbl.h"
#include "menu/put_frame_dialog_title.h"
#include "lobby/lb_font_print.h"
#include "lobby/lb_list_dialog_draw.h"
#include "sound/se_talk_bank_req.h"
#include "sound/se_talk_npc0_bank_req.h"

/* One fade key of `demo_fade_keys`: demo `demo_no` fades out 0x20 frames before `fade_out` and back in at
 * `fade_in` (half-frames).  size: 0x6 */
typedef struct DemoFadeKey {
    /* +0x0 */ s8 demo_no;
    /* +0x1 */ u8 pad_0x1;
    /* +0x2 */ s16 fade_out;
    /* +0x4 */ s16 fade_in;
} DemoFadeKey; /* size: 0x6 */

/* A repeating frame window: from `start` to `end`, every `step` half-frames.  size: 0x6 */
typedef struct DemoFrameKey {
    /* +0x0 */ s16 start;
    /* +0x2 */ s16 end;
    /* +0x4 */ s16 step;
} DemoFrameKey; /* size: 0x6 */

/* A frame window [start, end).  size: 0x4 */
typedef struct DemoFrameRange {
    /* +0x0 */ s16 start;
    /* +0x2 */ s16 end;
} DemoFrameRange; /* size: 0x4 */

/* The user-data block (`lobby_world_block`) as the counter screen reads it.  size: 0x47DA (a view: the fields
 * past +0x47D9 are not read here) */
typedef struct LbTradeUserData {
    /* +0x0000 */ u8 pad_0x0000[0x18];
    /* +0x0018 */ s32 money;            /* the zenny the combine fee is taken from */
    /* +0x001C */ u8 pad_0x001C[0x144];
    /* +0x0160 */ IdValue supplies[8];  /* the carried supplies */
    /* +0x0180 */ IdValue box[0xF20];   /* the item box */
    /* +0x3E00 */ u8 pad_0x3E00[0x9D6];
    /* +0x47D6 */ u16 trade_reward;     /* the item the trade pays out */
    /* +0x47D8 */ u8 trade_wait;        /* days left until the reward (0: nothing ordered, 0xFF: ready) */
    /* +0x47D9 */ u8 trade_page;        /* the trade page the reward came from */
} LbTradeUserData; /* size: 0x47DA */

/* One combination recipe of `lobby/lb_companion_ui.cpp`'s table, as this unit reads it.  size: 0x8 (GUESS: the
 * bytes past +0x06 are not read here) */
struct LbComboRecipe {
    /* +0x0 */ u16 pad_0x0;
    /* +0x2 */ u16 result;       /* the item the combination makes */
    /* +0x4 */ u8 pad_0x4;
    /* +0x5 */ u8 yield_group;   /* the `lb_combo_yield_tbl` index */
    /* +0x6 */ u8 known_bit;     /* the user-data bit that marks the recipe known */
    /* +0x7 */ u8 pad_0x7;
};

/* One row of the counter's combine list.  size: 0x10 */
typedef struct LbComboRow {
    /* +0x0 */ u16 result;
    /* +0x2 */ u16 item_a;
    /* +0x4 */ u16 item_b;
    /* +0x6 */ u16 yield;   /* items made per combination */
    /* +0x8 */ u16 flags;   /* `lb_screen26_combo_flags_get` for one combination */
    /* +0xA */ u8 pad_0xA[0x2];
    /* +0xC */ s32 price;   /* the fee per combination */
} LbComboRow; /* size: 0x10 */

/* The counter screen's work (screen 0x1A, `lobby_w.screen26_0x0AC`, cleared 0x2000 bytes by `lb_screen26_open`).
 * size: 0x1EC (a view of the 0x2000-byte block) */
typedef struct LbScreen26Work {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 sub_step;
    /* +0x002 */ u8 grid_mode;         /* set while the trade's cursor is on the reward grid */
    /* +0x003 */ u8 pad_0x003;
    /* +0x004 */ s16 blink;            /* the confirmation's 20-frame blink counter */
    /* +0x006 */ u8 pad_0x006[0x2];
    /* +0x008 */ u16 moved;            /* the page step's moved flags (the page arrows) */
    /* +0x00A */ s16 page;
    /* +0x00C */ s16 page_count;
    /* +0x00E */ s16 row_count;        /* the combine list's rows on this page */
    /* +0x010 */ s16 cursor;
    /* +0x012 */ s16 grid_cursor;      /* the reward grid's cursor (0..3) */
    /* +0x014 */ s16 count;            /* the combinations asked for */
    /* +0x016 */ s16 count_max;
    /* +0x018 */ s16 picked;           /* the trade items picked so far (0..3) */
    /* +0x01A */ u8 pad_0x01A[0x2];
    /* +0x01C */ s32 confirm;          /* the yes/no toggle's state word */
    /* +0x020 */ LbList menu;          /* the top menu: 0/1 combine into the pouch/box, 2 trade */
    /* +0x040 */ Eft052ItemGetReq reward;
    /* +0x06C */ u16 rewards[4];       /* the trade page's possible rewards */
    /* +0x074 */ u16 offer_items[5];   /* the five items the trade asks for */
    /* +0x07E */ u8 pad_0x07E[0x2];
    /* +0x080 */ s32 offer_counts[5];  /* how many of each the player holds */
    /* +0x094 */ u16 picks[3];
    /* +0x09A */ u16 pool[100];        /* the trade page's offer list, drawn from without repeats */
    /* +0x162 */ u8 pad_0x162[0x2];
    /* +0x164 */ LbComboRow rows[8];
    /* +0x1E4 */ struct _PLW* player;
    /* +0x1E8 */ struct _LB_NPC* npc;
} LbScreen26Work; /* size: 0x1EC */

/* One trade page tab: its layout anchor, the blink, frame and body sprites, and the page arrows.  size: 0x10 */
typedef struct LbTradeTab {
    /* +0x0 */ u16 lsp;
    /* +0x2 */ u16 blink;
    /* +0x4 */ u16 frame;
    /* +0x6 */ u16 pad_0x6;
    /* +0x8 */ u16* sprites;
    /* +0xC */ u16* arrows;
} LbTradeTab; /* size: 0x10 */

/* One quest NPC talk message: its window kind, the follow-up messages and the text.  size: 0x8 */
typedef struct NpcTalkRec {
    /* +0x0 */ u8 kind;       /* 0 plain, 1 two-answer choice, 2 chained, 3 timed */
    /* +0x1 */ u8 msg_a;      /* the next message (the first answer's), or the timed message's frames */
    /* +0x2 */ u8 msg_b;      /* the second answer's next message */
    /* +0x3 */ u8 pad_0x3;
    /* +0x4 */ char* text;
} NpcTalkRec; /* size: 0x8 */

/* The talk window (`npc_talk_work`, .bss 0x806D2B48).  size: 0x118 */
typedef struct NpcTalkWork {
    /* +0x000 */ char text[0x100];  /* the message as shown (the item name expanded) */
    /* +0x100 */ u8 active;
    /* +0x101 */ u8 choice;
    /* +0x102 */ s8 speaker;        /* -1: none */
    /* +0x103 */ u8 kind;
    /* +0x104 */ u8 msg_a;
    /* +0x105 */ u8 msg_b;
    /* +0x106 */ u16 item;          /* the item named in the text (0: none) */
    /* +0x108 */ u8 index;          /* the message shown */
    /* +0x109 */ s8 wait;           /* frames the input waits */
    /* +0x10A */ u8 flag;           /* a countdown other units set (`npc_talk_flag_set`) */
    /* +0x10B */ u8 area;           /* the area the talk opened in */
    /* +0x10C */ s32 tick;          /* the typewriter's frame counter */
    /* +0x110 */ s32 shown;         /* the characters typed out */
    /* +0x114 */ u8 done;           /* set once the whole text is out */
    /* +0x115 */ u8 pad_0x115[0x3];
} NpcTalkWork; /* size: 0x118 */

/* The server-select screen's work (screen 0x1B, `lobby_w.server_sel_0x0AC`, cleared 0x2000 bytes by
 * `lb_server_select_open`): the network list menu `NetCtrlWk` fills, the top menu and the player.  size: 0x790
 * (a view of the 0x2000-byte block) */
typedef struct LbServerSelWork {
    /* +0x000 */ NetListMenu list;
    /* +0x76C */ LbList menu;
    /* +0x78C */ struct _PLW* player;
} LbServerSelWork; /* size: 0x790 */

/* `Network/network_pat_control.cpp`'s request calls as these call sites see them: the retail calls test the
 * queued command's result (the owner's header spells them `void`). */
#define request_list_refresh_c1 ((s32 (*)(s8*))NetCtrlWk::requestListRefresh)
#define request_command_0f_c1 ((s32 (*)(s8*, s32))NetCtrlWk::requestCommand0F)

/* `menu/menu_message.cpp`'s `toggle_word_step` as these call sites see it: the retail calls narrow the last
 * argument to a halfword (the owner's header spells it `s32`). */
#define toggle_word_step_c1 ((s32 (*)(s32*, u16, s32, s32, u16))toggle_word_step)

/* `Pl/pl_act.cpp`'s `pl_step_se_kind_get` as the call site sees it: the owner returns a byte (its header spells
 * the consumers' 32-bit view). */
#define pl_step_se_kind_get_c1 ((u8 (*)(struct _PLW*))pl_step_se_kind_get)

/* `lobby/fn_80212810.cpp`'s `lb_item_cell_draw_layout1` as these call sites see it: the retail calls pass the
 * count as a full word (the owner spells it `u16`). */
#define lb_item_cell_draw_layout1_c1 ((s32 (*)(u16, s32, s16*, s32, s32, s32, s32))lb_item_cell_draw_layout1)

DemoFadeKey demo_fade_keys[3] = {
    { 13, 0, 1050, 1080 },
    { 32, 0, 1000, 1030 },
    { -1, 0, 0, 0 },
};

DemoFrameKey demo23_break_keys[3] = {
    { 950, 1102, 60 },
    { 1102, 1770, 60 },
    { -1, 0, 0 },
};

/* Demo 23's rock-break positions, three floats per key of `demo23_break_keys` (the retail table runs 0x28 bytes:
 * a zero third position and one more zero word). */
f32 demo23_break_pos[10] = {
    -3400.0f, -1700.0f, -6800.0f,
    -3400.0f, -1300.0f, -6800.0f,
    0.0f, 0.0f, 0.0f,
    0.0f,
};


DemoDef demo_def_15E = { 0, 0, 0x15E, 1 };
DemoDef demo_def_15F = { 0, 0, 0x15F, 2 };
DemoDef demo_def_160 = { 2, 0, 0x160, 3 };
DemoDef demo_def_161 = { 2, 0, 0x161, 4 };
DemoDef demo_def_162 = { 2, 0, 0x162, 4 };
DemoDef demo_def_163 = { 2, 0, 0x163, 5 };
DemoDef demo_def_164 = { 2, 0, 0x164, 5 };
DemoDef demo_def_165 = { 2, 0, 0x165, 6 };
DemoDef demo_def_166 = { 2, 0, 0x166, 6 };
DemoDef demo_def_167 = { 2, 0, 0x167, 6 };
DemoDef demo_def_168 = { 2, 0, 0x168, 7 };
DemoDef demo_def_169 = { 2, 0, 0x169, 7 };
DemoDef demo_def_16A = { 2, 0, 0x16A, 7 };
DemoDef demo_def_16B = { 0, 0, 0x16B, 8 };
DemoDef demo_def_16C = { 0, 0, 0x16C, 9 };
DemoDef demo_def_16D = { 0, 0, 0x16D, 10 };
DemoDef demo_def_16E = { 0, 0, 0x16E, 11 };
DemoDef demo_def_16F = { 0, 0, 0x16F, 12 };
DemoDef demo_def_170 = { 0, 0, 0x170, 13 };
DemoDef demo_def_171 = { 1, 0, 0x171, 14 };
DemoDef demo_def_172 = { 0, 0, 0x172, 15 };
DemoDef demo_def_173 = { 0, 0, 0x173, 16 };
DemoDef demo_def_174 = { 0, 0, 0x174, 17 };
DemoDef demo_def_175 = { 0, 0, 0x175, 18 };
DemoDef demo_def_176 = { 9, 0, 0x176, 19 };
DemoDef demo_def_177 = { 7, 0, 0x177, 20 };
DemoDef demo_def_178 = { 7, 0, 0x178, 21 };
DemoDef demo_def_179 = { 5, 0, 0x179, 22 };
DemoDef demo_def_17A = { 12, 0, 0x17A, 23 };
DemoDef demo_def_17B = { 12, 0, 0x17B, 24 };
DemoDef demo_def_17C = { 9, 0, 0x17C, 25 };
DemoDef demo_def_17D = { 9, 0, 0x17D, 26 };
DemoDef demo_def_17E = { 6, 0, 0x17E, 27 };
DemoDef demo_def_17F = { 3, 0, 0x17F, 28 };
DemoDef demo_def_180 = { 3, 0, 0x180, 29 };
DemoDef demo_def_181 = { 8, 0, 0x181, 30 };
DemoDef demo_def_182 = { 2, 0, 0x182, 31 };
DemoDef demo_def_183 = { 2, 0, 0x183, 32 };
DemoDef demo_def_184 = { 6, 0, 0x184, 33 };
DemoDef demo_def_185 = { 8, 0, 0x185, 34 };
DemoDef demo_def_186 = { 3, 0, 0x186, 35 };
DemoDef demo_def_187 = { 7, 0, 0x187, 36 };
DemoDef demo_def_188 = { 5, 0, 0x188, 37 };
DemoDef demo_def_189 = { 7, 0, 0x189, 38 };
DemoDef demo_def_18B = { 0, 0, 0x18B, 43 };
DemoDef demo_def_18C = { 3, 0, 0x18C, 44 };
DemoDef demo_def_18D = { 1, 0, 0x18D, 47 };
DemoFrameRange demo27_stage_frames = { 868, 1108 };






/* The trade tables are defined after the talk window (the retail `.data` order). */
extern u16* lb_trade_reward_tbl[3];
extern u16* lb_trade_offer_tbl[3];

/* The demo/event work block (.bss 0x806D2B20) the functions below read and write. */
DemoWork demo_work;

/* The talk window (.bss 0x806D2B48). */
NpcTalkWork npc_talk_work;

/* The quest NPCs' talk tables, one per speaker (.sbss 0x80794C90; `get_qnpc_talk_data_ptr` hands its address out). */
NpcTalkRec** qnpc_talk_data;

extern "C" {

s32 demo_slot_set(DemoSlot* slot, const DemoDef* def);
s32 demo_map_slot_set(DemoWork* work);
s32 demo_slot_ready_ck(DemoSlot* slot);
s32 demo_slot_play(DemoSlot* slot);
void demo_fade_update(u8 demo);
void demo_effect_update(u8 demo);

void userdata_server_rows_reset(void);
void lb_screen26_open(struct _PLW* player, struct _LB_NPC* npc);
void lb_screen26_close(void);
void lb_screen26_step(void);
u8 lb_trade_state_get(void);
void lb_screen26_trade_build(LbScreen26Work* work, s16 page);
void lb_screen26_trade_open(LbScreen26Work* work);
void lb_screen26_pick_reset(LbScreen26Work* work);
void lb_screen26_pick_add(LbScreen26Work* work);
void lb_screen26_pick_remove(LbScreen26Work* work);
s32 lb_screen26_pick_step(LbScreen26Work* work);
void lb_screen26_trade_commit(LbScreen26Work* work);
s32 lb_screen26_trade_step(LbScreen26Work* work);
u16 lb_screen26_combo_flags_get(LbScreen26Work* work, LbComboRow* row, s32 count, s16 mode);
void lb_screen26_combo_build(LbScreen26Work* work, s16 page);
void lb_screen26_combo_open(LbScreen26Work* work);
void lb_screen26_combo_exec(LbScreen26Work* work);
s32 lb_screen26_combo_step(LbScreen26Work* work);
void lb_screen26_bg_draw(LbScreen26Work* work);
void lb_screen26_menu_draw(LbScreen26Work* work);
void lb_screen26_combo_list_draw(LbScreen26Work* work, u32 active);
void lb_screen26_combo_items_draw(LbScreen26Work* work);
void lb_screen26_combo_info_draw(LbScreen26Work* work);
void lb_screen26_combo_draw(LbScreen26Work* work);
void lb_screen26_trade_tabs_draw(LbScreen26Work* work, u32 active, u8 arrows);
void lb_screen26_trade_list_draw(LbScreen26Work* work, bool active, u8 locked);
void lb_screen26_picks_draw(LbScreen26Work* work, u32 active);
void lb_screen26_grid_help_draw(LbScreen26Work* work);
void lb_screen26_trade_draw(LbScreen26Work* work);
void lb_screen26_status_draw(void);
void lb_screen26_box_cell_pos_get(s32 index, _mh_ivec2_* out);
void lb_screen26_box_draw(Eft052ItemBox* box);
void lb_screen26_reward_draw(void);
void lb_screen26_help_draw(LbScreen26Work* work);
void npc_talk_reset(void);
void npc_talk_draw(void);
u8 npc_talk_flag_get(void);
void npc_talk_text_expand(u8* dst, const s8* src, u16 item);
u8 npc_talk_input_step(NpcTalkWork* work);
void lb_server_select_open(struct _PLW* player);
void lb_server_select_close(void);
void lb_server_select_screen_step(void);
void lb_server_select_rooms_open(LbServerSelWork* work);
void lb_server_select_rooms_build(LbServerSelWork* work, s16 page);
void lb_server_select_type_info_draw(LbServerSelWork* work, s16 index);
void lb_server_select_types_draw(LbServerSelWork* work, bool active);
void lb_server_select_title_draw(u16 str);
void lb_server_select_help_draw(LbServerSelWork* work);
void lb_server_select_servers_draw(LbServerSelWork* work, bool active);
void lb_server_select_rooms_draw(LbServerSelWork* work, bool active);
void lb_server_select_draw(void);
void lb_screen26_draw(void);

/* 0x803C3A5C (0x14): clears the demo work. */
void demo_work_clear(void)
{
    memset(&demo_work, 0, sizeof(DemoWork));
}

/* 0x803C3A70 (0x304): resets the demo work and fills its slots with the demos of the current map and quest. */
void demo_work_start(void)
{
    DemoWork* work = &demo_work;

    get_move_work_adrs(0);
    demo_work_clear();
    work->demo_no_0x02 = 0xFF;
    if (GameMode_ck() == 2) {
        return;
    }
    demo_map_slot_set(work);
    switch (stage_map_kind_get(get_now_mapno())) {
    case 10:
        demo_slot_set(&work->slots[0], &demo_def_18D);
        break;
    }
    switch ((u16)quest_id_get()) {
    case 1:
        demo_slot_set(&work->slots[0], &demo_def_15E);
        demo_slot_set(&work->slots[1], &demo_def_171);
        break;
    case 1000:
        demo_slot_set(&work->slots[0], &demo_def_15F);
        break;
    case 2:
        demo_slot_set(&work->slots[0], &demo_def_176);
        break;
    case 4:
        demo_slot_set(&work->slots[0], &demo_def_177);
        break;
    case 1004:
        demo_slot_set(&work->slots[0], &demo_def_178);
        break;
    case 2003:
        demo_slot_set(&work->slots[0], &demo_def_179);
        break;
    case 2004:
        demo_slot_set(&work->slots[0], &demo_def_17A);
        break;
    case 2000:
        demo_slot_set(&work->slots[0], &demo_def_17B);
        break;
    case 1013:
        demo_slot_set(&work->slots[0], &demo_def_17C);
        break;
    case 2001:
        demo_slot_set(&work->slots[0], &demo_def_17D);
        break;
    case 1024:
        demo_slot_set(&work->slots[0], &demo_def_17E);
        break;
    case 1025:
        break;
    case 1026:
        demo_slot_set(&work->slots[0], &demo_def_17F);
        break;
    case 1027:
        demo_slot_set(&work->slots[0], &demo_def_180);
        break;
    case 2002:
        demo_slot_set(&work->slots[0], &demo_def_181);
        break;
    case 1031:
        demo_slot_set(&work->slots[0], &demo_def_182);
        break;
    case 1036:
        demo_slot_set(&work->slots[0], &demo_def_183);
        break;
    case 1037:
        demo_slot_set(&work->slots[0], &demo_def_184);
        break;
    case 1044:
        demo_slot_set(&work->slots[0], &demo_def_185);
        break;
    case 1045:
        demo_slot_set(&work->slots[0], &demo_def_186);
        break;
    case 1046:
        demo_slot_set(&work->slots[0], &demo_def_187);
        break;
    case 1047:
        break;
    case 1049:
        demo_slot_set(&work->slots[0], &demo_def_188);
        break;
    case 1048:
        demo_slot_set(&work->slots[0], &demo_def_189);
        break;
    case 2005:
    case 2006:
        demo_slot_set(&work->slots[0], &demo_def_18B);
        demo_slot_set(&work->slots[1], &demo_def_18C);
        break;
    }
}

/* 0x803C3D74 (0x84): restarts the demo work and adds demo `demo`'s record (14, 19 or 20) to the first slot. */
void demo_work_start_ex(s32 demo)
{
    DemoWork* work = &demo_work;

    demo_work_start();
    switch (demo) {
    case 14:
        demo_slot_set(&work->slots[0], &demo_def_171);
        break;
    case 19:
        demo_slot_set(&work->slots[0], &demo_def_176);
        break;
    case 20:
        demo_slot_set(&work->slots[0], &demo_def_177);
        break;
    }
}

/* 0x803C3DF8 (0x24): true for the five map ids 394..398. */
u32 map_id_ck(u16 map)
{
    return (u32)(map - 394) <= 3;
}

/* 0x803C3E1C (0xB4): puts `def` into `slot` unless it was already seen or its camera keys are not loaded; -1 then. */
s32 demo_slot_set(DemoSlot* slot, const DemoDef* def)
{
    if (def->progress_flag != 0 && demo_play_ck() == 0 && map_id_ck(def->progress_flag) == 0 &&
        userdata_progress_flag_ck(def->progress_flag) == 1) {
        return -1;
    }
    if (stage_dcm_buffer_tbl[def->demo_no] == NULL) {
        return -1;
    }
    slot->active = 1;
    slot->def = def;
    emc_event_set(slot->def->demo_no, slot->def->area);
    return 0;
}

/* 0x803C3ED0 (0x90): queues `def` into `slot` once per pass; a demo already seen is skipped, and a second one in the
 * same pass is only marked seen. */
s32 demo_slot_queue(DemoWork* work, DemoSlot* slot, const DemoDef* def)
{
    if (def->progress_flag != 0) {
        if (userdata_progress_flag_ck(def->progress_flag) == 1) {
            return -1;
        }
        if (work->queued == 1) {
            userdata_progress_flag_set(def->progress_flag);
            return -1;
        }
    }
    slot->active = 1;
    slot->def = def;
    work->queued = 1;
    return 0;
}

/* 0x803C3F60 (0xC): runs `demo_work_process` on the demo work. */
void demo_work_update(void)
{
    demo_work_process(&demo_work);
}

/* 0x803C3F6C (0x1FC): the demo work's state machine - picks a ready slot, loads and starts its BGM, plays it. */
void demo_sound_update(void)
{
    DemoWork* work = &demo_work;
    DemoSlot* slot;
    s32 i;

    event_demo_ck();
    switch (demo_work.state_0x00) {
    case 0:
        work->state_0x00++;
        break;
    case 1:
        slot = &work->slots[0];
        for (i = 0; i < 3; i++, slot++) {
            if (slot->active != 0 && stage_dcm_buffer_tbl[slot->def->demo_no] != NULL &&
                demo_slot_ready_ck(slot) > 0) {
                set_str_bgm_main_vol(0.0f);
                set_str_se_main_vol(0.0f);
                work->state_0x00 = 2;
                work->slot = i;
                work->demo_no_0x02 = slot->def->demo_no;
                fade_set(0, 2);
                work->bgm_status = ((s8 (*)(u8))demo_bgm_load)(work->demo_no_0x02);
                break;
            }
        }
        break;
    case 2:
        fade_set(0, 2);
        if (work->bgm_status == 0) {
            if (demo_bgm_ready_ck() != 0) {
                demo_stream_play(work->demo_no_0x02);
                work->state_0x00 = 3;
            }
        } else {
            work->state_0x00 = 3;
        }
        break;
    case 3:
        fade_set(0, 2);
        if (work->bgm_status == 0) {
            if (demo_stream_stop_ck(work->demo_no_0x02) == 0) {
                break;
            }
            demo_stream_stop(work->demo_no_0x02);
        }
        set_BGM_volume(get_option_cfg(17));
        set_SE_volume(get_option_cfg(18));
        work->state_0x00 = 4;
    case 4:
        if (demo_slot_play(&work->slots[work->slot]) != 0) {
            work->state_0x00 = 1;
            stage_bgm_reload();
            root_wait_frames_set(30);
        }
        break;
    }
}

/* 0x803C4168 (0xA8): puts the current map kind's demo into the third slot (outside server select); returns 0. */
s32 demo_map_slot_set(DemoWork* work)
{
    if (isServerSelectState() != 1) {
        switch (stage_map_kind_get(get_now_mapno())) {
        case 2:
            demo_slot_set(&work->slots[2], &demo_def_172);
            break;
        case 3:
            demo_slot_set(&work->slots[2], &demo_def_173);
            break;
        case 4:
            demo_slot_set(&work->slots[2], &demo_def_174);
            break;
        case 5:
            demo_slot_set(&work->slots[2], &demo_def_175);
            break;
        }
    }
    return 0;
}

/* 0x803C4210 (0x2E0): queues the demos of map 22's areas 0 and 2 whose progress flags are set. */
void demo_work_process(DemoWork* work)
{
    work->queued = 0;
    if (get_now_mapno() != 22) {
        return;
    }
    switch (get_now_areano()) {
    case 0:
        if (userdata_progress_flag_ck(40) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_170);
        }
        if (userdata_progress_flag_ck(38) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_16D);
        }
        if (userdata_progress_flag_ck(33) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_16C);
        }
        if (userdata_progress_flag_ck(31) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_16F);
        }
        if (userdata_progress_flag_ck(28) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_16E);
        }
        if (userdata_progress_flag_ck(26) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_16B);
        }
        break;
    case 2:
        if (userdata_progress_flag_ck(25) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_160);
        }
        if (userdata_progress_flag_ck(86) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_167) == 0) {
            work->variant = 2;
        }
        if (userdata_progress_flag_ck(83) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_16A) == 0) {
            work->variant = 2;
        }
        if (userdata_progress_flag_ck(80) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_164) == 0) {
            work->variant = 1;
        }
        if (userdata_progress_flag_ck(78) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_162) == 0) {
            work->variant = 1;
        }
        if (userdata_progress_flag_ck(85) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_166) == 0) {
            work->variant = 1;
        }
        if (userdata_progress_flag_ck(82) == 1 && demo_slot_queue(work, &work->slots[0], &demo_def_169) == 0) {
            work->variant = 1;
        }
        if (userdata_progress_flag_ck(79) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_163);
        }
        if (userdata_progress_flag_ck(77) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_161);
        }
        if (userdata_progress_flag_ck(84) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_165);
        }
        if (userdata_progress_flag_ck(81) == 1) {
            demo_slot_queue(work, &work->slots[0], &demo_def_168);
        }
        break;
    }
}

/* 0x803C44F0 (0x170): 1 when the slot's demo may start now - the player is in its area, the lobby is not on hold, and
 * its demo-specific trigger holds. */
s32 demo_slot_ready_ck(DemoSlot* slot)
{
    DemoWork* work = &demo_work;
    _ENEMY_WORK* em = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 count = get_move_work_max(3);
    u16 i;
    s32 found;

    if (slot->def->area != get_now_areano()) {
        return 0;
    }
    if (GameMode_ck() == 2 && lobby_w.field_0x12C == 1) {
        return 0;
    }
    if (slot->def->demo_no == 14) {
        if (demo_play_ck() == 0) {
            return work->running == 1;
        }
        return 1;
    } else if (slot->def->demo_no == 44) {
        if (Pl_motion_input_ck(0) == 1) {
            return 0;
        }
        found = 0;
        for (i = 0; i < (u16)count; i++, em++) {
            if (em->active != 0 && em->team == 20 && em->area_no == slot->def->area) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x803C4660 (0x1B4): plays the slot's demo - start, per-frame update until the camera is done, end; 1 once done. */
s32 demo_slot_play(DemoSlot* slot)
{
    DemoWork* work = &demo_work;

    switch (slot->step) {
    case 0:
        camera_lobby_mode_offset_req(work->demo_no_0x02, NULL);
        if (GameMode_ck() == 1) {
            emc_event_hold_release(work->demo_no_0x02);
            emc_event_act_request(work->demo_no_0x02);
            ainpc_demo_start(work->demo_no_0x02);
            light_demo_start(work->demo_no_0x02);
            stage_demo_start(work->demo_no_0x02);
        } else if (PlayMode_ck() == 5) {
            lb_npc_area_light_set(work->demo_no_0x02);
        }
        work->field_0x04 = 0;
        slot->step++;
        break;
    case 1:
        work->field_0x04++;
        light_demo_update(work->demo_no_0x02);
        demo_fade_update(work->demo_no_0x02);
        demo_effect_update(work->demo_no_0x02);
        if (camera_work_ck() == 0) {
            slot->step++;
            if (demo_play_ck() == 0) {
                userdata_progress_flag_set(slot->def->progress_flag);
                unlock_bit_raise(work->demo_no_0x02);
                fade_set(0, 2);
            }
            if (GameMode_ck() == 1) {
                emc_event_act_end_request();
                ainpc_demo_end();
                light_demo_end();
                if (work->demo_no_0x02 == 44) {
                    lb_event_request(28);
                } else {
                    lb_event_request(14);
                }
            } else if (PlayMode_ck() == 5) {
                lb_npc_area_light_clear();
            }
        }
        break;
    case 2:
        slot->step = 0;
        slot->active = 0;
        work->field_0x04 = 0;
        work->demo_no_0x02 = 0xFF;
        return 1;
    }
    return 0;
}

} /* extern "C" */

/* 0x803C4814 (0x18): non-zero while the demo work is playing the demo the game asks about. */
u32 event_demo_ck(void)
{
    return demo_work.state_0x00 == 4;
}

extern "C" {

/* 0x803C482C (0x14): non-zero while the demo work is past its waiting state. */
u32 event_demo_running_ck(void)
{
    return demo_work.state_0x00 > 1;
}

/* 0x803C4840 (0x10): the demo frame. */
u32 get_demo_data(void)
{
    return demo_work.field_0x04;
}

} /* extern "C" */

/* 0x803C4850 (0x10): the demo id the demo work is playing, 0xFF when none. */
u8 get_demo_no(void)
{
    return demo_work.demo_no_0x02;
}

extern "C" {

/* 0x803C4860 (0x50): non-zero while any of the demo work's three slots holds a demo. */
u32 demo_flag_ck(void)
{
    DemoSlot* slot = demo_work.slots;
    s32 i;

    for (i = 0; i < 3; i++, slot++) {
        if (slot->active != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803C48B0 (0xE8): fades the screen around demo `demo`'s cuts and before its camera ends. */
void demo_fade_update(u8 demo)
{
    s16 frame;
    DemoFadeKey* key;

    if (demo_play_ck() == 1 && gallery_demo_running_ck() == 1) {
        return;
    }
    frame = get_demo_data() * 2;
    for (key = demo_fade_keys; key->demo_no > 0; key++) {
        if (demo == key->demo_no) {
            if (frame == key->fade_out - 0x20) {
                fade_set(0, 1);
            } else if (frame == key->fade_in) {
                fade_set(0, 2);
            }
        }
    }
    if (frame == camera_event_length_get() * 2 - 0x20) {
        fade_set(0, 1);
    }
}

/* 0x803C4998 (0x108): the demo-timed effects - demo 23's breaking rocks, demo 27's stage flags. */
void demo_effect_update(u8 demo)
{
    nw4r::math::VEC3 pos;
    s16 frame;
    DemoFrameKey* key;
    nw4r::math::VEC3* at;
    DemoFrameRange* range;

    VEC3_ctor(&pos);
    frame = get_demo_data() * 2;
    switch (demo) {
    case 23:
        key = demo23_break_keys;
        at = (nw4r::math::VEC3*)demo23_break_pos;
        for (; key->start > 0; key++, at++) {
            if (frame >= key->start && frame < key->end && (frame - key->start) % key->step == 0) {
                setVector3(&pos, at->x, at->y, at->z);
                eft028_set_koware(1, &pos, 12, 0);
            }
        }
        break;
    case 27:
        range = &demo27_stage_frames;
        if (frame >= range->start && frame < range->end) {
            stage_demo_flags_set();
        }
        break;
    }
}

/* 0x803C4AA0 (0x14): marks the demo work running. */
void demo_set_running(void)
{
    demo_work.running = 1;
}

/* 0x803C4AB4 (0x60): 1 while the playing demo is one of the server-leaving ones (39, 43, 44, 47) or none. */
u32 lb_server_leave_ck(void)
{
    switch (get_demo_no()) {
    case 39:
    case 43:
    case 44:
    case 47:
        return 1;
    case 0xFF:
        return 1;
    }
    return 0;
}

/* 0x803C4B14 (0x60): 1 in the server-select state while demo 47 or none plays. */
u32 lb_server_ready_ck(void)
{
    if (isServerSelectState() == 0) {
        return 0;
    }
    switch (get_demo_no()) {
    case 47:
        return 1;
    case 0xFF:
        return 1;
    }
    return 0;
}

/* 0x803C4B74 (0x2C): initialises the demo work for a fresh play-through. */
void demo_work_init(void)
{
    DemoSlot* slot = &demo_work.slots[0];

    slot->step = 0;
    slot->active = 0;
    demo_work.state_0x00 = 1;
    demo_work.field_0x04 = 0;
    demo_work.demo_no_0x02 = 0xFF;
}

/* 0x803C4BA0 (0xD34): the lobby player's SE frame callback - per motion, the body, voice and step sounds keyed to
 * motion frames. */
void lb_pl_se_frame_callback(struct _PLW* plw, s32 event)
{
    struct _se_w* se_body;
    struct _se_w* se_voice;
    struct _se_w* se_bank;
    u8 kind;

    if (event_demo_ck() != 1 && plw->field_0x001 != 0 && (PlayMode_ck() != 3 || pl_now_area_ck(plw) != 0)) {
        se_body = plw->field_0xAF4;
        se_bank = plw->field_0xAF8;
        se_voice = plw->field_0xAFC;
        kind = pl_step_se_kind_get_c1(plw);
        if (kind == 0) {
            kind = plw->field_0x5A7;
        }
        switch (Get_motion_no(plw)) {
        case 0x25A:
            se_req_frame_set(se_voice, 0x30, 8, 7, 3);
            se_req_frame_set(se_voice, 0x3E, 8, 7, 3);
            break;
        case 0x260:
            se_req_frame_set(se_body, 0x44, 0x1C, 0xE, 0x3000003);
            se_req_frame_set_bank(se_bank, 0x22, (kind << 24), 0x14, 2);
            se_req_frame_set_bank(se_bank, 0x36, (kind << 24) | 1, 0x11, 2);
            se_req_frame_set_parts3(se_body, 0x22, 3);
            break;
        case 0x262:
            se_req_frame_set_bank(se_bank, 0xE, (kind << 24) | 1, 0x11, 2);
            se_req_frame_set_bank(se_bank, 0x40, (kind << 24), 0x14, 2);
            se_req_frame_set_parts3(se_body, 0x40, 3);
            break;
        case 0x263:
            se_req_frame_set(se_voice, 0x2A, 5, 0xC, 3);
            se_req_frame_set(se_voice, 0x40, 5, 0xC, 3);
            se_req_frame_set(se_voice, 0x6C, 7, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x6C, 0xB, 3);
            break;
        case 0x264:
            se_req_frame_set(se_body, 0x2A, 0x27, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x7C, 0x2E, 0xB, 0x3000003);
            se_req_frame_set(se_voice, 0x38, 6, 0xB, 3);
            se_req_frame_set(se_voice, 0x80, 7, 7, 3);
            se_req_frame_set_part4(se_body, 0x30, 0xB, 3);
            break;
        case 0x265:
            se_req_frame_set(se_body, 0x48, 0xD, 0xC, 0x3000003);
            se_req_frame_set(se_voice, 0x84, 0xF, 0xC, 3);
            se_req_frame_set(se_voice, 0xCE, 0x10, 0xC, 3);
            se_req_frame_set_part4(se_body, 0x4C, 7, 3);
            break;
        case 0x268:
            se_req_frame_set(se_body, 0x20, 0x12, 0xE, 0x3000003);
            se_req_frame_set_part4(se_body, 0x52, 0xB, 3);
            se_req_frame_set_parts3(se_body, 0x66, 3);
            se_req_frame_set(se_body, 0x4A, 0xD, 0xE, 0x3000003);
            se_req_frame_set(se_voice, 0x7E, 0xF, 0xC, 3);
            se_req_frame_set(se_voice, 0xF6, 0x10, 0xC, 3);
            break;
        case 0x26A:
            se_req_frame_set(se_voice, 0x26, 7, 0xB, 3);
            se_req_frame_set(se_body, 0x7E, 0xD, 0x14, 0x3000003);
            se_req_frame_set(se_body, 0x7E, 0x27, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0xBE, 0x27, 0xB, 0x3000003);
            se_req_frame_set_bank(se_bank, 0x44, ((kind << 24) | 1), 0x11, 2);
            se_req_frame_set_bank(se_bank, 0x5E, ((kind << 24) | 1), 0x14, 2);
            se_req_frame_set_parts3(se_body, 0x60, 3);
            se_req_frame_set_part4(se_body, 0x2A, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x84, 0xB, 3);
            se_req_frame_set_part4(se_body, 0xC0, 0xB, 3);
            break;
        case 0x26E:
            se_req_frame_set_bank(se_bank, 0x1A, (kind << 24) | 1, 0x14, 2);
            se_req_frame_set_parts3(se_body, 0x1C, 3);
            se_req_frame_set(se_voice, 0x44, 9, 0xB, 3);
            se_req_frame_set(se_voice, 0x8C, 0xA, 0xC, 3);
            se_req_frame_set(se_voice, 0xCC, 0xB, 0xC, 3);
            se_req_frame_set(se_voice, 0x130, 7, 0xB, 3);
            break;
        case 0x270:
            se_req_frame_set(se_body, 0x2C, 0x10, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x3C, 0x10, 0xB, 0x3000003);
            se_req_frame_set(se_voice, 0x5A, 9, 0xB, 3);
            se_req_frame_set_bank(se_bank, 0x1E, (kind << 24), 0x11, 2);
            se_req_frame_set_bank(se_bank, 0x92, (kind << 24) | 1, 0x11, 2);
            se_req_frame_set_parts3(se_body, 0x92, 3);
            se_req_frame_set_part4(se_body, 0x2C, 7, 3);
            se_req_frame_set_part4(se_body, 0x3C, 7, 3);
            se_req_frame_set_part4(se_body, 0x50, 7, 3);
            break;
        case 0x271:
            se_req_frame_set(se_voice, 0x74, 7, 0xB, 3);
            se_req_frame_set_bank(se_bank, 0x56, (kind << 24), 0x11, 2);
            se_req_frame_set_bank(se_bank, 0x62, (kind << 24) | 1, 0x11, 2);
            se_req_frame_set_parts3(se_body, 0x56, 3);
            se_req_frame_set_parts3(se_body, 0x62, 3);
            se_req_frame_set_part4(se_body, 0xE, 0xB, 3);
            break;
        case 0x272:
            se_req_frame_set(se_voice, 0x18, 7, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x18, 0xB, 3);
            break;
        case 0x276:
            se_req_frame_set(se_body, 0xA0, SE_Code_Make(1, 4, 0, 4), 0xC, 3);
            se_req_frame_set_part4(se_body, 0x24, 0xB, 3);
            se_req_frame_set_part4(se_body, 0xA0, 0xB, 3);
            se_req_frame_set_parts3(se_body, 0x26, 3);
            se_req_frame_set_bank(se_bank, 0x24, (kind << 24), 0x14, 2);
            se_req_frame_set_bank(se_bank, 0xA0, (kind << 24), 0x14, 2);
            se_req_frame_set(se_body, 0x1C, 0x1C, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x50, 0x27, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x80, 0x27, 0xB, 0x3000003);
            se_req_frame_set(se_voice, 0x9E, 7, 0xB, 3);
            break;
        case 0x277:
            se_req_frame_set(se_body, 0xBC, SE_Code_Make(3, 4, 1, 4), 0xC, 3);
            se_req_frame_set(se_body, 0xFA, SE_Code_Make(7, 4, 8, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x128, SE_Code_Make(7, 4, 8, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x154, SE_Code_Make(7, 4, 8, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x232, SE_Code_Make(3, 4, 1, 4), 0xC, 3);
            se_req_frame_set_bank(se_bank, 0x14, (kind << 24), 0x14, 2);
            se_req_frame_set_parts3(se_body, 0x16, 3);
            se_req_frame_set_bank(se_bank, 0xB0, (kind << 24) | 1, 0x14, 2);
            se_req_frame_set_parts3(se_body, 0xB2, 3);
            se_req_frame_set(se_body, 0xBA, 0x10, 0x11, 0x3000003);
            se_req_frame_set(se_body, 0x14E, 0x10, 0x14, 0x3000003);
            se_req_frame_set(se_body, 0x230, 0x10, 0x14, 0x3000003);
            se_req_frame_set(se_voice, 0x1EA, 7, 7, 3);
            se_req_frame_set(se_voice, 0x1FA, 7, 7, 3);
            se_req_frame_set(se_voice, 0x206, 7, 7, 3);
            se_req_frame_set_part4(se_body, 0xCE, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x104, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x12C, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x22E, 0xB, 3);
            se_req_frame_set(se_body, 0x34, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0xC0, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x106, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x130, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x1BE, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x232, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0xDA, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x118, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x140, 3, 0xB, 0x3000003);
            se_req_frame_set(se_body, 0x258, 3, 0xB, 0x3000003);
            break;
        case 0x278:
            se_req_frame_set(se_body, 0xA, SE_Code_Make(3, 4, 1, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x4C, SE_Code_Make(7, 4, 8, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x7A, SE_Code_Make(7, 4, 7, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x10C, SE_Code_Make(4, 4, 5, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x160, SE_Code_Make(3, 4, 1, 4), 0xC, 3);
            se_req_frame_set(se_body, 0x1DA, SE_Code_Make(7, 4, 8, 4), 0xC, 3);
            se_req_frame_set(se_body, 0xBA, 0x10, 0x14, 0x3000003);
            se_req_frame_set_bank(se_bank, 0x144, (kind << 24), 0x11, 2);
            se_req_frame_set_parts3(se_body, 0x146, 3);
            se_req_frame_set_parts3(se_body, 0x246, 3);
            se_req_frame_set_part4(se_body, 0x26, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x17A, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x244, 0xB, 3);
            se_req_frame_set(se_voice, 0x130, 7, 7, 3);
            se_req_frame_set(se_voice, 0x140, 7, 7, 3);
            break;
        case 0x279:
            se_req_frame_set(se_body, 4, SE_Code_Make(3, 4, 2, 4), 0xC, 3);
            se_req_frame_set(se_body, 8, 0xE, 0xB, 0x3000003);
            se_req_frame_set(se_voice, 8, 7, 0xB, 3);
            se_req_frame_set(se_body, 0x54, 0x1C, 0xB, 0x3000003);
            se_req_frame_set_part4(se_body, 0xC, 0xB, 3);
            se_req_frame_set_part4(se_body, 0x54, 0xB, 3);
            se_req_frame_set_bank(se_bank, 0xB0, (kind << 24) | 2, 0x14, 2);
            se_req_frame_set_parts3(se_body, 0xB2, 3);
            break;
        }
    }
}


/* The combine fee by item rarity. */
s32 lb_combo_price_tbl[8] = { 5, 5, 5, 10, 15, 1000, 1000, 0 };

/* The trade reward's odds in percent, by reward slot. */
const u16 lb_trade_reward_odds[4] = { 40, 35, 20, 5 };

/* The counter screen's sprite runs (0xFFFF-terminated layout ids) and row anchors. */
u16 lb_s26_bg_sprites[20] = {
    0x24C4, 0x24C5, 0x24C6, 0x24C7, 0x24C1, 0x24C2, 0x24C3, 0x24C8, 0x24C9, 0x24CA,
    0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0x24D0, 0x24D1, 0x24BF, 0x24C0, 0xFFFF,
};
u16 lb_s26_frame_sprites[6] = { 0x24D5, 0x24D6, 0x24D3, 0x24D4, 0xFFFF, 0x0000 };
u16 lb_s26_frame_wide_sprites[6] = { 0x24DA, 0x24DB, 0x24D8, 0x24D9, 0xFFFF, 0x0000 };
u16 lb_s26_menu_sprites[12] = {
    0x24E7, 0x24E8, 0x24E9, 0x24E3, 0x24E4, 0x24E5, 0x24E6, 0x24DF, 0x24E0, 0x24E1, 0x24E2, 0xFFFF,
};
u16 lb_s26_combo_list_sprites[12] = {
    0x250F, 0x2510, 0x2511, 0x250D, 0x250E, 0x2508, 0x2509, 0x250A, 0x250B, 0x250C, 0xFFFF, 0x0000,
};
u16 lb_s26_combo_row_lsp[8] = { 0x2513, 0x2514, 0x2515, 0x2516, 0x2517, 0x2518, 0x2519, 0x251A };
u16 lb_s26_combo_info_sprites[34] = {
    0x253B, 0x253D, 0x253E, 0x253C, 0x253A, 0x2521, 0x2522, 0x2523, 0x2524, 0x2525, 0x2526, 0x2527,
    0x2528, 0x2529, 0x252A, 0x252B, 0x252C, 0x252D, 0x252E, 0x252F, 0x2530, 0x2531, 0x2532, 0x2533,
    0x2534, 0x2535, 0x2536, 0x2537, 0x2538, 0x2539, 0x251C, 0x251D, 0xFFFF, 0x0000,
};
u16 lb_s26_tab0_sprites[10] = { 0x2575, 0x2576, 0x2577, 0x2578, 0x2579, 0x257A, 0x257B, 0x257C, 0xFFFF, 0x0000 };
u16 lb_s26_tab0_arrows[6] = { 0x2572, 0x2574, 0x2571, 0x2573, 0xFFFF, 0x0000 };
u16 lb_s26_tab1_sprites[10] = { 0x2584, 0x2585, 0x2586, 0x2587, 0x2588, 0x2589, 0x258A, 0x258B, 0x258C, 0xFFFF };
u16 lb_s26_tab1_arrows[6] = { 0x2581, 0x2583, 0x2580, 0x2582, 0xFFFF, 0x0000 };
u16 lb_s26_tab2_sprites[10] = { 0x2594, 0x2595, 0x2596, 0x2597, 0x2598, 0x2599, 0x259A, 0x259B, 0xFFFF, 0x0000 };
u16 lb_s26_tab2_arrows[8] = { 0x2591, 0x2593, 0x2590, 0x2592, 0xFFFF, 0x0000, 0x0000, 0x0000 };

/* The trade page tabs. */
LbTradeTab lb_s26_trade_tabs[3] = {
    { 0x256E, 0x2570, 0x256F, 0, lb_s26_tab0_sprites, lb_s26_tab0_arrows },
    { 0x257D, 0x257F, 0x257E, 0, lb_s26_tab1_sprites, lb_s26_tab1_arrows },
    { 0x258D, 0x258F, 0x258E, 0, lb_s26_tab2_sprites, lb_s26_tab2_arrows },
};

/* The tab drawing order on each trade page (the current one last). */
u8 lb_s26_tab_order[12] = { 2, 1, 0, 0, 2, 1, 0, 1, 2, 0, 0, 0 };

u16 lb_s26_trade_list_sprites[11] = {
    0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E, 0x2553, 0x2554, 0x2557, 0x2558, 0xFFFF,
};
u16 lb_s26_reward_row_lsp[6] = { 0x255F, 0x2560, 0x2561, 0x2562, 0xFFFF, 0x0000 };
u16 lb_s26_offer_row_lsp[6] = { 0x2563, 0x2564, 0x2565, 0x2566, 0x2567, 0xFFFF };
u16 lb_s26_trade_frame_sprites[6] = { 0x2569, 0x256A, 0x256B, 0x256C, 0x256D, 0xFFFF };
u16 lb_s26_picks_sprites[38] = {
    0x25B9, 0x25A6, 0x25A7, 0x25A8, 0x25A9, 0x25AA, 0x25AB, 0x25AC, 0x25AD, 0x25AE, 0x25AF, 0x25B0, 0x25B1,
    0x25B2, 0x25B3, 0x25B4, 0x25B5, 0x25B6, 0x25B7, 0x25B8, 0x25BC, 0x25BD, 0x25BA, 0x25BB, 0x25A3, 0x25A4,
    0x25A5, 0x25BE, 0x25BF, 0x25C0, 0x25C1, 0x25A0, 0x25A1, 0x25A2, 0x259D, 0x259E, 0x259F, 0xFFFF,
};
u16 lb_s26_status_sprites[18] = {
    0x24F6, 0x24EE, 0x24EF, 0x24F0, 0x24F1, 0x24F2, 0x24F3, 0x24F4, 0x24F5,
    0x24F7, 0x24F8, 0x24F9, 0x24FA, 0x24FB, 0x24FC, 0x24FD, 0xFFFF, 0x0000,
};
u16 lb_s26_box_cell_lsp[12] = {
    0x2600, 0x2601, 0x2602, 0x2603, 0x2604, 0x2605, 0x2606, 0x2607, 0x2608, 0x2609, 0xFFFF, 0x0000,
};
u16 lb_s26_box_sprites[10] = { 0x25F4, 0x25F5, 0x25F6, 0x25F7, 0x25F8, 0x25F9, 0x25FA, 0x25FB, 0x25FC, 0xFFFF };
u16 lb_s26_reward_sprites[30] = {
    0x25F1, 0x25F2, 0x25EE, 0x25EF, 0x25F0, 0x25DD, 0x25DE, 0x25DF, 0x25E0, 0x25E1, 0x25E2, 0x25E3,
    0x25E4, 0x25E5, 0x25E6, 0x25E7, 0x25E8, 0x25E9, 0x25EA, 0x25EB, 0x25EC, 0x25ED, 0x25D9, 0x25DA,
    0x25DB, 0x25DC, 0xFFFF, 0x0000, 0x0000, 0x0000,
};

/* The counter screen's small sprite runs and row anchors. */
u16 lb_s26_menu_row_lsp[4] = { 0x24EA, 0x24EB, 0x24EC, 0x0000 };
u16 lb_s26_grid_cursor_sprites[4] = { 0x2555, 0x2556, 0xFFFF, 0x0000 };
u16 lb_s26_pick_row_lsp[4] = { 0x25C2, 0x25C3, 0x25C4, 0x0000 };
u16 lb_s26_grid_help_sprites[4] = { 0x25D5, 0x25D6, 0x25D7, 0xFFFF };
u16 lb_s26_status_page_sprites[4] = { 0x24FF, 0x2500, 0x2501, 0x0000 };

/* 0x803C58D4 (0x58): counts the trade counter's waiting days down; the last one marks the reward ready (0xFF). */
void userdata_server_rows_reset(void)
{
    LbTradeUserData* user = (LbTradeUserData*)lobby_world_block;

    if (lb_trade_state_get() == 1) {
        if (--user->trade_wait == 0) {
            user->trade_wait = 0xFF;
        }
    }
}

/* 0x803C592C (0x70): opens the counter screen (screen 0x1A) for `player` talking to `npc`. */
void lb_screen26_open(struct _PLW* player, struct _LB_NPC* npc)
{
    LbScreen26Work* work = lobby_w.screen26_0x0AC;

    memset(work, 0, 0x2000);
    work->player = player;
    work->npc = npc;
    lobby_w.state_0x000 = 0x1A;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* 0x803C599C (0x4): closes the counter screen. */
void lb_screen26_close(void)
{
    lb_panel_close();
}

/* 0x803C59A0 (0x290): the counter screen's step - the top menu, the combine list, the trade and the reward pickup. */
void lb_screen26_step(void)
{
    LbScreen26Work* work = lobby_w.screen26_0x0AC;
    LbTradeUserData* user = (LbTradeUserData*)lobby_world_block;
    u16 off_mask;

    work->moved = 0;
    switch (work->step) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->step = 1;
            off_mask = 0;
            if (lb_trade_state_get() == 1) {
                off_mask = 4;
            }
            memset(&work->menu, 0, sizeof(LbList));
            lb_list_init(&work->menu, 3, 0, 7, 0x10, off_mask, -1, 3, 0);
            camera_talk_lock_set(1);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->menu)) {
        case 1:
            switch ((u32)work->menu.cursor_0x00) {
            case 0:
            case 1:
                work->step = 2;
                lb_screen26_combo_open(work);
                sysSE_req(5);
                break;
            case 2:
                if (lb_trade_state_get() == 2) {
                    work->step = 4;
                    memset(&work->reward, 0, sizeof(Eft052ItemGetReq));
                    work->reward.item_0x00 = user->trade_reward;
                    work->reward.count_0x02 = 1;
                    eft052_item_get_open(&work->reward, lb_item_get_data + 0xA0);
                    sysSE_stop(0xC);
                } else {
                    work->step = 3;
                    lb_screen26_trade_open(work);
                    sysSE_stop(0xE);
                }
                break;
            }
            break;
        case 2:
            work->step = 5;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if ((u32)(lb_screen26_combo_step(work) - 1) <= 1) {
            work->step = 1;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if ((u32)(lb_screen26_trade_step(work) - 1) <= 1) {
            work->step = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(eft052_item_get_step() - 1) <= 1) {
            work->step = 1;
            user->trade_wait = 0;
            user->trade_reward = 0;
        }
        break;
    case 5:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_screen26_close();
        }
        break;
    }
    if (work->step != 0 && work->step != 5) {
        subTransSet((u32)lb_screen26_draw, 0, 0);
    }
}

/* 0x803C5C30 (0x34): the trade counter's state - 0 nothing ordered, 1 waiting, 2 the reward is ready. */
u8 lb_trade_state_get(void)
{
    switch (((LbTradeUserData*)lobby_world_block)->trade_wait) {
    case 0xFF:
        return 2;
    case 0:
        return 0;
    default:
        return 1;
    }
}

/* 0x803C5C64 (0x210): draws trade page `page`'s four rewards and five offered items from the day's seed. */
void lb_screen26_trade_build(LbScreen26Work* work, s16 page)
{
    s16 i;
    u16 seed;
    const u16* rewards;
    u16 pick;
    u16 roll;
    const u16* src;
    s16 left;
    s32 k;

    work->picks[0] = 0;
    work->picks[1] = 0;
    work->picks[2] = 0;
    seed = lobby_w.trade_seed_0x03C;
    for (i = 0; i < page * 73; i++) {
        seed = rand_lcg_step(seed);
    }
    memset(work->pool, 0, sizeof(work->pool));
    rewards = lb_trade_reward_tbl[page];
    pick = seed % 3;
    roll = rand_lcg_step(seed);
    if (userdata_rank_band_get() == 2 && (roll & 1) != 0) {
        pick += 3;
    }
    pick *= 4;
    work->rewards[0] = rewards[pick];
    work->rewards[1] = rewards[pick + 1];
    work->rewards[2] = rewards[pick + 2];
    work->rewards[3] = rewards[pick + 3];
    memset(work->pool, 0, sizeof(work->pool));
    src = lb_trade_offer_tbl[page];
    left = 0;
    while (*src != 0) {
        work->pool[left] = *src;
        left++;
        src++;
    }
    seed = rand_lcg_step(roll);
    for (k = 0; k < 5; k++) {
        pick = seed % left;
        work->offer_items[k] = work->pool[pick];
        seed = rand_lcg_step(seed);
        memcpy(&work->pool[pick], &work->pool[pick + 1], (100 - (pick + 1)) * 2);
        work->offer_counts[k] = userdata_item_count_total(work->offer_items[k], lobby_world_block);
        left--;
    }
}

/* 0x803C5E74 (0x1C): opens the trade on its first page. */
void lb_screen26_trade_open(LbScreen26Work* work)
{
    work->sub_step = 0;
    work->page = 0;
    work->page_count = 3;
    lb_screen26_trade_build(work, 0);
}

/* 0x803C5E90 (0x24): clears the trade's picks and cursors. */
void lb_screen26_pick_reset(LbScreen26Work* work)
{
    work->cursor = 0;
    work->grid_cursor = 0;
    work->picked = 0;
    work->grid_mode = 0;
    memset(work->picks, 0, sizeof(work->picks));
}

/* 0x803C5EB4 (0x3C): adds the item under the cursor to the picks. */
void lb_screen26_pick_add(LbScreen26Work* work)
{
    work->picks[work->picked] = work->offer_items[work->cursor];
    work->offer_counts[work->cursor]--;
}

/* 0x803C5EF0 (0x70): takes the last pick back. */
void lb_screen26_pick_remove(LbScreen26Work* work)
{
    s16 i;

    for (i = 0; i < 5; i++) {
        if (work->picks[work->picked - 1] == work->offer_items[i]) {
            work->picks[work->picked - 1] = 0;
            work->offer_counts[i]++;
            break;
        }
    }
    work->picked--;
}

/* 0x803C5F60 (0x1E0): the trade's pick step - 1 once three items are picked, 2 on cancel. */
s32 lb_screen26_pick_step(LbScreen26Work* work)
{
    s32 result = 0;

    if (work->grid_mode != 0) {
        if (lb_cmd_pressed_ck(0xA0) != 0) {
            work->grid_mode = 0;
            sysSE_req(0);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->grid_cursor = (work->grid_cursor + 2) % 4;
            sysSE_req(3);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            if ((work->grid_cursor & 1) != 0) {
                work->grid_cursor--;
            } else {
                work->grid_cursor++;
            }
            sysSE_req(3);
        }
    } else if (lb_cmd_pressed_ck(0x10) != 0) {
        if (work->offer_counts[work->cursor] != 0) {
            lb_screen26_pick_add(work);
            if (++work->picked >= 3) {
                result = 1;
            }
            sysSE_req(0);
        } else {
            sysSE_req(2);
        }
    } else if (lb_cmd_pressed_ck(0x20) != 0) {
        if (work->picked == 0) {
            result = 2;
        } else {
            lb_screen26_pick_remove(work);
        }
        sysSE_req(1);
    } else if (lb_cmd_pressed_ck(0x80) != 0) {
        work->grid_mode = 1;
        sysSE_req(0);
    } else if (lb_cmd_repeat_ck(3) != 0) {
        work->cursor = menu_cursor_step(work->cursor, 5, lb_cmd_repeat_get(), 1, 2);
    }
    return result;
}

/* 0x803C6140 (0x11C): commits the trade - rolls the reward from the day's second seed, starts the three-day wait
 * and hands the picks over. */
void lb_screen26_trade_commit(LbScreen26Work* work)
{
    LbTradeUserData* user = (LbTradeUserData*)lobby_world_block;
    s32 pick = 0;
    u16 roll = lobby_w.trade_seed_0x03E % 100;
    u16 sum = 0;
    s32 i;

    for (i = 0; i < 4; i++) {
        sum += lb_trade_reward_odds[i];
        if (roll < sum) {
            pick = i;
            break;
        }
    }
    user->trade_reward = work->rewards[pick];
    user->trade_wait = 3;
    user->trade_page = work->page;
    for (i = 0; i < 3; i++) {
        eft052_page_count_add(work->picks[i], 1);
    }
}

/* 0x803C625C (0x1B4): the trade step - page choice, picking, the confirmation; 1 once traded, 2 on cancel. */
s32 lb_screen26_trade_step(LbScreen26Work* work)
{
    s32 result = 0;

    switch (work->sub_step) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            work->sub_step = 1;
            lb_screen26_pick_reset(work);
            sysSE_req(5);
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->page = menu_cursor_step_forward(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8, 6,
                                                  &work->moved);
            lb_screen26_trade_build(work, work->page);
        }
        break;
    case 1:
        switch (lb_screen26_pick_step(work)) {
        case 1:
            work->sub_step = 2;
            work->confirm = 0;
            break;
        case 2:
            work->sub_step = 0;
            break;
        }
        break;
    case 2:
        if (++work->blink >= 20) {
            work->blink = 0;
        }
        switch (toggle_word_step_c1(&work->confirm, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            result = 1;
            lb_screen26_trade_commit(work);
            work->menu.off_mask_0x02 = 4;
            sysSE_stop(0x25);
            break;
        case 2:
            work->sub_step = 1;
            lb_screen26_pick_remove(work);
            break;
        }
        break;
    }
    return result;
}

/* 0x803C6410 (0x154): the reasons combine row `row` cannot be made `count` times: 0x80 no room (0x82 none at all),
 * 4 not enough money, 8 not enough ingredients. */
u16 lb_screen26_combo_flags_get(LbScreen26Work* work, LbComboRow* row, s32 count, s16 mode)
{
    LbTradeUserData* user;
    u16 flags = 0;
    s32 room = row->yield * count;

    if (room > eft052_page_count_ck(row->result, mode != 0)) {
        flags |= 0x80;
        room = 0;
        if (mode == 1) {
            room = (u16)item_slots_free_count(((LbTradeUserData*)lobby_world_block)->box,
                                              userdata_box_capacity(lobby_world_block));
        } else {
            if (GetItemData(row->result)->kind_0x00 == 1 && pl_item_page2_ck(work->player) != 0) {
                room = (u16)item_slots_free_count(((LbTradeUserData*)lobby_world_block)->supplies, 8);
            }
            room += (u16)item_slots_free_count((IdValue*)userdata_equip_item_slots_get(lobby_world_block), 24);
        }
        if (room == 0) {
            flags |= 2;
        }
    }
    user = (LbTradeUserData*)lobby_world_block;
    if (row->price * count > user->money) {
        flags |= 4;
    }
    if (userdata_item_count_total(row->item_a, lobby_world_block) < count) {
        flags |= 8;
    }
    if (userdata_item_count_total(row->item_b, lobby_world_block) < count) {
        flags |= 8;
    }
    return flags;
}

/* 0x803C6564 (0x180): fills the combine list's page `page` with the known recipes the counter offers. */
void lb_screen26_combo_build(LbScreen26Work* work, s16 page)
{
    LbComboRow* row;
    s32 first = page * 8;
    s32 last = first + 7;
    s16 n;
    u16 item_a;
    u16 item_b;
    struct LbComboRecipe* recipe;
    ItemDataRecord* data;
    u16 i;

    work->row_count = 0;
    memset(work->rows, 0, sizeof(work->rows));
    n = 0;
    for (i = 0; i < 131; i++) {
        recipe = lb_page_row_pair_get(i, &item_a, &item_b, 0);
        if (lb_combo_recipe_known_ck(recipe, 0) != 1) {
            continue;
        }
        data = GetItemData(recipe->result);
        if ((data->field_0x002 & 3) != 0) {
            continue;
        }
        if (first <= n && n <= last) {
            row = &work->rows[work->row_count];
            row->result = recipe->result;
            row->item_a = item_a;
            row->item_b = item_b;
            row->yield = lb_combo_yield_tbl[recipe->yield_group];
            row->price = lb_combo_price_tbl[data->level_0x01];
            row->flags = lb_screen26_combo_flags_get(work, row, 1, work->menu.cursor_0x00);
            work->row_count++;
        }
        n++;
    }
    if (work->cursor >= work->row_count) {
        work->cursor = work->row_count - 1;
    }
    work->page = page;
    work->page_count = menu_page_count(n, 8);
    if (work->page_count == 0) {
        work->page_count = 1;
    }
}

/* 0x803C66E4 (0x1C): opens the combine list on its first page. */
void lb_screen26_combo_open(LbScreen26Work* work)
{
    work->sub_step = 0;
    work->cursor = 0;
    work->count = 1;
    lb_screen26_combo_build(work, 0);
}

/* 0x803C6700 (0xE4): combines the row under the cursor `count` times, pays for it and stores the results. */
void lb_screen26_combo_exec(LbScreen26Work* work)
{
    LbComboRow* row = &work->rows[work->cursor];
    s32 total = 0;
    s16 i;
    u8 yield;

    for (i = 0; i < work->count; i++) {
        lb_combo_exec(&yield, row->item_a, row->item_b, 1, 0xFFFF, 0xFFFF);
        total += yield;
    }
    score_add_clamped(-(row->price * work->count), &((LbTradeUserData*)lobby_world_block)->money);
    sysSE_req(9);
    if (work->menu.cursor_0x00 == 1) {
        eft052_page_put(work->player, row->result, total, 0);
    } else {
        eft052_page_take(work->player, row->result, total, 0);
    }
}

/* 0x803C67E4 (0x29C): the combine list's step - row choice, the count window, the confirmation; 2 on cancel. */
s32 lb_screen26_combo_step(LbScreen26Work* work)
{
    s32 result = 0;
    s32 n;

    switch (work->sub_step) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (work->rows[work->cursor].flags == 0 && work->row_count != 0) {
                work->sub_step = 1;
                work->count = 1;
                work->count_max = 99;
                for (n = 99; n > 0; n--) {
                    if (lb_screen26_combo_flags_get(work, &work->rows[work->cursor], n, work->menu.cursor_0x00) == 0) {
                        work->count_max = n;
                        break;
                    }
                }
                sysSE_req(5);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cursor = menu_cursor_step(work->cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                                     &work->moved);
            lb_screen26_combo_build(work, work->page);
        }
        break;
    case 1:
        ainpc_page_hold_clear();
        switch (menu_cursor_page_move(&work->count, work->count_max, lobby_w.count_0x084, lb_cmd_repeat_get(),
                                      &work->moved)) {
        case 1:
            work->sub_step = 2;
            work->confirm = 0;
            sysSE_req(0);
            break;
        case 2:
            work->sub_step = 0;
            work->count = 1;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        switch (toggle_word_step_c1(&work->confirm, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->sub_step = 0;
            lb_screen26_combo_exec(work);
            lb_screen26_combo_build(work, work->page);
            lb_npc_talk_mode_set(work->npc, 3);
            work->count = 1;
            break;
        case 2:
            work->sub_step = 1;
            break;
        }
        break;
    }
    return result;
}


/* 0x803C6A80 (0x88): draws the counter screen's frame (the wide-screen variant when set) and the money. */
void lb_screen26_bg_draw(LbScreen26Work* work)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x24D7, &pos);
        draw_sprite_ary(lb_s26_frame_wide_sprites, &pos);
    } else {
        get_lsp_data(0x24D2, &pos);
        draw_sprite_ary(lb_s26_frame_sprites, &pos);
    }
    get_lsp_data(0x24BE, &pos);
    draw_sprite_ary(lb_s26_bg_sprites, &pos);
    menu_money_draw(0x24DC);
}

/* 0x803C6B08 (0x100): draws the top menu's rows (the trade row reads "collect" once the reward is ready). */
void lb_screen26_menu_draw(LbScreen26Work* work)
{
    _mh_ivec2_ pos;
    const u16* lsp;
    s32 i;
    bool grey;
    u16 str;
    bool picked;
    s32 color;

    get_lsp_data(0x24DE, &pos);
    draw_sprite_anim_ary(lb_s26_menu_sprites, work->menu.count_0x06, &pos);
    for (i = 0, lsp = lb_s26_menu_row_lsp; i < work->menu.count_0x06; lsp++, i++) {
        picked = work->menu.cursor_0x00 == i;
        grey = (work->menu.off_mask_0x02 & (1 << i)) == 0;
        str = work->menu.str_base_0x0A + i;
        if (i == 2 && lb_trade_state_get() == 2) {
            str = 10;
        }
        color = GetMenuFontColor(grey, picked, true, false);
        lb_frame_draw_at(0x24DE, *lsp, (s8*)LbStr(0, str), picked, color, work->menu.kind_0x08);
    }
}

/* 0x803C6C08 (0x170): draws the combine list's page: its arrows and rows, or the "nothing to combine" line. */
void lb_screen26_combo_list_draw(LbScreen26Work* work, u32 active)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbComboRow* row;
    const u16* lsp;
    s16 i;
    s32 picked;
    s32 grey;
    u16 moved;

    get_lsp_data(0x2507, &base);
    draw_sprite_ary(lb_s26_combo_list_sprites, &base);
    if (active == 1) {
        moved = work->moved;
    } else {
        moved = 0;
    }
    lb_page_arrow_draw(0x2505, work->page, work->page_count, moved, active);
    if (work->row_count == 0) {
        spr_data_copy(&spr, get_lsp_data(0x2512, NULL));
        lb_text_center_print(0x2511, (s8*)LbStr(3, 24), 0, 18, &base, spr.color);
        return;
    }
    for (i = 0, row = work->rows, lsp = lb_s26_combo_row_lsp; i < work->row_count; row++, lsp++, i++) {
        picked = i == work->cursor;
        grey = work->rows[i].flags == 0;
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_item_cell_draw_layout1_c1(row->result, row->price, (s16*)&pos, grey, picked, active, 2);
    }
}

/* 0x803C6D78 (0x154): draws the picked recipe's two ingredients with the counts held. */
void lb_screen26_combo_items_draw(LbScreen26Work* work)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbComboRow* row;
    s32 count;

    get_lsp_data(0x2540, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x36), &base);
    row = &work->rows[work->cursor];
    get_lsp_data(0x2541, &pos);
    pos.x += base.x;
    pos.y += base.y;
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x37), &pos);
    count = userdata_item_count_total(row->item_a, lobby_world_block);
    menu_item_cell_draw(&pos, row->item_a, count, work->count <= count, 0, 1);
    get_lsp_data(0x2542, &pos);
    pos.x += base.x;
    pos.y += base.y;
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x37), &pos);
    count = userdata_item_count_total(row->item_b, lobby_world_block);
    menu_item_cell_draw(&pos, row->item_b, count, work->count <= count, 0, 1);
    draw_sprite_idx(0x374, &base);
}

/* 0x803C6ECC (0xC0): draws the picked recipe's result: its panel, the yield per combination and in total, the text. */
void lb_screen26_combo_info_draw(LbScreen26Work* work)
{
    _mh_ivec2_ pos;
    LbComboRow* row = &work->rows[work->cursor];

    get_lsp_data(0x251B, &pos);
    draw_sprite_ary(lb_s26_combo_info_sprites, &pos);
    lb_item_detail_panel_draw(0x253F, row->result, 2);
    if (row->result != 0) {
        draw_font_idx_num(0x251E, row->yield, 1, &pos);
        draw_font_idx_num(0x251F, row->yield * work->count, 1, &pos);
        draw_font_idx(0x2520, (s8*)ItemExp(row->result), 0, &pos);
    }
}

/* 0x803C6F8C (0xC4): draws the combine screen in its current step. */
void lb_screen26_combo_draw(LbScreen26Work* work)
{
    switch (work->sub_step) {
    case 0:
        lb_screen26_combo_list_draw(work, 1);
        lb_screen26_combo_items_draw(work);
        lb_screen26_combo_info_draw(work);
        break;
    case 1:
        lb_screen26_combo_list_draw(work, 0);
        lb_screen26_combo_items_draw(work);
        lb_screen26_combo_info_draw(work);
        lb_count_window_draw(0x2545, 0x2546, work->count, work->count_max, work->rows[work->cursor].price, 0,
                             work->moved);
        break;
    case 2:
        lb_screen26_combo_list_draw(work, 0);
        lb_screen26_combo_items_draw(work);
        lb_screen26_combo_info_draw(work);
        break;
    }
}

/* 0x803C7050 (0x190): draws the three trade page tabs, the current one blinking and with its arrows. */
void lb_screen26_trade_tabs_draw(LbScreen26Work* work, u32 active, u8 arrows)
{
    _mh_ivec2_ pos;
    s32 i;
    u8 tab;
    u16 frame;
    u16 frame_on = active == 0 ? 6 : 0;
    LbTradeTab* t;
    u16 blink;

    for (i = 0; i < 3; i++) {
        tab = lb_s26_tab_order[work->page * 3 + i];
        if (tab != work->page) {
            frame = 6;
            blink = 0;
        } else {
            frame = frame_on;
            blink = (u32)lobby_w.counter_0x030 % 40;
        }
        t = &lb_s26_trade_tabs[tab];
        get_lsp_data(t->lsp, &pos);
        draw_sprite_anim_ary(t->sprites, frame, &pos);
        if (chk_pointer() == 0) {
            if (tab == work->page) {
                draw_sprite_anim_idx(t->blink, blink, &pos);
            }
            if (tab == work->page && active == 1 && arrows != 0) {
                draw_sprite_ary(t->arrows, &pos);
                if (lb_cmd_repeat_ck(4) != 0) {
                    draw_sprite_anim_idx(t->arrows[2], 1, &pos);
                } else if (lb_cmd_repeat_ck(8) != 0) {
                    draw_sprite_anim_idx(t->arrows[3], 1, &pos);
                }
            }
        }
        draw_sprite_anim_idx(t->frame, frame, &pos);
    }
}

/* 0x803C71E0 (0x270): draws the trade page: the four rewards and the five offered items with the counts held. */
void lb_screen26_trade_list_draw(LbScreen26Work* work, bool active, u8 locked)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 hold[2];
    s16 i;
    const u16* lsp;
    bool held;
    bool picked;
    s32 color;

    get_lsp_data(0x2568, &base);
    draw_sprite_ary(lb_s26_trade_frame_sprites, &base);
    get_lsp_data(0x2552, &base);
    draw_sprite_ary(lb_s26_trade_list_sprites, &base);
    for (i = 0, lsp = lb_s26_reward_row_lsp; i < 4; lsp++, i++) {
        if (i == work->grid_cursor && locked == 0 && work->grid_mode == 1) {
            picked = true;
            hold[0] = work->rewards[i];
            hold[1] = 0;
            menu_hold_row_draw_by_lsp(0x2547, hold);
        } else {
            picked = false;
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        color = GetMenuFontColor(true, picked, active, false);
        lb_list_row_draw((s8*)ItemName(work->rewards[i]), picked, &pos, color, 14);
    }
    for (i = 0, lsp = lb_s26_offer_row_lsp; i < 5; lsp++, i++) {
        if (i == work->cursor && locked == 0 && work->grid_mode == 0) {
            picked = true;
            hold[0] = work->offer_items[i];
            hold[1] = 0;
            menu_hold_row_draw_by_lsp(0x2547, hold);
        } else {
            picked = false;
        }
        held = work->offer_counts[i] != 0;
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        GetMenuFontColor(held, picked, active, false);
        lb_item_cell_draw_layout1_c1(work->offer_items[i], work->offer_counts[i], (s16*)&pos, held, picked, active, 0);
    }
    if (work->sub_step != 0) {
        draw_sprite_anim_ary(lb_s26_grid_cursor_sprites, work->blink % 40, &base);
    }
}

/* 0x803C7450 (0xE0): draws the three picks. */
void lb_screen26_picks_draw(LbScreen26Work* work, u32 active)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    const u16* lsp;
    s32 picked;
    s16 i;

    get_lsp_data(0x259C, &base);
    draw_sprite_ary(lb_s26_picks_sprites, &base);
    for (i = 0, lsp = lb_s26_pick_row_lsp; i < 3; lsp++, i++) {
        if (i == work->picked && active == 1) {
            picked = 1;
        } else {
            picked = 0;
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_item_cell_draw_layout11(work->picks[i], 0, (s32)&pos, 1, picked, 1, 0x80);
    }
}

/* 0x803C7530 (0xA4): draws the reward grid's help: the button sprite for the controller setting and its text. */
void lb_screen26_grid_help_draw(LbScreen26Work* work)
{
    _mh_ivec2_ pos;

    if (chk_pointer() == 0) {
        get_lsp_data(0x25D1, &pos);
        draw_sprite_ary(lb_s26_grid_help_sprites, &pos);
        if (get_option_cfg(7) == 0) {
            draw_sprite_idx(0x25D3, &pos);
        } else {
            draw_sprite_idx(0x25D4, &pos);
        }
        draw_font_idx(0x25D2, (s8*)LbStr(0, work->grid_mode + 15), 0, &pos);
    }
}

/* 0x803C75D4 (0xC4): draws the trade screen in its current step. */
void lb_screen26_trade_draw(LbScreen26Work* work)
{
    switch (work->sub_step) {
    case 0:
        lb_screen26_trade_tabs_draw(work, 1, 1);
        lb_screen26_trade_list_draw(work, 0, 1);
        break;
    case 1:
        lb_screen26_trade_tabs_draw(work, 1, 0);
        lb_screen26_trade_list_draw(work, 1, 0);
        lb_screen26_picks_draw(work, 1);
        lb_screen26_grid_help_draw(work);
        break;
    case 2:
        lb_screen26_trade_tabs_draw(work, 1, 0);
        lb_screen26_trade_list_draw(work, 0, 1);
        lb_screen26_picks_draw(work, 0);
        break;
    }
}

/* 0x803C7698 (0x178): draws the trade order's status: nothing ordered, the days left, or the reward ready. */
void lb_screen26_status_draw(void)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s8 text[48];
    u8 wait;
    LbTradeUserData* user = (LbTradeUserData*)lobby_world_block;
    s8* unit;

    get_lsp_data(0x24ED, &base);
    draw_sprite_ary(lb_s26_status_sprites, &base);
    if (user->trade_wait != 0) {
        draw_sprite_idx(0x24FE, &base);
        draw_sprite_idx(lb_s26_status_page_sprites[user->trade_page], &base);
    }
    draw_font_idx(0x2502, (s8*)LbStr(0, 11), 1, &base);
    wait = user->trade_wait;
    switch (wait) {
    case 0xFF:
        pos.x = base.x - 13;
        pos.y = base.y;
        draw_font_idx(0x2503, (s8*)ItemName(user->trade_reward), 0, &pos);
        break;
    case 0:
        draw_font_idx(0x2503, (s8*)LbStr(0, 12), 0, &base);
        break;
    default:
        unit = (s8*)LbStr(0, 14);
        sprintf((char*)text, "%s%s", (char*)LbStr(0, 13), (char*)unit, wait);
        sprintf((char*)text, (char*)text, user->trade_wait);
        draw_font_idx(0x2503, text, 0, &base);
        break;
    }
}

/* 0x803C7810 (0x70): the position of item box cell `index` (ten to a page). */
void lb_screen26_box_cell_pos_get(s32 index, _mh_ivec2_* out)
{
    _mh_ivec2_ pos;

    get_lsp_data(lb_s26_box_cell_lsp[index % 10], &pos);
    out->x = pos.x;
    out->y = pos.y;
}

/* 0x803C7880 (0x2A0): draws the item box page of the reward pickup: the cells, the cursor and the picked cell. */
void lb_screen26_box_draw(Eft052ItemBox* box)
{
    _SPR_DATA_ cell;
    _SPR_DATA_ icon;
    _SPR_DATA_ cursor;
    _mh_ivec2_ at;
    _mh_ivec2_ origin;
    _mh_ivec2_ picked_at;
    _mh_ivec2_ base;
    s32 i;
    s32 picked;
    Eft052BoxCell* cells;

    get_lsp_data(0x25F3, &base);
    draw_sprite_anim_ary(lb_s26_box_sprites, 0, &base);
    picked = 0;
    spr_data_copy(&icon, get_lsp_data(0x1307, NULL));
    uv_pair_copy(&origin, &icon);
    get_lsp_data(0x25FF, &base);
    for (i = 0; i < 10; i++) {
        lb_screen26_box_cell_pos_get(i, &at);
        spr_data_copy(&cell, get_lsp_data(0x2609, NULL));
        uv_pair_copy(&cell, &at);
        icon.pos.x = at.x + origin.x;
        icon.pos.y = at.y + origin.y;
        draw_sprite(cell, &base);
        if (box->cursor_0x0E == i) {
            picked = 1;
            uv_pair_copy(&picked_at, &at);
        }
        if ((u8)(box->state_0x00 - 1) <= 1 && chk_pointer() == 0 && box->hover_0x10 == i) {
            spr_data_copy(&cursor, get_lsp_data(0x1304, NULL));
            cursor.pos.x += at.x;
            cursor.pos.y += at.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw(&cursor, box->frame_0x08, &base);
            set_blendmode(4, 5, 1);
        }
        if (box->cells_0x24[i].item_0x00 != 0) {
            draw_itemicon_item_id(icon, box->cells_0x24[i].item_0x00, &base);
        }
    }
    if (box->picked_0x0C != 0 && picked != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy(&cursor, get_lsp_data(0x1305, NULL));
        cursor.pos.x += picked_at.x;
        cursor.pos.y += picked_at.y;
        spr_anim_draw(&cursor, box->frame_0x0A, &base);
        set_blendmode(4, 5, 1);
        spr_data_copy(&cursor, get_lsp_data(0x1306, NULL));
        cursor.pos.x += picked_at.x;
        cursor.pos.y += picked_at.y;
        spr_anim_draw(&cursor, box->frame_0x0A, &base);
    }
}

/* 0x803C7B20 (0x78): draws the reward pickup: the frame, the item box page, its counts and its menu. */
void lb_screen26_reward_draw(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x25D8, &pos);
    draw_sprite_ary(lb_s26_reward_sprites, &pos);
    lb_screen26_box_draw(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 0x25FE, 0);
    eft052_box_list_draw(0x260B, lb_s26_menu_sprites, lb_s26_menu_row_lsp, 0x2467, 0);
}

/* 0x803C7B98 (0x258): draws the help panel's message lines for the screen's current step. */
void lb_screen26_help_draw(LbScreen26Work* work)
{
    u16 msg = 0xFFFF;
    u16 line = 0xFFFF;
    s32 color = 1;
    u16 flags;
    s16 mode;

    switch (work->step) {
    case 1:
        msg = work->menu.cursor_0x00 + 0x10;
        if (work->menu.cursor_0x00 == 2) {
            switch (lb_trade_state_get()) {
            case 1:
                line = 0x14;
                color = 2;
                break;
            case 2:
                msg = 0x13;
                break;
            }
        }
        break;
    case 2:
        switch (work->sub_step) {
        case 0:
            mode = work->menu.cursor_0x00;
            if (mode == 0) {
                msg = 0x15;
                color = 1;
            } else {
                msg = 0x16;
                color = 2;
            }
            flags = work->rows[work->cursor].flags;
            if ((flags & 0x80) != 0) {
                if ((flags & 2) != 0) {
                    if (mode == 1) {
                        line = 0x30;
                    } else {
                        line = 0x2F;
                    }
                } else {
                    line = 0x31;
                }
                color = 2;
            } else if ((flags & 8) != 0) {
                line = 0x17;
                color = 2;
            } else if ((flags & 4) != 0) {
                line = 0x52;
                color = 2;
            }
            break;
        case 1:
            msg = 0x19;
            break;
        case 2:
            lb_help_num_put(0x1877, 0x1A, work->count * work->rows[work->cursor].price);
            lb_panel_yes_no_draw(0x1879, work->confirm);
            return;
        }
        break;
    case 3:
        switch (work->sub_step) {
        case 0:
            msg = 0x1B;
            break;
        case 1:
            if (work->grid_mode == 0) {
                msg = 0x1C;
                if (work->offer_counts[work->cursor] == 0) {
                    line = 0x1D;
                    color = 2;
                }
            } else {
                msg = 0x1E;
            }
            break;
        case 2:
            msg = 0x1F;
            lb_panel_yes_no_draw(0x1879, work->confirm);
            break;
        }
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (line != 0xFFFF) {
            lb_panel_line_draw(0x1877, line, 2, color);
        }
    }
}

/* 0x803C7DF0 (0xB4): the counter screen's draw callback. */
void lb_screen26_draw(void)
{
    LbScreen26Work* work = lobby_w.screen26_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_screen26_bg_draw(work);
    switch (work->step) {
    case 1:
        lb_screen26_menu_draw(work);
        lb_screen26_status_draw();
        break;
    case 2:
        lb_screen26_combo_draw(work);
        break;
    case 3:
        lb_screen26_trade_draw(work);
        break;
    case 4:
        lb_screen26_reward_draw();
        break;
    }
    lb_screen26_help_draw(work);
}


} /* extern "C" */

/* 0x803C7EA4 (0x8): the address of the quest NPCs' talk table pointer. */
NpcTalkRec*** get_qnpc_talk_data_ptr(void)
{
    return &qnpc_talk_data;
}

extern "C" {

/* 0x803C7EAC (0x70): clears the talk window. */
void npc_talk_init(void)
{
    NpcTalkWork* work = &npc_talk_work;

    work->speaker = -1;
    memset(work->text, 0, sizeof(work->text));
    work->kind = 0;
    work->msg_a = 0;
    work->msg_b = 0;
    work->active = 0;
    work->index = 0;
    work->wait = 0;
    work->flag = 0;
    work->tick = 0;
    work->shown = 0;
    work->done = 0;
}

/* 0x803C7F1C (0x6C): clears the talk window for the next message (its flag byte survives). */
void npc_talk_reset(void)
{
    NpcTalkWork* work = &npc_talk_work;

    work->speaker = -1;
    memset(work->text, 0, sizeof(work->text));
    work->kind = 0;
    work->msg_a = 0;
    work->msg_b = 0;
    work->active = 0;
    work->index = 0;
    work->wait = 0;
    work->tick = 0;
    work->shown = 0;
    work->done = 0;
}

/* 0x803C7F88 (0x2A0): the talk window's frame step - closes it when the player leaves or a demo starts, types the
 * text out and runs the message kind's input. */
void npc_talk_update(void)
{
    NpcTalkWork* work = &npc_talk_work;
    struct _PLW* plw = my_player_work_get();

    if (work->flag != 0) {
        work->flag--;
    }
    if (work->active == 0) {
        work->speaker = -1;
        return;
    }
    if (event_demo_ck() == 1) {
        work->active = 0;
        return;
    }
    if (work->area != get_now_areano()) {
        work->active = 0;
        return;
    }
    if (Pl_motion_input_ck(1) == 1) {
        work->active = 0;
        return;
    }
    if (menu_frame_busy_ck((MenuFrameWork*)plw) == 1) {
        work->active = 0;
        return;
    }
    if (work->done == 0) {
        if (npc_talk_input_step(work) == 2) {
            work->done = 1;
            work->shown = font_str_char_count(work->text);
            work->wait = 6;
            sysSE_req(0x22);
        } else {
            work->shown = work->tick / 2 + 1;
            work->tick++;
        }
    }
    switch (work->kind) {
    case 0:
        if (npc_talk_input_step(work) == 1) {
            work->active = 0;
        }
        subTransSetPrio(4, (u32)npc_talk_draw, 0, 0);
        break;
    case 1:
        if (npc_talk_input_step(work) == 1) {
            if (work->choice == 0) {
                npc_talk_start(work->speaker, work->msg_a, work->item, 1);
            } else if (work->choice == 1) {
                npc_talk_start(work->speaker, work->msg_b, work->item, 1);
            }
        }
        subTransSetPrio(4, (u32)npc_talk_draw, 0, 0);
        break;
    case 2:
        if (npc_talk_input_step(work) == 1) {
            npc_talk_start(work->speaker, work->msg_a, work->item, 1);
        }
        subTransSetPrio(4, (u32)npc_talk_draw, 0, 0);
        break;
    case 3:
        if (--work->msg_a == 0) {
            work->active = 0;
        }
        subTransSetPrio(4, (u32)npc_talk_draw, 0, 0);
        break;
    default:
        npc_talk_init();
        break;
    }
}

/* The plain-string format the talk window prints with (an 8-byte buffer in the retail `.sdata`). */
static char npc_talk_str_fmt[8] = "%s";

/* 0x803C8228 (0x22C): draws the talk window: the speaker's name, the typed text and the choice or wait marker. */
void npc_talk_draw(void)
{
    NpcTalkWork* work = &npc_talk_work;
    char** names;
    _mh_ivec2_ pos;
    s16 color_a;
    s16 color_b;
    char* text_a;
    char* text_b;
    s32 len;

    if (work->speaker == -1) {
        return;
    }
    names = npc_talk_name_tbl[system_w.field_0x09];
    font_set_size(16, 16);
    draw_help_window(0);
    note_box_anchor_upper(&pos);
    font_print_ex(pos.x, pos.y, 5, (s8*)npc_talk_str_fmt, names[work->speaker]);
    font_set_size(18, 18);
    note_box_anchor_lower(&pos);
    if (work->done == 1) {
        switch (work->kind) {
        case 1:
            if (work->choice == 0) {
                color_a = 6;
                color_b = 0;
                text_a = names[11];
                text_b = names[12];
            } else {
                color_a = 0;
                color_b = 6;
                text_a = names[13];
                text_b = names[14];
            }
            len = flfntStrLen(text_a);
            font_print_ex(pos.x + 180, pos.y + 40, color_a, (s8*)text_a);
            font_print_ex(pos.x + 180 + len * 9, pos.y + 40, color_b, (s8*)text_b);
            break;
        case 0:
        case 2:
            draw_spin_icon(pos.x + 347, pos.y + 50, 0);
            break;
        }
    }
    if (work->kind == 3) {
        font_print_ex(pos.x, pos.y, 0, (s8*)npc_talk_str_fmt, work->text);
        return;
    }
    if (work->done == 0) {
        if (font_print_typed(pos.x, pos.y, 0, work->shown, work->text) != 0) {
            work->done = 1;
            sysSE_req(0x22);
        }
    } else {
        font_print_ex(pos.x, pos.y, 0, (s8*)npc_talk_str_fmt, work->text);
    }
}

/* 0x803C8454 (0x20C): opens NPC `npc`'s talk message `msg` (with item `item` named in it, or the message text as
 * is when 0) in window kind `kind`; 0 while a demo runs or the window cannot open. */
s32 npc_talk_start(s8 npc, u8 msg, u16 item, u8 kind)
{
    NpcTalkWork* work = &npc_talk_work;
    struct _PLW* plw = my_player_work_get();
    NpcTalkRec* recs;

    if (event_demo_ck() == 1) {
        return 0;
    }
    switch (kind) {
    case 0:
        if (npc_talk_active_ck() != 0) {
            return 0;
        }
        if (menu_frame_busy_ck((MenuFrameWork*)plw) == 1) {
            return 0;
        }
        break;
    case 2:
        if (npc_talk_active_ck() != 0 && (talk_msg_speaker_get() == 0 || talk_msg_speaker_get() == 1)) {
            return 0;
        }
        if (menu_frame_busy_ck((MenuFrameWork*)plw) == 1) {
            return 0;
        }
        break;
    }
    if (npc >= 11) {
        return 0;
    }
    recs = qnpc_talk_data[npc];
    if (recs == NULL) {
        return 0;
    }
    switch (npc) {
    case 0:
        if (npc != talk_msg_speaker_get()) {
            se_talk_npc0_bank_req();
        }
        break;
    case 1:
        if (npc != talk_msg_speaker_get()) {
            se_talk_bank_req();
        }
        break;
    default:
        se_talk_bank_req();
        break;
    }
    npc_talk_reset();
    work->active = 1;
    work->speaker = npc;
    work->kind = recs[msg].kind;
    if (work->kind == 1) {
        work->choice = 1;
    }
    work->msg_a = recs[msg].msg_a;
    work->msg_b = recs[msg].msg_b;
    work->item = item;
    work->wait = 6;
    work->index = msg;
    work->area = get_now_areano();
    if (work->item == 0) {
        sprintf(work->text, npc_talk_str_fmt, recs[msg].text);
    } else {
        npc_talk_text_expand((u8*)work->text, (const s8*)recs[msg].text, work->item);
    }
    return 1;
}

/* 0x803C8660 (0x10): non-zero while the talk window is open. */
u8 npc_talk_active_ck(void)
{
    return npc_talk_work.active;
}

/* 0x803C8670 (0x10): the talk window's speaker (-1: none). */
s8 talk_msg_speaker_get(void)
{
    return npc_talk_work.speaker;
}

/* 0x803C8680 (0x10): the talk window's choice (0 the first answer, 1 the second). */
u8 talk_msg_choice_ck(void)
{
    return npc_talk_work.choice;
}

/* 0x803C8690 (0x14): closes the talk window. */
void npc_talk_end(void)
{
    npc_talk_work.active = 0;
}

/* 0x803C86A4 (0x10): the talk window's message index. */
u8 talk_msg_index_get(void)
{
    return npc_talk_work.index;
}

/* 0x803C86B4 (0x10): sets the talk window's countdown flag. */
void npc_talk_flag_set(u8 flag)
{
    npc_talk_work.flag = flag;
}

/* 0x803C86C4 (0x10): the talk window's countdown flag. */
u8 npc_talk_flag_get(void)
{
    return npc_talk_work.flag;
}

/* The highlight the talk text puts around an item name (a 16-byte buffer in the retail `.data`). */
static char npc_talk_item_fmt[16] = "~C05%s~C00";

/* 0x803C86D4 (0xC0): copies `src` into `dst`, the "||" marker replaced with item `item`'s highlighted name. */
void npc_talk_text_expand(u8* dst, const s8* src, u16 item)
{
    const s8* s = src;
    u8* d = dst;

    while (*s != '\0') {
        if (s[0] == '|' && s[1] == '|') {
            s += 2;
            d += (s8)sprintf((char*)d, npc_talk_item_fmt, (char*)ItemName(item));
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

/* The trade pages' rewards: six sets of four (the second three on a high rank band), then a terminator. */
u16 lb_trade_rewards_0[26] = {
    0x0007, 0x0014, 0x0012, 0x0178, 0x0002, 0x0237, 0x0259, 0x0178, 0x0005, 0x0017, 0x0015, 0x0178, 0x0007,
    0x0014, 0x0178, 0x0013, 0x0005, 0x0017, 0x0178, 0x0016, 0x0007, 0x0005, 0x0178, 0x0034, 0x0000, 0x0000,
};
u16 lb_trade_rewards_1[26] = {
    0x0164, 0x016A, 0x014B, 0x0178, 0x000E, 0x0166, 0x014C, 0x0178, 0x0111, 0x016B, 0x014E, 0x0178, 0x0164,
    0x016B, 0x0178, 0x0179, 0x000E, 0x016A, 0x0178, 0x0168, 0x016B, 0x016A, 0x0178, 0x0033, 0x0000, 0x0000,
};
u16 lb_trade_rewards_2[26] = {
    0x0176, 0x018E, 0x0010, 0x0178, 0x017A, 0x0003, 0x000F, 0x0178, 0x016E, 0x0004, 0x0239, 0x0178, 0x0176,
    0x018E, 0x0178, 0x01AF, 0x0004, 0x0003, 0x0178, 0x023A, 0x017A, 0x018F, 0x0178, 0x023B, 0x0000, 0x0000,
};

/* The trade pages' offer lists (zero-terminated item ids). */
u16 lb_trade_offers_0[18] = {
    0x0061, 0x0097, 0x0063, 0x00C3, 0x0074, 0x0187, 0x0188, 0x0189, 0x018A,
    0x001B, 0x001C, 0x0131, 0x0141, 0x014A, 0x0234, 0x00D0, 0x0000, 0x0000,
};
u16 lb_trade_offers_1[16] = {
    0x006B, 0x01BF, 0x00C6, 0x0162, 0x0086, 0x0087, 0x0089, 0x0091,
    0x0142, 0x0132, 0x0135, 0x0233, 0x0190, 0x0153, 0x0220, 0x0000,
};
u16 lb_trade_offers_2[16] = {
    0x0064, 0x0065, 0x0147, 0x0077, 0x0075, 0x0170, 0x008C, 0x009A,
    0x0191, 0x0154, 0x0221, 0x0151, 0x0150, 0x00D6, 0x00D7, 0x0000,
};

u16* lb_trade_reward_tbl[3] = { lb_trade_rewards_0, lb_trade_rewards_1, lb_trade_rewards_2 };
u16* lb_trade_offer_tbl[3] = { lb_trade_offers_0, lb_trade_offers_1, lb_trade_offers_2 };

/* 0x803C8794 (0x150): the talk window's input: 2 skips the typing, 1 closes (or answers) the message, else 0. */
u8 npc_talk_input_step(NpcTalkWork* work)
{
    struct _PLW* plw = my_player_work_get();

    if (work->wait > 0) {
        work->wait--;
    } else if (work->done == 0) {
        if (pl_id_bit_ck(plw, 0x19) != 0 || pl_id_bit_ck(plw, 0x1A) != 0) {
            return 2;
        }
    } else if (work->kind == 1) {
        if (pl_id_bit_ck(plw, 0x1C) != 0) {
            work->choice = 0;
        } else if (pl_id_bit_ck(plw, 0x1B) != 0) {
            work->choice = 1;
        }
        if (pl_id_bit_ck(plw, 0x19) != 0) {
            return 1;
        }
        if (pl_id_bit_ck(plw, 0x1A) != 0) {
            if (work->choice == 1) {
                return 1;
            }
            work->choice = 1;
        }
    } else if (pl_id_bit_ck(plw, 0x19) != 0 || pl_id_bit_ck(plw, 0x1A) != 0) {
        return 1;
    }
    return 0;
}

/* 0x803C88E4 (0x78): opens the server-select screen (screen 0x1B) for `player`. */
void lb_server_select_open(struct _PLW* player)
{
    LbServerSelWork* work = lobby_w.server_sel_0x0AC;

    memset(work, 0, 0x2000);
    work->player = player;
    lobby_w.state_0x000 = 0x1B;
    lobby_w.active_0x008 = 1;
    sysSE_req(5);
}

/* 0x803C895C (0x4): closes the server-select screen. */
void lb_server_select_close(void)
{
    lb_panel_close();
}

/* 0x803C8960 (0x3C0): the server-select screen's step - the menu, the list refresh, and the server and room choice. */
void lb_server_select_screen_step(void)
{
    LbServerSelWork* work = lobby_w.server_sel_0x0AC;
    s32 i;
    NetMenuRow* row;

    if (work->list.anim_0x004 < 6) {
        work->list.anim_0x004++;
    }
    work->list.field_0x006 = 0;
    switch (work->list.mode_0x000) {
    case 0:
        work->list.mode_0x000 = 1;
        lb_list_init(&work->menu, 2, 0xFF, 0x25F, -1, 0, 0, 3, 0);
        break;
    case 1:
        switch (lb_choice_step(&work->menu)) {
        case 1:
            switch (work->menu.cursor_0x00) {
            case 0:
                if (request_list_refresh_c1(&work->list.result_0x002) == 0) {
                    sysSE_req(2);
                } else {
                    work->list.mode_0x000 = 2;
                    sysSE_req(0);
                    clear_qResult_work();
                }
                break;
            case 1:
                work->list.mode_0x000 = 3;
                work->list.step_0x001 = 0;
                work->list.cursor2_0x604 = 0;
                work->list.count_0x008 = 4;
                for (i = 0, row = work->list.rows_0x00C; i < work->list.count_0x008; row++, i++) {
                    NetCtrlWk::getServerTypeInfo(i + 1, (NetServerTypeInfo*)row);
                }
                work->list.page2_0x608 = 0;
                work->list.pages2_0x60A = 0;
                work->list.count2_0x606 = 0;
                sysSE_req(0);
                clear_qResult_work();
                break;
            }
            break;
        case 2:
            lb_server_select_close();
            break;
        }
        break;
    case 2:
        if (work->list.result_0x002 != 0) {
            if (work->list.result_0x002 < 0) {
                work->list.step_0x001 = 0x62;
            } else {
                lobby_w.field_0x0B0 = 3;
            }
        }
        break;
    case 3:
        switch (work->list.step_0x001) {
        case 0:
            if (NetCtrlWk::requestCommand08(&work->list.result_0x002) == 0) {
                work->list.step_0x001 = 0x63;
            } else {
                work->list.step_0x001++;
            }
            break;
        case 1:
            if (work->list.result_0x002 != 0) {
                if (work->list.result_0x002 < 0) {
                    work->list.step_0x001 = 0x63;
                } else {
                    lb_server_select_rooms_open(work);
                    work->list.step_0x001++;
                }
            }
            break;
        case 2:
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (request_command_0f_c1(&work->list.result_0x002,
                                          work->list.views2_0x60C[work->list.cursor2_0x604].id_0x08) == 0) {
                    work->list.step_0x001 = 0x62;
                    break;
                }
                work->list.step_0x001++;
                sysSE_req(0);
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                work->list.mode_0x000 = 1;
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                work->list.cursor2_0x604 =
                    menu_cursor_step(work->list.cursor2_0x604, work->list.count2_0x606, lb_cmd_repeat_get(), 1, 2);
            } else if (lb_cmd_repeat_ck(0xC) != 0) {
                work->list.page2_0x608 = menu_cursor_step_forward(work->list.page2_0x608, work->list.pages2_0x60A,
                                                                  lb_cmd_repeat_get(), 4, 8, 6,
                                                                  (u16*)&work->list.field_0x006);
            }
            lb_server_select_rooms_build(work, work->list.page2_0x608);
            break;
        case 3:
            if (work->list.result_0x002 != 0) {
                if (work->list.result_0x002 < 0) {
                    work->list.step_0x001 = 0x62;
                } else {
                    fade_set(0, 6);
                    lb_server_select_close();
                }
            }
            break;
        case 0x62:
            work->list.step_0x001 = 0;
            break;
        case 0x63:
            lb_server_select_close();
            break;
        }
        break;
    }
    if (work->list.mode_0x000 != 2) {
        subTransSet((u32)lb_server_select_draw, 0, 0);
    }
}

/* 0x803C8D20 (0x68): opens the room list of the picked server on its first page. */
void lb_server_select_rooms_open(LbServerSelWork* work)
{
    work->list.page2_0x608 = 0;
    work->list.pages2_0x60A = menu_page_count(work->list.views_0x4A4[work->list.cursor_0x49C].kind_0x08, 8);
    work->list.pages2_0x60A = NetCtrlWk::getRoomPageCount(8);
    lb_server_select_rooms_build(work, work->list.page2_0x608);
}

/* 0x803C8D88 (0x160): fills room page `page` of the picked server. */
void lb_server_select_rooms_build(LbServerSelWork* work, s16 page)
{
    s32 first;
    s32 i;
    s16 rooms;

    memset(work->list.views2_0x60C, 0, sizeof(work->list.views2_0x60C));
    work->list.count2_0x606 = 0;
    rooms = work->list.views_0x4A4[work->list.cursor_0x49C].kind_0x08;
    first = page * 8;
    for (i = 0; i < 8; i++) {
        if (first + i < rooms) {
            work->list.views2_0x60C[work->list.count2_0x606].id_0x08 = first + i;
            work->list.count2_0x606++;
        }
    }
    work->list.count2_0x606 = NetCtrlWk::copyRoomViews(work->list.views2_0x60C, first, 8);
    if (work->list.cursor2_0x604 >= work->list.count2_0x606) {
        work->list.cursor2_0x604 = work->list.count2_0x606 - 1;
    }
}

/* 0x803C8EE8 (0xAC): draws server type `index`'s description window. */
void lb_server_select_type_info_draw(LbServerSelWork* work, s16 index)
{
    s16 x = get_wide_offset(4) + 50;
    u8 text[0x19];

    draw_window_box(x, 0x50, 0x100, 0x80, 0, 0);
    font_set_size(18, 18);
    memset(text, 0, sizeof(text));
    font_print_ex(x + 20, 0x62, 0, (s8*)work->list.rows_0x00C[index].description_0x12);
}

/* 0x803C8F94 (0x1E0): draws the server type list with each type's server count. */
void lb_server_select_types_draw(LbServerSelWork* work, bool active)
{
    char text[0x20];
    bool picked;
    s32 color;
    s32 shade;
    s16 y;
    s16 x;
    s16 x2;
    s16 i;

    x = put_frame_dialog_title(0x168, 0x40, 0x104, 0xAA, 0, 0, 0, (s8*)LbStr(0, 0x262), 4) + 20;
    x2 = x + 110;
    put_frame_label(x, 0x54, 0x64, 0, 0, (s8*)LbStr(0, 0x26B));
    put_frame_label(x2, 0x54, 0x6E, 0, 0, (s8*)LbStr(0, 0x26C));
    x += 10;
    x2 += 78;
    y = 0x6C;
    font_set_size(18, 18);
    for (i = 0; i < work->list.count_0x008; i++) {
        put_frame_separator(x - 10, y, 0xDC, 0x1A, 0);
        if (i == work->list.cursor_0x00A) {
            picked = true;
            shade = 6;
            lb_server_select_type_info_draw(work, i);
            put_frame_cursor_bar(x - 10, y, 0xDC, 0x1A, 0, 0);
        } else {
            picked = false;
            shade = 2;
        }
        color = GetMenuFontColor(true, picked, active, false);
        lb_font_print(x, y + 3, (s8*)work->list.rows_0x00C[i].name_0x02, color, shade);
        sprintf(text, "%2d", work->list.rows_0x00C[i].id_0x00);
        lb_font_print(x2, y + 3, (s8*)text, color, shade);
        y += 0x1A;
    }
}

/* 0x803C9174 (0x44): draws the screen's title from lobby string `str`. */
void lb_server_select_title_draw(u16 str)
{
    put_frame_header(0, 0, 0, (s8*)LbStr(0, str), 0);
}

/* 0x803C91B8 (0xEC): draws the help line for the screen's current step. */
void lb_server_select_help_draw(LbServerSelWork* work)
{
    s8* text = NULL;
    s16 x = get_wide_offset(4) + 0x170;

    draw_window_frame_preset(x, 0x14A, 0);
    font_set_size(16, 16);
    switch (work->list.mode_0x000) {
    case 1:
        text = (s8*)LbStr(3, work->menu.cursor_0x00 + 0x1E2);
        break;
    case 2:
        text = (s8*)LbStr(3, 0x1E5);
        break;
    case 3:
        text = (s8*)LbStr(3, 0x1E6);
        break;
    }
    if (text != NULL) {
        font_print_ex(x + 24, 0x15A, 0, text);
    }
}

/* 0x803C92A4 (0x200): draws the server list of the picked type with each server's population. */
void lb_server_select_servers_draw(LbServerSelWork* work, bool active)
{
    s32 top;
    char text[0x20];
    bool picked;
    s32 color;
    s32 shade;
    s16 i;
    s16 x;
    s16 x2;
    s16 y;

    top = put_frame_dialog_title(0x12C, 0x40, 0x140, 0x10A, 0, 0, 0,
                                 (s8*)work->list.rows_0x00C[work->list.cursor_0x00A].name_0x02, 4);
    lb_page_arrow_put(top + 200, 0x36, work->list.page_0x4A0, work->list.pages_0x4A2, work->list.field_0x006, active);
    x = top + 20;
    x2 = x + 180;
    put_frame_label(x, 0x54, 0x78, 0, 0, (s8*)LbStr(0, 0x26D));
    put_frame_label(x2, 0x54, 0x64, 0, 0, (s8*)LbStr(0, 0x26E));
    x += 10;
    x2 += 8;
    y = 0x6C;
    font_set_size(18, 18);
    for (i = 0; i < work->list.count_0x49E; i++) {
        put_frame_separator(x - 10, y, 0x118, 0x1A, 0);
        if (i == work->list.cursor_0x49C) {
            picked = true;
            shade = 6;
            put_frame_cursor_bar(x - 10, y, 0x118, 0x1A, 0, 0);
        } else {
            picked = false;
            shade = 2;
        }
        color = GetMenuFontColor(true, picked, active, false);
        lb_font_print(x, y + 3, (s8*)work->list.views_0x4A4[i].name_0x0C, color, shade);
        sprintf(text, "%4d/%d", work->list.views_0x4A4[i].population_0x00, work->list.views_0x4A4[i].capacity_0x04);
        lb_font_print(x2, y + 3, (s8*)text, color, shade);
        y += 0x1A;
    }
}

/* 0x803C94A4 (0x21C): draws the room list of the picked server with each room's population. */
void lb_server_select_rooms_draw(LbServerSelWork* work, bool active)
{
    s32 top;
    char text[0x20];
    bool picked;
    s32 shade;
    s16 i;
    s16 x;
    s16 x2;
    s16 y;
    s32 color;
    char* name = work->list.views_0x4A4[work->list.cursor_0x49C].name_0x0C;

    name = *name != '\0' ? name : NetCtrlWk::getSelectedServerName();
    top = put_frame_dialog_title(0x12C, 0x40, 0x140, 0x10A, 0, 0, 0, (s8*)name, 4);
    lb_page_arrow_put(top + 200, 0x36, work->list.page2_0x608, work->list.pages2_0x60A, work->list.field_0x006, active);
    x = top + 20;
    x2 = x + 180;
    put_frame_label(x, 0x54, 0x78, 0, 0, (s8*)LbStr(0, 0x26F));
    put_frame_label(x2, 0x54, 0x64, 0, 0, (s8*)LbStr(0, 0x26E));
    x += 10;
    x2 += 8;
    y = 0x6C;
    font_set_size(18, 18);
    for (i = 0; i < work->list.count2_0x606; i++) {
        put_frame_separator(x - 10, y, 0x118, 0x1A, 0);
        if (i == work->list.cursor2_0x604) {
            picked = true;
            shade = 6;
            put_frame_cursor_bar(x - 10, y, 0x118, 0x1A, 0, 0);
        } else {
            picked = false;
            shade = 2;
        }
        color = GetMenuFontColor(true, picked, active, false);
        lb_font_print(x, y + 3, (s8*)work->list.views2_0x60C[i].name_0x0A, color, shade);
        sprintf(text, "%4d/%d", work->list.views2_0x60C[i].population_0x00, work->list.views2_0x60C[i].capacity_0x04);
        lb_font_print(x2, y + 3, (s8*)text, color, shade);
        y += 0x1A;
    }
}

/* 0x803C96C0 (0xB4): the server-select screen's draw callback. */
void lb_server_select_draw(void)
{
    LbServerSelWork* work = lobby_w.server_sel_0x0AC;

    switch (work->list.mode_0x000) {
    case 1:
        lb_server_select_title_draw(0x261);
        lb_list_dialog_draw(&work->menu, 0, 0, (s8*)LbStr(0, 0x268), 1);
        break;
    case 2:
        lb_server_select_title_draw(0x25F);
        lb_server_select_servers_draw(work, 1);
        break;
    case 3:
        lb_server_select_title_draw(0x263);
        lb_server_select_rooms_draw(work, 1);
        break;
    }
    lb_server_select_help_draw(work);
}

} /* extern "C" */

/* 0x803C9774 (0x108): draws the server-select transition panel of step `step` over `work`. */
/* untyped: caller-owned payload - the map's mangling (`lb_server_sel_trans__FPvUc`) is the retail `void*` */
void lb_server_sel_trans(void* payload, u8 step)
{
    s16 x;
    LbServerSelWork* work = (LbServerSelWork*)payload;

    switch (step) {
    case 0:
        lb_server_select_title_draw(0x262);
        lb_server_select_types_draw(work, 1);
        x = get_wide_offset(4) + 0x170;
        draw_window_frame_preset(x, 0x14A, 0);
        font_set_size(16, 16);
        font_print_ex(x + 24, 0x15A, 0, (s8*)LbStr(3, 0x1E4));
        break;
    case 1:
        lb_server_select_title_draw(0x25F);
        lb_server_select_servers_draw(work, 1);
        break;
    case 2:
        lb_server_select_title_draw(0x263);
        lb_server_select_rooms_draw(work, 1);
        break;
    }
    if (step != 0) {
        lb_server_select_help_draw(work);
    }
}

