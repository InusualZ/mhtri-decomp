/* lobby/fn_80219260.cpp - the lobby equipment page layer (the scroll list's staged sides and their detail panels),
 *   the guild-card award and title checks over the save block, the NPC talk layer (talk tables, talk pages, the
 *   screen an NPC opens) and the lobby target marker's scratch steps.
 * RANGE. .text 0x80219260-0x8021E1EC (76 functions, all written); .data 0x805B9DDC-0x805BA0C0, .bss
 *   0x806AA790-0x806AA8C8 (`lb_talk_state`), .sdata 0x80791EB0-0x80791EE0, .sdata2 0x80799C40-0x80799C60, extab,
 *   extabindex.  Every claimed section is emitted.
 * FLAGS. `cflags_lobby`; file-scope `#pragma peephole off` (retail keeps `extsb` + `cmpwi` in `lb_screen07_step` and
 *   `clrlwi` + `cmpwi` in `lb_scroll_gun_set_ok`) and `#pragma pool_data off` (retail addresses every table with its
 *   own `lis`/`addi`; pooled, `lb_equip_page_draw` read them off one base register).
 * NAMES. No `__FILE__` string covers the range and the dump answers `zz_` throughout, so the file keeps the map's
 *   stem; the range is one TU by its `.data` run (jump tables and tables in function order, no vtable) and its
 *   `.sdata2` pool (0.0f/1.0f/3.0f/6.0f and the two conversion doubles, read by the scratch steps and
 *   `lb_menu_angle_rank_get`).  Module `lobby`.
 *   GUESS (each from its body and its callers): every `lb_scroll_*`, `lb_equip_detail_*`, `lb_equip_cell_put`,
 *   `lb_screen_rect_fill`, `userdata_award_bit_ck`, `lb_award_*`, `lb_title_bit_set`, `lb_menu_kind_deco_ck`,
 *   `lb_menu_slot_set_ck`, `lb_npc_talk_*`, `lb_talk_*`, `lb_menu_scratch_reset`, `lb_menu_marker_step`,
 *   `lb_menu_row_insert`, `lb_menu_row16_copy`, `lb_cmd_repeat_ready_ck`, `lb_menu_angle_rank_get`,
 *   `lb_menu_actor_angle_get` and the data rows `lb_equip_bg_ids` ... `lb_talk_reward_equips`, `lb_talk_state`;
 *   `lb_title_range_find`/`lb_title_owned_ck` are menu_item_sub's names.
 * RESIDUALS.
 *  - `lb_award_hunt_bits_count` (89.6) and `lb_award_check_all` (93.6): retail hoists the bit loops' `li rN,2` ctr
 *    counts and the `1` shift base out of the loops into saved registers; ours rebuilds them per loop.
 *  - `lb_scroll_side_stage` (92.0): the `memcpy` targets' registers differ (472 B against 496 B).
 *  - `lb_equip_detail_blade_draw` (98.0): retail tests the 15/16 case range with `subi`+`cmplwi`, ours with two
 *    signed compares.  `lb_equip_detail_item_draw` (98.5): retail masks the ternary anchor (`clrlwi r7,r7,16`).
 *  - `lb_award_rows_apply` (98.3), `lb_menu_row_insert` (99.1), `lb_talk_reward_give` (99.1): register order only.
 *  - `lb_screen07_step` (98.8): the base and byte registers swap (r4/r3).
 *  - `lb_menu_angle_rank_get` (99.9): our frame is 0x40 against 0x30 (the conversion slots are not shared).
 *  - `.data`: our first table after `lb_equip_detail_draw`'s jump table is 8-aligned (+4 B); every later table
 *    follows it.  `.sdata` 45 B against 48 B (section tail).
 *  - `LbScrollWork` (lobby/lb_scroll_work.h) is also defined in `lobby/fn_80212810.cpp`, which should include it.
 *  - 15 registry names (`weapon_kind_slot_index`, `system_weapon_class_set`, `ai_npc_level_get`, `lb_screen05_open`,
 *    `lb_screen13_open`, `lb_menu_marker_update`, `lb_menu_marker_draw`, the `lb_award_*`/`lb_title_*_rows` tables,
 *    `lb_gun_set_frame_ids`) resolve once the registry lands.
 * SHAPES. Locals are declared in the order retail allocates their saved registers (first declared, highest);
 *   switch cases are written in the order retail lays out their bodies (single cases before a case range);
 *   `lb_talk_state` is defined at the foot so every use keeps its own `lis`/`addi`.
 */
#include "types.h"

#include "Pl/plw.h"
#include "Pl/fn_8027D684.h"            /* Gunner_opt_ok_ck, equip_kind_table_class */
#include "Pl/pl_item_page2_ck.h"
#include "Pl/pl_status_sheet_get.h"    /* weapon_class_index */
#include "Pl/weapon_kind_slot_index.h"
#include "ef/system_weapon_class_set.h"
#include "ef/eft052.h"                 /* hud_msg_push */
#include "hud/layout_types.h"
#include "hud/layout.h"                /* get_lsp_data, draw_sprite_*, drawshape_*, the _EQUIP accessors */
#include "drawshape_set_corner_colors.h"
#include "font/flfnt.h"                /* font_flush */
#include "main.h"                      /* ck_WideMode */
#include "menu/menu_infomation.h"
#include "menu/put_equip_panel_row.h"
#include "menu/put_equip_row_variant.h"
#include "menu/put_equip_row_variant2.h"
#include "menu/put_equip_row_variant_ex.h"
#include "menu/put_lsp_sprite_offset.h"
#include "lobby/lb_scroll_work.h"
#include "lobby/lb_scroll_rows_build.h"
#include "lobby/lb_item_cell_draw_layout3.h"
#include "lobby/lb_equip_cell_draw_layout3.h"
#include "lobby/lb_kakou_source_draw.h"
#include "lobby/lb_smith_deco_info_draw.h"
#include "lobby/lb_list_init.h"        /* lb_item_need_list_draw */
#include "lobby/LbPutAnaPageArrow.h"
#include "lobby/LbStr.h"
#include "lobby/spr_data_copy.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_npc.h"              /* lb_npc_talk_mode_set */
#include "lobby/lb_gun_set_frame_ids.h"
#include "lobby/lb_award_flag_run_tbl.h"
#include "lobby/lb_award_monster_ids.h"
#include "lobby/lb_award_progress_flags.h"
#include "lobby/lb_award_equip_kind_lists.h"
#include "lobby/lb_award_item_lists.h"
#include "lobby/lb_title_rank_rows.h"
#include "lobby/lb_title_hunt_rows.h"
#include "lobby/lb_menu_scratch.h"     /* `lb_menu_scratch`, owned by lobby/lb_menu_scratch.cpp (rule 2) */
#include "fn_80047398/lobby_world_block.h"
#include "fn_8004CAD8.h"               /* userdata_box_item_ck, monster_size_crown_get, userdata_progress_flag_ck */
#include "userdata_flag_ck.h"
#include "userdata_equip_flag_ck.h"
#include "equip_record_copy.h"
#include "equip_record_wear.h"
#include "sound/se_guide_voice_play.h" /* sysSE_window_open_play */
#include "ai/ai_npc_level_get.h"

#include "lobby/lb_unlock_cond_ck.h"   /* lb_unlock_cond_ck, kujira_event_over_ck */
#include "lobby/lb_menu_pos_tbl.h"     /* getItemListSelection */
#include "lobby/lb_unlock_cond_last_ck.h"
#include "lobby/LbCheckKujiraEvent.h"
#include "lobby/lb_shop_open.h"
#include "lobby/lb_eqshop_open.h"
#include "lobby/lb_goods_sale_open.h"
#include "lobby/lb_smith_open.h"
#include "lobby/lb_box_screen_open.h"
#include "lobby/lb_screen05_open.h"
#include "lobby/lb_screen13_open.h"
#include "lobby/lb_barter_open.h"
#include "lobby/lb_world_mode_get.h"
#include "lobby/lb_screen26_open.h"
#include "lobby/lb_quest_ui.h"             /* lb_kitchen_open, lb_trade_open */
#include "lobby/lb_quest_screen.h"         /* note_trade_open */
#include "lobby/lb_panel_close.h"
#include "lobby/lb_progress_unlock.h"
#include "lobby/lb_speaker_name_get.h"
#include "lobby/lb_cmd_repeat_ck.h"
#include "lobby/lb_menu_marker_update.h"
#include "lobby/lb_menu_marker_draw.h"
#include "menu/multi_result.h"             /* name_menu_open, pig_menu_open */
#include "menu/lb_quest_accept_screen_open.h"
#include "menu/lb_quest_join_screen_open.h"
#include "menu/lb_screen24_open.h"
#include "menu/lb_screen30_open.h"
#include "menu/lb_screen34_open.h"
#include "menu/lb_community_select_open.h"
#include "menu/menu_item.h"                /* ItemName, menu_busy_ck, game_ready_ck */
#include "menu/menu_pad_map_set.h"
#include "menu/get_option_pair.h"
#include "ef/menu_target_marker_set.h"
#include "menu/eft_em_model_ring_attach.h"
#include "camera/camera_lobby_npc_set.h"
#include "unsplit/lobby_area_view.h"       /* lb_area_view_pos_tbl */
#include "ef/eft052_page_put.h"
#include "ef/eft052_page_take.h"
#include "ef/get_move_work_adrs.h"
#include "ef/fn_800CDB2C.h"                /* my_player_no, ran_suu */
#include "ef/system_core.h"                /* rand_lcg_step */
#include "sound/fn_800D7F54.h"             /* sysSE_req, sysSE_stop */
#include "sound/bgm_lobby_return_req.h"
#include "hud/draw_help_window.h"
#include "hud/draw_spin_icon.h"
#include "font/font_print_typed.h"
#include "font/font_str_char_count.h"
#include "fn_80047398.h"                   /* subTransSet */
#include "userdata_item.h"                 /* userdata_equip_box_* */
#include "lobby/lb_server_sel_trans.h"     /* event_demo_ck */
#include "Pl/Pl_master_ck.h"               /* Pl_act_ck */
#include "MSL_C/printf.h"
#include "MSL_C/strcat.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* Call view (the `_c1` convention): retail passes the unlock stage of an unlock row as a halfword. */
#define lb_unlock_cond_last_ck_c1 ((u32 (*)(u16))lb_unlock_cond_last_ck)

/* The retail object keeps the unfused compare forms the peephole pass folds (`extsb` + `cmpwi`
 * in `lb_screen07_step`, `clrlwi` + `cmpwi` in `lb_scroll_gun_set_ok`), so the unit is built with the pass off. */
#pragma peephole off
#pragma pool_data off

/* The save block (`lobby_world_block`, 0x6000 B) as this unit reads it: the equipment records the page copies,
 * the hunt bits and counts the awards check, the award and title bit masks. */
typedef struct LbSaveView {
    /* +0x0000 */ u8 pad_0x0000[0x18];
    /* +0x0018 */ s32 points_0x18;             /* the points a talk reward adds (`score_add_clamped`) */
    /* +0x001C */ u8 pad_0x001C[4];
    /* +0x0020 */ _EQUIP body_0x20;            /* the worn body piece */
    /* +0x002C */ _EQUIP gun_parts_0x2C[2];    /* the worn bowgun barrel and stock */
    /* +0x0044 */ _EQUIP set_0x44[6];          /* the worn weapon and armour set */
    /* +0x008C */ u8 pad_0x008C[0x3960 - 0x8C];
    /* +0x3960 */ u8 hunt_bits_0x3960[0x40];   /* one bit per quest cleared (`lb_award_hunt_bits_count`) */
    /* +0x39A0 */ u8 pad_0x39A0[0x12];
    /* +0x39B2 */ u8 event_bits_0x39B2[8];     /* the event-quest bits award 19 counts */
    /* +0x39BA */ u8 pad_0x39BA[6];
    /* +0x39C0 */ u16 kill_count_0x39C0[0x80]; /* per monster: the kills */
    /* +0x3AC0 */ u16 capture_count_0x3AC0[0x80]; /* per monster: the captures */
    /* +0x3BC0 */ u16 size_0x3BC0[0x89][2];    /* per monster: the smallest and the largest size seen */
    /* +0x3DE4 */ u16 hunter_rank_0x3DE4;
    /* +0x3DE6 */ u8 pad_0x3DE6[0x3F38 - 0x3DE6];
    /* +0x3F38 */ u8 pad_0x3F38[2];
    /* +0x3F3A */ u8 grade_0x3F3A;             /* the three farm grades award 8 checks (stride 0xC) */
    /* +0x3F3B */ u8 pad_0x3F3B[0x3F46 - 0x3F3B];
    /* +0x3F46 */ u8 grade_0x3F46;
    /* +0x3F47 */ u8 pad_0x3F47[0x3F52 - 0x3F47];
    /* +0x3F52 */ u8 grade_0x3F52;
    /* +0x3F53 */ u8 pad_0x3F53[0x40F8 - 0x3F53];
    /* +0x40F8 */ u16 award_lo_0x40F8;         /* bits 0-11 of the guild card awards */
    /* +0x40FA */ u16 award_hi_0x40FA;         /* bits 12-23 */
    /* +0x40FC */ u8 pad_0x40FC[8];
    /* +0x4104 */ u32 title_bits_0x4104[4];    /* titles 0-117 */
    /* +0x4114 */ u8 pad_0x4114[0x4654 - 0x4114];
    /* +0x4654 */ u8 level_0x4654[4];          /* the four levels award 6 needs at 3 or more */
    /* +0x4658 */ u8 pad_0x4658[0x51A0 - 0x4658];
    /* +0x51A0 */ u16 companion_points_0x51A0; /* the companion level points award 10 reads */
    /* +0x51A2 */ u8 pad_0x51A2[0x52E4 - 0x51A2];
    /* +0x52E4 */ s32 record_a_0x52E4[10];     /* award 2: no slot may be empty in both */
    /* +0x530C */ s32 record_b_0x530C[10];
    /* +0x5334 */ u8 pad_0x5334[0x6000 - 0x5334];
} LbSaveView; /* size: 0x6000 */

/* A `{count, first flag}` run of `lb_award_flag_run_tbl`.  size: 0x4 */
typedef struct LbAwardFlagRun {
    /* +0x00 */ s16 count_0x00;  /* -1 ends the list */
    /* +0x02 */ u16 flag_0x02;
} LbAwardFlagRun;

/* A `lb_title_rank_rows` row.  size: 0x6 */
typedef struct LbTitleRankRow {
    /* +0x00 */ u16 rank_0x00;   /* 0xFFFF ends the table */
    /* +0x02 */ u16 title_0x02;
    /* +0x04 */ u16 count_0x04;
} LbTitleRankRow;

/* A `lb_title_hunt_rows` row.  size: 0x4 */
typedef struct LbTitleHuntRow {
    /* +0x00 */ u16 monster_0x00; /* 0xFFFF ends the table */
    /* +0x02 */ u16 title_0x02;   /* the first of the three hunt-count titles */
} LbTitleHuntRow;

/* A talk entry: the page links and the reward and progress flag of the page a talk opened on.  size: 0xA */
typedef struct LbTalkEntry {
    /* +0x00 */ u8 next_yes_0x00;  /* the text offset "yes" (or the A button) leads to, 0 to end */
    /* +0x01 */ u8 next_no_0x01;   /* the text offset "no" leads to, 0 to end */
    /* +0x02 */ u8 cond_0x02;      /* 1 for a conditional row's entry: B cannot cancel it */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u16 value_0x04;    /* the reward row (0 for none) */
    /* +0x06 */ u16 page_0x06;     /* the first text of the page */
    /* +0x08 */ u16 flag_0x08;     /* the progress flag the talk unlocks */
} LbTalkEntry;

/* A conditional talk row of an NPC's talk data.  size: 0x10 */
typedef struct LbTalkCondRow {
    /* +0x00 */ u8 flags_0x00;     /* bit 2: the NPC shows the new-talk mark */
    /* +0x01 */ u8 next_yes_0x01;
    /* +0x02 */ u8 next_no_0x02;
    /* +0x03 */ u8 cond_0x03;      /* how the two progress flags combine (0-5) */
    /* +0x04 */ u16 flag_a_0x04;
    /* +0x06 */ u16 flag_b_0x06;
    /* +0x08 */ u16 done_flag_0x08; /* set once the talk was had */
    /* +0x0A */ u16 unlock_0x0A;   /* the unlock stage it needs, 0xFF for none */
    /* +0x0C */ u16 value_0x0C;
    /* +0x0E */ u16 page_0x0E;     /* 0xFFFF ends the table */
} LbTalkCondRow;

/* An unlock talk row.  size: 0x8 */
typedef struct LbTalkUnlockRow {
    /* +0x00 */ u8 mask_0x00;      /* the condition mask `lb_talk_cond_mask_ck` tests */
    /* +0x01 */ u8 unlock_0x01;    /* 0xFF for none */
    /* +0x02 */ u8 next_yes_0x02;
    /* +0x03 */ u8 next_no_0x03;
    /* +0x04 */ u16 value_0x04;
    /* +0x06 */ u16 page_0x06;     /* 0xFFFF ends the table */
} LbTalkUnlockRow;

/* A random talk row.  size: 0xC */
typedef struct LbTalkRandRow {
    /* +0x00 */ u32 mask_0x00;
    /* +0x04 */ u8 next_yes_0x04;
    /* +0x05 */ u8 next_no_0x05;
    /* +0x06 */ u16 value_0x06;
    /* +0x08 */ u16 page_0x08;     /* 0xFFFF ends the table */
    /* +0x0A */ u8 pad_0x0A[2];
} LbTalkRandRow;

/* A talk page.  size: 0x8 */
typedef struct LbTalkPage {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 next_yes_0x01;
    /* +0x02 */ u8 next_no_0x02;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u16 value_0x04;
    /* +0x06 */ u16 text_0x06;
} LbTalkPage;

/* A talk reward: points, a pouch item or an equipment piece.  size: 0x6 */
typedef struct LbTalkReward {
    /* +0x00 */ u8 kind_0x00;      /* 1 points, 2 item, 3 equipment */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 id_0x02;       /* the points, the item, or the equipment kind */
    /* +0x04 */ u16 count_0x04;    /* the item count or the equipment id */
} LbTalkReward;

/* A talk text.  size: 0x8 */
typedef struct LbTalkText {
    /* +0x00 */ u8 type_0x00;      /* 1/2 a yes/no question, 3 continues with the next text */
    /* +0x01 */ u8 kind_0x01;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ char* text_0x04;
} LbTalkText;

/* A word of a tag's word list.  size: 0x8 */
typedef struct LbTalkWord {
    /* +0x00 */ u32 mask_0x00;
    /* +0x04 */ char* text_0x04;
} LbTalkWord;

/* The word list one `~S0n` tag draws from.  size: 0x8 */
typedef struct LbTalkWordList {
    /* +0x00 */ LbTalkWord* words_0x00;
    /* +0x04 */ s16 count_0x04;
    /* +0x06 */ u8 pad_0x06[2];
} LbTalkWordList;

/* One NPC kind's talk data (`lobby_w.talk_tbl_0x0C0[kind]`).  size: 0x1C */
typedef struct LbNpcTalkData {
    /* +0x00 */ LbTalkCondRow* cond_rows_0x00;
    /* +0x04 */ LbTalkUnlockRow* unlock_rows_0x04;
    /* +0x08 */ LbTalkRandRow* rand_rows_0x08;
    /* +0x0C */ LbTalkPage* pages_0x0C;
    /* +0x10 */ LbTalkReward* rewards_0x10;
    /* +0x14 */ LbTalkText* texts_0x14;
    /* +0x18 */ LbTalkWordList* words_0x18;  /* the four `~S00`-`~S03` lists */
} LbNpcTalkData;

/* The talk state (.bss 0x806AA790): the talking player and NPC, the page's entry and its typed text.  size: 0x138 */
typedef struct LbTalkState {
    /* +0x000 */ _PLW* player_0x00;
    /* +0x004 */ _LB_NPC* npc_0x04;
    /* +0x008 */ u8 state_0x08;    /* `lb_screen07_step`'s tick state */
    /* +0x009 */ u8 flags_0x09;    /* bit 0: the talk is a special one, bit 1: the text is fully shown */
    /* +0x00A */ u8 text_kind_0x0A;
    /* +0x00B */ u8 pad_0x0B;
    /* +0x00C */ s32 text_type_0x0C;
    /* +0x010 */ s32 choice_0x10;  /* the yes/no cursor: 0 yes, 1 no */
    /* +0x014 */ u32 value_0x14;   /* the chosen value `lb_talk_page_value_get` returns */
    /* +0x018 */ s32 text_id_0x18;
    /* +0x01C */ s32 typed_0x1C;   /* the typing steps so far */
    /* +0x020 */ s32 shown_0x20;   /* the characters shown */
    /* +0x024 */ LbTalkEntry entry_0x24;
    /* +0x02E */ u8 pad_0x2E[2];
    /* +0x030 */ LbNpcTalkData* talk_0x30;
    /* +0x034 */ char text_0x34[0x100];
    /* +0x134 */ u8 pad_0x134[4];
} LbTalkState;

extern "C" {

extern LbTalkState lb_talk_state;

void lb_npc_talk_select(_LB_NPC* npc);
void lb_npc_talk_screen_open(void);
void lb_talk_text_set(u32 id);
void lb_talk_tag_replace(LbTalkWordList* list, char* text, const char* tag);
s32 lb_talk_reward_give(LbTalkState* state);
void lb_npc_talk_reply(_LB_NPC* npc, s32 flag, u32 page);
s32 lb_menu_step(u8 active, s32 se_close, s32 se_yes, s32 se_no);
s32 lb_talk_page_mode_set(s32 mode);
s32 lb_talk_page_mode_reset(void);
void lb_talk_page_draw(void);
void lb_menu_row_marks_clear(void);
void lb_menu_scratch_init(s16 full);
void lb_menu_row16_copy(LbMenuRow* dst, const LbMenuRow* src);

/* The sub-frame sprite row `lb_scroll_sub_frame_draw` draws for side 0. */
u16 lb_scroll_sub_frame_ids[4] = { 0x19AA, 0x19AB, 0x19AC, 0xFFFF };

void lb_equip_cell_put(u16 id, _EQUIP* equip, _EQUIP* part, _mh_ivec2_* ofs);

/* Returns the player's equipment record for `kind`: the weapon slots 1-6, the body record for 7-11/14/15, and the
 * gunner parts for 12/13 when the player has them. */
_EQUIP* lb_scroll_equip_rec_get(LbScrollWork* self, u8 kind) {
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
        return &self->owner_0x34->equipB;
    case 12:
        if (pl_item_page2_ck(self->owner_0x34) == 1) {
            return &self->owner_0x34->equipC;
        }
        return &self->owner_0x34->equipB;
    case 13:
        if (pl_item_page2_ck(self->owner_0x34) == 1) {
            return &self->owner_0x34->equipD;
        }
        return &self->owner_0x34->equipB;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return &self->owner_0x34->equipA[weapon_kind_slot_index(kind)];
    default:
        return 0;
    }
}

/* Stages `src` on side `page` of the scroll list: its triple of records is filled from the source record, from
 * the player's own set, or from a pair of empty gunner parts. */
void lb_scroll_side_stage(LbScrollWork* self, _EQUIP* src, u8 page) {
    u8 kind = self->kind_0x00;

    _EQUIP* a;
    _EQUIP* b;
    _EQUIP* c;
    _PLW* plw = self->owner_0x34;

    if (page == 0) {
        a = &self->base_0x5C[0];
        b = &self->base_0x5C[1];
        c = &self->base_0x5C[2];
    } else {
        a = &self->cmp_0x38[0];
        b = &self->cmp_0x38[1];
        c = &self->cmp_0x38[2];
    }

    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
    case 10:
        memcpy(a, src, 12);
        break;
    default:
        if (pl_item_page2_ck(plw) == 1) {
            switch (src->kind) {
            case 11:
                memcpy(a, src, 12);
                if (Gunner_opt_ok_ck(src) == 0) {
                    memset(b, 0, 12);
                    memset(c, 0, 12);
                    b->kind = 12;
                    b->item_id = src->item_id;
                    c->kind = 13;
                    c->item_id = src->item_id;
                } else {
                    memcpy(b, &plw->equipC, 12);
                    memcpy(c, &plw->equipD, 12);
                }
                break;
            case 12:
                memcpy(a, &plw->equipB, 12);
                memcpy(b, src, 12);
                memcpy(c, &plw->equipD, 12);
                break;
            case 13:
                memcpy(a, &plw->equipB, 12);
                memcpy(b, &plw->equipC, 12);
                memcpy(c, src, 12);
                break;
            default:
                memcpy(a, src, 12);
                break;
            }
        } else {
            memcpy(a, src, 12);
        }
        break;
    }
}

/* Stages the player's record for `src`'s kind on the compared side and rebuilds the list, resetting the cursor
 * when it ran past the count. */
void lb_equip_compare_set(LbScrollWork* self, _EQUIP* src) {
    _EQUIP* equip = lb_scroll_equip_rec_get(self, src->kind);

    lb_scroll_side_stage(self, equip, 1);
    lb_scroll_rows_build(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

/* Stages `src` on the base side and rebuilds the list, resetting the cursor when it ran past the count. */
void lb_equip_base_set(LbScrollWork* self, _EQUIP* src) {
    lb_scroll_side_stage(self, src, 0);
    lb_scroll_rows_build(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

/* Draws the detail panel's sub-frame: side 0 under the plain frame row, side 1 under the menu table's. */
void lb_scroll_sub_frame_draw(u8 page) {
    _mh_ivec2_ pos;

    if (page == 0) {
        get_lsp_data(0x19A9, &pos);
        draw_sprite_ary(lb_scroll_sub_frame_ids, &pos);
    } else {
        get_lsp_data(0x1C3D, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(137), &pos);
    }
}

/* Whether side `page`'s staged records can be shown: 0 only while a compared bowgun set is staged and the side is
 * not a complete frame, barrel and stock. */
u32 lb_scroll_gun_set_ok(LbScrollWork* self, u8 page, u8 group) {
    _EQUIP* frame;
    _EQUIP* barrel;
    _EQUIP* stock;

    if (page == 1) {
        frame = &self->cmp_0x38[0];
        barrel = &self->cmp_0x38[1];
        stock = &self->cmp_0x38[2];
    } else {
        frame = &self->base_0x5C[0];
        barrel = &self->base_0x5C[1];
        stock = &self->base_0x5C[2];
    }

    if ((self->flags_0x10 & 3) != 0 && (u8)(self->kind_0x00 - 5) <= 1 && group == 2) {
        if (frame->kind != 11 || barrel->kind != 12 || stock->kind != 13 || frame->item_id == 0
            || barrel->item_id == 0 || stock->item_id == 0) {
            return 0;
        }
    }
    return 1;
}

/* Draws side `page`'s detail panel `mode` (page `sub` of it) at layout sprite `anchor`. */
void lb_equip_detail_draw(LbScrollWork* self, u8 page, u8 mode, u8 sub, u16 anchor, u8 flags) {
    _EQUIP* a;
    _EQUIP* b;
    _EQUIP* c;
    _EQUIP* other = NULL;
    _EQUIP* other_b = NULL;
    _EQUIP* other_c = NULL;
    s32 compare = 0;
    _mh_ivec2_ pos;

    if (page == 1) {
        a = &self->cmp_0x38[0];
        b = &self->cmp_0x38[1];
        c = &self->cmp_0x38[2];
    } else {
        a = &self->base_0x5C[0];
        b = &self->base_0x5C[1];
        c = &self->base_0x5C[2];
    }
    if (self->kind_0x00 == 2 || self->kind_0x00 == 6) {
        compare = 1;
        other = &self->cmp_0x38[0];
        other_b = &self->cmp_0x38[1];
        other_c = &self->cmp_0x38[2];
    }

    if (lb_scroll_gun_set_ok(self, page, sub) == 0) {
        if (self->kind_0x00 == 6 && page == 0 && Gunner_opt_ok_ck(a) == 1 && Gunner_opt_ok_ck(other) == 0) {
            ammo_kind_label_draw(anchor, 14);
            return;
        }
        if ((self->flags_0x10 & 2) != 0) {
            ammo_kind_label_draw(anchor, 15);
            return;
        }
        if (a->kind != 11 || a->item_id == 0) {
            ammo_kind_label_draw(anchor, 11);
        }
        if (b->kind != 12 || b->item_id == 0) {
            ammo_kind_label_draw(anchor, 12);
        }
        if (c->kind != 13 || c->item_id == 0) {
            ammo_kind_label_draw(anchor, 13);
        }
        return;
    }

    switch (mode) {
    case 1:
        switch (sub) {
        case 1:
            if (compare == 0) {
                equip_detail_melee_draw(self->owner_0x34, a, anchor, flags);
                return;
            }
            blade_basis_page_draw(self->owner_0x34, a, other, anchor, flags);
            return;
        case 2:
            if (self->paged_0x03 == 0) {
                if (compare == 0) {
                    equip_detail_bowgun_draw(self->owner_0x34, a, anchor, flags);
                    return;
                }
                gun_basis_entry(self->owner_0x34, a, other, anchor, flags);
                return;
            }
            if (compare == 0) {
                put_equip_row_variant(self->owner_0x34, a, b, c, anchor, flags);
                return;
            }
            if ((self->flags_0x10 & 3) != 0) {
                gun_basis_entry_pair(self->owner_0x34, a, b, c, other, other_b, other_c, anchor, flags);
                return;
            }
            put_equip_row_variant(self->owner_0x34, a, b, c, anchor, flags);
            return;
        case 3:
            if (compare == 0) {
                armor_stats_entry_zero(self->owner_0x34, a, anchor, flags);
                return;
            }
            armor_stats_page_draw(self->owner_0x34, a, other, anchor, flags);
            return;
        case 4:
            if (compare == 0) {
                equip_detail_kind6_draw(self->owner_0x34, a, anchor, flags);
                return;
            }
            deco_skills_page_draw(self->owner_0x34, a, other, anchor, flags);
            return;
        }
        break;
    case 2:
        if (compare == 0) {
            equip_detail_slots_draw(self->owner_0x34, a, anchor, flags);
            return;
        }
        weapon_desc_entry(self->owner_0x34, a, other, anchor, flags);
        return;
    case 17:
        ammo_kind_label7_draw(anchor);
        return;
    case 18:
        ammo_kind_label6_draw(anchor);
        return;
    case 3:
        put_lsp_sprite_offset(anchor);
        return;
    case 4:
        switch (sub) {
        case 1:
            if (compare == 0) {
                armor_skills_entry_zero(self->owner_0x34, a, anchor, flags);
                return;
            }
            armor_skills_entry(self->owner_0x34, a, other, anchor, flags);
            return;
        case 2:
            if (self->paged_0x03 == 0) {
                if (compare == 0) {
                    armor_skills_entry_zero(self->owner_0x34, a, anchor, flags);
                    return;
                }
                armor_skills_entry(self->owner_0x34, a, other, anchor, flags);
                return;
            }
            if (compare == 0) {
                put_equip_row_variant_ex(self->owner_0x34, a, b, c, anchor, flags);
                return;
            }
            if ((self->flags_0x10 & 3) != 0) {
                armor_skills_entry_pair(self->owner_0x34, a, b, c, other, other_b, other_c, anchor, flags);
                return;
            }
            put_equip_row_variant_ex(self->owner_0x34, a, b, c, anchor, flags);
            return;
        case 3:
        case 4:
            if (compare == 0) {
                weapon_skills_entry_zero(self->owner_0x34, a, anchor, flags);
                return;
            }
            weapon_skills_page_draw(self->owner_0x34, a, other, anchor, flags);
            return;
        }
        break;
    case 5:
        switch (sub) {
        case 2:
            if (self->paged_0x03 == 0) {
                if (compare == 0) {
                    deco_page_entry_zero(self->owner_0x34, a, anchor, flags);
                    return;
                }
                deco_page_entry(self->owner_0x34, a, other, anchor, flags);
                return;
            }
            if (compare == 0) {
                put_equip_row_variant2(self->owner_0x34, a, b, c, anchor, flags);
                return;
            }
            if ((self->flags_0x10 & 3) != 0) {
                deco_page_entry_pair(self->owner_0x34, a, b, c, other, other_b, other_c, anchor, flags);
                return;
            }
            put_equip_row_variant2(self->owner_0x34, a, b, c, anchor, flags);
            return;
        case 1:
        case 3:
        case 4:
            if (compare == 0) {
                deco_page_entry_zero(self->owner_0x34, a, anchor, flags);
                return;
            }
            deco_page_entry(self->owner_0x34, a, other, anchor, flags);
            return;
        }
        break;
    case 6: {
        s32 skill_flags = ((flags & 1) != 0) + 4;

        switch (sub) {
        case 2:
            if (self->paged_0x03 == 0) {
                if (compare == 0) {
                    active_skills_entry_zero(self->owner_0x34, a, anchor, skill_flags);
                    return;
                }
                active_skills_entry(self->owner_0x34, a, other, anchor, skill_flags);
                return;
            }
            active_skills_entry_ex(self->owner_0x34, a, b, c, anchor, skill_flags);
            return;
        case 1:
        case 3:
        case 4:
            if (compare == 0) {
                active_skills_entry_zero(self->owner_0x34, a, anchor, skill_flags);
                return;
            }
            active_skills_entry(self->owner_0x34, a, other, anchor, skill_flags);
            return;
        }
        break;
    }
    case 7:
    case 8:
    case 9: {
        u8 grid;

        if (mode == 7) {
            grid = 2;
        } else {
            grid = mode == 8;
        }
        switch (sub) {
        case 2:
            if (self->paged_0x03 == 0) {
                if (compare == 0) {
                    ammo_panel_entry_zero(self->owner_0x34, a, anchor, flags, grid);
                    return;
                }
                ammo_panel_entry(self->owner_0x34, a, other, anchor, flags, grid);
                return;
            }
            if (compare == 0) {
                put_equip_panel_row_variant(self->owner_0x34, a, b, c, anchor, flags, grid);
                return;
            }
            if ((self->flags_0x10 & 3) != 0) {
                ammo_panel_entry_pair(self->owner_0x34, a, b, c, other, other_b, other_c, anchor, flags, grid);
                return;
            }
            put_equip_panel_row_variant(self->owner_0x34, a, b, c, anchor, flags, grid);
            return;
        case 1:
        case 3:
        case 4:
            if (compare == 0) {
                ammo_panel_entry_zero(self->owner_0x34, a, anchor, flags, grid);
                return;
            }
            ammo_panel_entry(self->owner_0x34, a, other, anchor, flags, grid);
            return;
        }
        break;
    }
    case 10:
        if (self->paged_0x03 == 0) {
            put_equip_panel_row_zero(self->owner_0x34, a, anchor, flags);
            return;
        }
        put_equip_panel_row(self->owner_0x34, a, b, c, anchor, flags);
        return;
    case 11: {
        u8 tab;

        switch (self->flag_0x01) {
        case 11:
            tab = 0;
            break;
        case 12:
            tab = 1;
            break;
        case 13:
            tab = 2;
            break;
        default:
            tab = 0xFF;
            break;
        }
        put_equip_panel_kind4(self->owner_0x34, a, b, c, anchor, flags, tab);
        return;
    }
    case 12:
    case 13:
        put_equip_panel_kind5(self->owner_0x34, a, b, c, anchor, flags, mode == 13);
        return;
    case 14:
        lb_smith_deco_info_draw(self->owner_0x34, self->value_0x0E);
        return;
    case 15:
        if (self->need_items_0x30 != NULL) {
            u8 need_mode;
            u16 need_a;
            u16 need_b;

            switch ((u8)equip_kind_table_class(a->kind)) {
            case 1:
            case 2:
                need_mode = 1;
                need_a = a->kind;
                need_b = a->item_id;
                break;
            case 0:
                need_mode = 2;
                need_a = a->kind;
                need_b = a->item_id;
                break;
            default:
                need_mode = 0;
                need_a = self->value_0x0E;
                need_b = 0;
                break;
            }
            get_lsp_data(anchor, &pos);
            lb_item_need_list_draw(self->need_items_0x30, self->value_0x0C, 0, &pos, need_mode, need_a, need_b);
        }
        return;
    case 16:
        get_lsp_data(anchor, &pos);
        lb_kakou_source_draw(self->kakou_src_0x2C, a->kind, self->kakou_state_0x28, a, &pos);
        break;
    }
}

/* Draws the two detail panels of a blade or gunner list (kinds 1/5) and, for a lone base side, its cell. */
void lb_equip_detail_blade_draw(LbScrollWork* self, _mh_ivec2_* ofs) {
    u16 anchor;
    s32 shown;

    if (lb_scroll_gun_set_ok(self, 0, self->left_group_0x11) == 1) {
        if ((u32)(self->left_rows_0x13[self->cursor_0x08] - 11) <= 2) {
            shown = 1;
            anchor = 0x19B4;
        } else {
            shown = 0;
            anchor = 0x19B1;
        }
    } else {
        anchor = 0x19B1;
    }
    lb_equip_detail_draw(self, 0, self->left_rows_0x13[self->cursor_0x08], self->left_group_0x11, anchor,
                         self->left_mode_0x04);
    if (lb_scroll_gun_set_ok(self, 0, self->left_group_0x11) == 1) {
        switch ((s32)self->right_rows_0x1D[self->cursor_0x08]) {
        case 15:
        case 16:
            anchor = 0x1A7E;
            break;
        case 0:
            break;
        default:
            shown = 0;
            anchor = 0x19B2;
            break;
        }
        lb_equip_detail_draw(self, 0, self->right_rows_0x1D[self->cursor_0x08], self->right_group_0x12, anchor,
                             self->right_mode_0x05);
        if (self->right_rows_0x1D[self->cursor_0x08] != 0) {
            if (self->mode_0x02 == 3 || self->mode_0x02 == 7) {
                lb_scroll_sub_frame_draw(1);
            } else {
                lb_scroll_sub_frame_draw(0);
            }
        }
        if (shown == 0) {
            if (self->kind_0x00 == 5) {
                if (self->right_group_0x12 == 2) {
                    lb_equip_cell_put(0x19A8, &self->base_0x5C[0], &self->base_0x5C[1], ofs);
                } else {
                    lb_equip_cell_put(0x19A8, &self->base_0x5C[0], NULL, ofs);
                }
            } else {
                lb_equip_cell_put(0x19A8, &self->base_0x5C[0], NULL, ofs);
            }
        }
    }
}

/* Draws the compared and the base side's detail panels and cells of a compare list (kinds 2/6). */
void lb_equip_detail_compare_draw(LbScrollWork* self, _mh_ivec2_* ofs) {
    lb_equip_detail_draw(self, 1, self->left_rows_0x13[self->cursor_0x08], self->left_group_0x11, 0x19CF,
                         self->left_mode_0x04);
    lb_equip_detail_draw(self, 0, self->right_rows_0x1D[self->cursor_0x08], self->right_group_0x12, 0x19D0,
                         self->right_mode_0x05);
    if (self->kind_0x00 == 6) {
        if (self->left_group_0x11 == 2) {
            lb_equip_cell_put(0x19CD, &self->cmp_0x38[0], &self->cmp_0x38[1], ofs);
        } else {
            lb_equip_cell_put(0x19CD, &self->cmp_0x38[0], NULL, ofs);
        }
        if (self->right_group_0x12 == 2) {
            lb_equip_cell_put(0x19CE, &self->base_0x5C[0], &self->base_0x5C[1], ofs);
        } else {
            lb_equip_cell_put(0x19CE, &self->base_0x5C[0], NULL, ofs);
        }
    } else {
        lb_equip_cell_put(0x19CD, &self->cmp_0x38[0], NULL, ofs);
        lb_equip_cell_put(0x19CE, &self->base_0x5C[0], NULL, ofs);
    }
}

/* Draws an item list's two detail panels (kind 8) and the item's cell. */
void lb_equip_detail_item_draw(LbScrollWork* self, _mh_ivec2_* ofs) {
    _mh_ivec2_ pos;
    u16 anchor;

    lb_equip_detail_draw(self, 0, self->left_rows_0x13[self->cursor_0x08], self->left_group_0x11, 0x19B1,
                         self->left_mode_0x04);
    anchor = (s32)self->right_rows_0x1D[self->cursor_0x08] == 15 ? 0x1A7E : 0x19B2;
    lb_equip_detail_draw(self, 0, self->right_rows_0x1D[self->cursor_0x08], self->right_group_0x12, anchor,
                         self->right_mode_0x05);
    if (self->right_rows_0x1D[self->cursor_0x08] != 0) {
        if (self->mode_0x02 == 3 || self->mode_0x02 == 7) {
            lb_scroll_sub_frame_draw(1);
        } else {
            lb_scroll_sub_frame_draw(0);
        }
    }
    get_lsp_data(0x19CD, &pos);
    pos.x += ofs->x;
    pos.y += ofs->y;
    lb_item_cell_draw_layout3(self->value_0x0E, 0, &pos, 1, 0, 1, 0x80);
}

/* Draws the equipment cell of `equip` at layout sprite `id` shifted by `ofs`, with the bowgun icon when the barrel
 * `part` is given. */
void lb_equip_cell_put(u16 id, _EQUIP* equip, _EQUIP* part, _mh_ivec2_* ofs) {
    _mh_ivec2_ pos;
    u8 type;

    get_lsp_data(id, &pos);
    pos.x += ofs->x;
    pos.y += ofs->y;
    if (part == NULL) {
        lb_equip_cell_draw_layout3(equip, 0, &pos, 1, 0, 1, 0, 0);
        return;
    }
    lb_equip_cell_draw_layout3(equip, 0, &pos, 1, 0, 1, 8, 0);
    type = Get_pl_type(equip, part);
    draw_weaponicon_idx(0x13D, type, get_rare_color(Get_equip_rare(equip)), &pos);
}

/* The background sprite rows of the equipment screens, normal and wide. */
u16 lb_equip_bg_ids[8] = { 0x19F3, 0x19EF, 0x19F0, 0x19F1, 0x19F2, 0xFFFF, 0, 0 };
u16 lb_equip_bg_wide_ids[8] = { 0x19FB, 0x19F5, 0x19F6, 0x19F7, 0x19F8, 0x19F9, 0x19FA, 0xFFFF };

/* Draws the equipment screens' background. */
void lb_equip_bg_draw(void) {
    _mh_ivec2_ pos;

    font_flush();
    if (ck_WideMode() != 0) {
        get_lsp_data(0x19F4, &pos);
        draw_sprite_ary(lb_equip_bg_wide_ids, &pos);
        return;
    }
    get_lsp_data(0x19EE, &pos);
    draw_sprite_ary(lb_equip_bg_ids, &pos);
}

/* Returns the lobby screen window's rectangle: its anchor plus the frame's offset, and the frame's size. */
void lb_screen_rect_get(s16* x, s16* y, s16* w, s16* h) {
    _mh_ivec2_ pos;
    _SPR_DATA_ frame;

    get_lsp_data(0x19DE, &pos);
    spr_data_copy(&frame, get_lsp_data(0x19E0, NULL));
    *x = pos.x + frame.pos.x;
    *y = pos.y + frame.pos.y;
    *w = frame.width;
    *h = frame.height;
}

/* The page frames' sprite rows and the screen window's corner colours. */
u16 lb_screen_frame_ids[12] = { 0x19EB, 0x19EC, 0x19E3, 0x19E4, 0x19E5, 0x19E6, 0x19E7, 0x19E8, 0x19E9, 0x19EA, 0xFFFF, 0 };
u16 lb_equip_compare_frame_ids[22] = { 0x19C8, 0x19C9, 0x19CA, 0x19CB, 0x19B7, 0x19B8, 0x19B9, 0x19BA, 0x19BB, 0x19BC, 0x19BD,
                         0x19BE, 0x19BF, 0x19C0, 0x19C1, 0x19C2, 0x19C3, 0x19C4, 0x19C5, 0x19C6, 0x19C7, 0xFFFF };
u16 lb_equip_detail_frame_ids[14] = { 0x19A6, 0x19A7, 0x199F, 0x19A0, 0x19A1, 0x19A2, 0x19A3, 0x19A4, 0x199D, 0x19A5, 0xFFFF, 0, 0,
                         0 };
u32 lb_screen_rect_colors[4] = { 0x2E3A38FF, 0x2C2C2BFF, 0x28221FFF, 0x252927FF };

/* Fills the lobby screen window's rectangle with its four corner colours. */
void lb_screen_rect_fill(void) {
    s16 x;
    s16 y;
    s16 w;
    s16 h;

    lb_screen_rect_get(&x, &y, &w, &h);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x, y, w, h);
    drawshape_set_corner_colors(lb_screen_rect_colors[0], lb_screen_rect_colors[1], lb_screen_rect_colors[2], lb_screen_rect_colors[3]);
    drawshape_exec();
}

/* The detail page's frame sprite row for the list kinds 3/9. */
u16 lb_screen_frame_row_ids[4] = { 0x19DF, 0xFFFF, 0, 0 };

/* Draws the scroll list's page: the kind's frame, its detail panels and the page arrows. */
void lb_equip_page_draw(LbScrollWork* self, bool active) {
    _mh_ivec2_ pos;
    _mh_ivec2_ pos2;
    u8 kind = self->kind_0x00;

    if (kind == 0) {
        return;
    }
    switch (kind) {
    case 1:
    case 5:
        if (self->mode_0x02 == 3 || self->mode_0x02 == 7) {
            get_lsp_data(0x1B8E, &pos);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(136), &pos);
        } else {
            get_lsp_data(0x199E, &pos);
            draw_sprite_ary(lb_equip_detail_frame_ids, &pos);
        }
        break;
    case 2:
    case 6:
        if (self->mode_0x02 == 3 || self->mode_0x02 == 7) {
            get_lsp_data(0x1B66, &pos);
            draw_sprite_ary(lb_gun_set_frame_ids, &pos);
        } else {
            get_lsp_data(0x19B5, &pos);
            draw_sprite_ary(lb_equip_compare_frame_ids, &pos);
        }
        break;
    case 3:
    case 9:
        if (kind != 9) {
            get_lsp_data(0x19E2, &pos);
            draw_sprite_ary(lb_screen_frame_ids, &pos);
            get_lsp_data(0x19DE, &pos2);
            draw_sprite_ary(lb_screen_frame_row_ids, &pos2);
            lb_screen_rect_fill();
        }
        break;
    case 8:
        get_lsp_data(0x199E, &pos);
        if (self->right_rows_0x1D[self->cursor_0x08] == 0) {
            draw_sprite_anim_ary(lb_equip_detail_frame_ids, 1, &pos);
        } else {
            draw_sprite_ary(lb_equip_detail_frame_ids, &pos);
        }
        break;
    case 4:
        equip_page_count_step(self->owner_0x34, &self->base_0x5C[0], 0, self->cursor_0x08);
        break;
    case 10:
        skill_table_page_draw(self->owner_0x34, &self->base_0x5C[0], &self->base_0x5C[1], &self->base_0x5C[2], 0,
                              self->cursor_0x08);
        break;
    case 7:
        return;
    }

    switch (self->kind_0x00) {
    case 1:
    case 5:
        lb_equip_detail_blade_draw(self, &pos);
        break;
    case 2:
    case 6:
        lb_equip_detail_compare_draw(self, &pos);
        break;
    case 8:
        lb_equip_detail_item_draw(self, &pos);
        break;
    case 3:
        lb_equip_cell_put(0x19A8, &self->base_0x5C[0], NULL, &pos);
        break;
    }

    if (self->kind_0x00 == 4 || self->kind_0x00 == 10) {
        LbPutAnaPageArrow(0x693, 1, self->cursor_0x08, self->count_0x0A, self->moved_0x06, NULL, active, 0);
        return;
    }
    if (self->mode_0x02 == 3 || self->mode_0x02 == 7) {
        LbPutAnaPageArrow(0x1B7D, 0, self->cursor_0x08, self->count_0x0A, self->moved_0x06, &pos, active, 1);
        return;
    }
    LbPutAnaPageArrow(0x19CC, 1, self->cursor_0x08, self->count_0x0A, self->moved_0x06, &pos, active, 1);
}

/* Copies the nine equipment records between the records the caller holds and the player (mode 0 reads, mode 1
 * writes) or the save block (mode 2 reads). */
void lb_equip_records_copy(_PLW* plw, u8 mode, _EQUIP* set, _EQUIP* body, _EQUIP* parts) {
    LbSaveView* save = (LbSaveView*)lobby_world_block;

    switch (mode) {
    case 0:
        equip_record_copy(&set[0], &plw->equipA[0]);
        equip_record_copy(&set[1], &plw->equipA[1]);
        equip_record_copy(&set[2], &plw->equipA[2]);
        equip_record_copy(&set[3], &plw->equipA[3]);
        equip_record_copy(&set[4], &plw->equipA[4]);
        equip_record_copy(&set[5], &plw->equipA[5]);
        equip_record_copy(body, &plw->equipB);
        equip_record_copy(&parts[0], &plw->equipC);
        equip_record_copy(&parts[1], &plw->equipD);
        break;
    case 1:
        equip_record_copy(&plw->equipA[0], &set[0]);
        equip_record_copy(&plw->equipA[1], &set[1]);
        equip_record_copy(&plw->equipA[2], &set[2]);
        equip_record_copy(&plw->equipA[3], &set[3]);
        equip_record_copy(&plw->equipA[4], &set[4]);
        equip_record_copy(&plw->equipA[5], &set[5]);
        equip_record_copy(&plw->equipB, body);
        equip_record_copy(&plw->equipC, &parts[0]);
        equip_record_copy(&plw->equipD, &parts[1]);
        break;
    case 2:
        equip_record_copy(&set[0], &save->set_0x44[0]);
        equip_record_copy(&set[1], &save->set_0x44[1]);
        equip_record_copy(&set[2], &save->set_0x44[2]);
        equip_record_copy(&set[3], &save->set_0x44[3]);
        equip_record_copy(&set[4], &save->set_0x44[4]);
        equip_record_copy(&set[5], &save->set_0x44[5]);
        equip_record_copy(body, &save->body_0x20);
        equip_record_copy(&parts[0], &save->gun_parts_0x2C[0]);
        equip_record_copy(&parts[1], &save->gun_parts_0x2C[1]);
        break;
    }
}

/* Publishes the weapon class the equipment kind of `equip` selects. */
void lb_equip_kind_se_set(_EQUIP* equip) {
    u8 kind = equip->kind;

    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        system_weapon_class_set(weapon_class_index(kind));
        break;
    case 11:
    case 12:
    case 13:
        system_weapon_class_set(4);
        break;
    default:
        system_weapon_class_set(0);
        break;
    }
}

/* Reads guild card award bit `index` (0-23). */
u16 userdata_award_bit_ck(LbSaveView* save, s16 index) {
    if (index < 12) {
        return (u16)(save->award_lo_0x40F8 & (1 << index));
    }
    return (u16)(save->award_hi_0x40FA & (1 << (index - 12)));
}

/* Sets guild card award bit `index` (0-23). */
void userdata_award_bit_set(LbSaveView* save, s16 index) {
    if (index < 12) {
        save->award_lo_0x40F8 |= (u16)(1 << index);
        return;
    }
    save->award_hi_0x40FA |= (u16)(1 << (index - 12));
}

/* Sets award `award` once 13 or more pieces of the equipment kinds' lists are owned. */
void lb_award_equip_kinds_ck(LbSaveView* save, s16 award) {
    u16** lists;
    s32 kind;
    s16 count = 0;
    u16* item;

    kind = 0;
    lists = lb_award_equip_kind_lists;
    do {
        item = *lists;
        if (item != NULL) {
            while (*item != 0xFFFF) {
                if (userdata_equip_flag_ck((u8)kind, *item, 0) == 1) {
                    count++;
                }
                item++;
            }
        }
        lists++;
        kind++;
    } while (kind < 16);
    if (count >= 13) {
        userdata_award_bit_set(save, award);
    }
}

/* Sets award `award` once every flag of the runs of flag-run table `table` is set. */
void lb_award_flag_runs_ck(LbSaveView* save, s16 award, u16 table) {
    LbAwardFlagRun* run = (LbAwardFlagRun*)lb_award_flag_run_tbl[table];
    s16 i;
    u16 flag;

    while (run->count_0x00 != -1) {
        flag = run->flag_0x02;
        for (i = 0; i < run->count_0x00; i++) {
            if (userdata_flag_ck(flag) == 0) {
                return;
            }
            flag++;
        }
        run++;
    }
    userdata_award_bit_set(save, award);
}

/* Sets award `award` once `need` items of item list `list` are in the box. */
void lb_award_box_items_ck(LbSaveView* save, s16 award, u16 list, s16 need) {
    u16* item = lb_award_item_lists[list];
    s16 count = 0;

    while (*item != 0xFFFF) {
        if ((u32)userdata_box_item_ck((struct LbItemDb*)save, *item++) == 1) {
            count++;
        }
    }
    if (count >= need) {
        userdata_award_bit_set(save, award);
    }
}

/* Sets award `award` once every listed monster passes check `check`: captured (0), a crown (1), the gold large
 * crown (2) or the gold small crown (3). */
void lb_award_monsters_ck(LbSaveView* save, s16 award, u8 check, u8 all) {
    u8* id;
    u8 monster;
    u8 crown;

    if (all == 0) {
        id = &lb_award_monster_ids[1];
    } else {
        id = &lb_award_monster_ids[0];
    }
    for (; (monster = *id) != 0xFF; id++) {
        switch (check) {
        case 0:
            if (save->capture_count_0x3AC0[monster] == 0) {
                return;
            }
            break;
        case 1:
            crown = monster_size_crown_get(monster, save->size_0x3BC0[monster][1]);
            if (crown != 2 && crown != 1) {
                return;
            }
            break;
        case 2:
            if (monster_size_crown_get(monster, save->size_0x3BC0[monster][1]) != 1) {
                return;
            }
            break;
        case 3:
            if (monster_size_crown_get(monster, save->size_0x3BC0[monster][0]) != 3) {
                return;
            }
            break;
        }
    }
    userdata_award_bit_set(save, award);
}

/* Sets award `award` once every flag of the progress list is set. */
void lb_award_progress_ck(LbSaveView* save, s16 award) {
    u16* flag;

    for (flag = lb_award_progress_flags; *flag != 0xFFFF; flag++) {
        if (userdata_progress_flag_ck(*flag) == 0) {
            return;
        }
    }
    userdata_award_bit_set(save, award);
}

/* Counts the set bits of the save block's 64 hunt-bit bytes. */
s16 lb_award_hunt_bits_count(LbSaveView* save) {
    s16 count = 0;
    s16 i;
    s16 bit;
    u8 bits;

    for (i = 0; i < 0x40; i++) {
        bit = 0;
        bits = save->hunt_bits_0x3960[i];
        for (; bit < 8; bit++) {
            if ((1 << bit) & bits) {
                count++;
            }
        }
    }
    return count;
}

/* Checks every guild card award not yet earned and sets the ones whose conditions now hold. */
void lb_award_check_all(void) {
    LbSaveView* save = (LbSaveView*)lobby_world_block;
    s32 i;

    for (i = 0; i < 24; i++) {
        if (userdata_award_bit_ck(save, i) != 0) {
            continue;
        }
        switch ((s16)i) {
        case 0:
            lb_award_flag_runs_ck(save, i, 0);
            break;
        case 1:
            lb_award_flag_runs_ck(save, i, 1);
            break;
        case 2: {
            s32 empty = 0;
            s16 j;

            for (j = 0; j < 10; j++) {
                if (save->record_a_0x52E4[j] == 0 && save->record_b_0x530C[j] == 0) {
                    empty = 1;
                    break;
                }
            }
            if (empty == 0) {
                userdata_award_bit_set(save, i);
            }
            break;
        }
        case 3:
            lb_award_flag_runs_ck(save, i, 3);
            break;
        case 4:
            lb_award_flag_runs_ck(save, i, 4);
            break;
        case 5:
            if (lb_award_hunt_bits_count(save) >= 75) {
                userdata_award_bit_set(save, i);
            }
            break;
        case 6: {
            s16 count = 0;

            if (save->level_0x4654[0] >= 3) {
                count = 1;
            }
            if (save->level_0x4654[1] >= 3) {
                count++;
            }
            if (save->level_0x4654[2] >= 3) {
                count++;
            }
            if (save->level_0x4654[3] >= 3) {
                count++;
            }
            if (count >= 4) {
                userdata_award_bit_set(save, i);
            }
            break;
        }
        case 7:
            lb_award_box_items_ck(save, i, 0, 7);
            break;
        case 8: {
            s16 count = 0;

            if (save->grade_0x3F3A >= 2) {
                count = 1;
            }
            if (save->grade_0x3F46 >= 2) {
                count++;
            }
            if (save->grade_0x3F52 >= 2) {
                count++;
            }
            if (count >= 3) {
                userdata_award_bit_set(save, i);
            }
            break;
        }
        case 9:
            lb_award_progress_ck(save, i);
            break;
        case 10:
            if (ai_npc_level_get(save->companion_points_0x51A0) >= 30) {
                userdata_award_bit_set(save, i);
            }
            break;
        case 11:
            lb_award_monsters_ck(save, i, 1, 0);
            break;
        case 12:
            lb_award_flag_runs_ck(save, i, 5);
            break;
        case 13:
            lb_award_flag_runs_ck(save, i, 6);
            break;
        case 14:
            lb_award_flag_runs_ck(save, i, 7);
            break;
        case 16:
            lb_award_flag_runs_ck(save, i, 8);
            break;
        case 17:
            lb_award_flag_runs_ck(save, i, 9);
            break;
        case 18:
            lb_award_monsters_ck(save, i, 0, 1);
            break;
        case 19:
            if (lb_award_hunt_bits_count(save) >= 100) {
                s16 count = 0;
                s16 j;
                s16 bit;

                for (j = 0; j < 8; j++) {
                    u8 bits;

                    bit = 0;
                    bits = save->event_bits_0x39B2[j];
                    for (; bit < 8; bit++) {
                        if ((1 << bit) & bits) {
                            count++;
                        }
                    }
                }
                if (count >= 35) {
                    userdata_award_bit_set(save, i);
                }
            }
            break;
        case 20:
            lb_award_equip_kinds_ck(save, i);
            break;
        case 21:
            lb_award_box_items_ck(save, i, 1, 20);
            break;
        case 22:
            lb_award_monsters_ck(save, i, 2, 1);
            break;
        case 23:
            lb_award_monsters_ck(save, i, 3, 1);
            break;
        }
    }
}

/* Returns the title the hunter-rank rows give `value` (the last row whose range holds it), or, past the rank rows,
 * the hunt-count title `value` stands for. */
s16 lb_title_range_find(s16 value) {
    s16 found = -1;
    s16 i = 0;
    LbTitleRankRow* row = (LbTitleRankRow*)lb_title_rank_rows;

    while (row->rank_0x00 != 0xFFFF) {
        if (row->title_0x02 <= value && value < row->title_0x02 + row->count_0x04) {
            found = i;
        }
        i++;
        row++;
    }
    if (found < 0) {
        found = i + (value - ((LbTitleHuntRow*)lb_title_hunt_rows)->title_0x02);
    }
    return found;
}

/* Whether title `index` (0-117) is owned. */
u32 lb_title_owned_ck(LbSaveView* save, s16 index) {
    if (index > 117) {
        return 0;
    }
    return save->title_bits_0x4104[index >> 5] & (1 << (index & 31));
}

/* Adds title `index` (0-117) to the owned titles. */
void lb_title_bit_set(LbSaveView* save, s16 index) {
    if (index > 117) {
        return;
    }
    save->title_bits_0x4104[index >> 5] |= 1 << (index & 31);
}

/* Hunt-count title thresholds: the normal monsters', then the three elder ones' (gold, silver, bronze). */
s16 lb_title_hunt_need[6] = { 100, 50, 20, 30, 20, 10 };

/* Grants the titles the hunter rank and the hunt counts have earned. */
void lb_award_rows_apply(void) {
    LbTitleHuntRow* hunt;
    LbSaveView* save;
    s16 i;
    LbTitleRankRow* rank;
    s16* need;
    u16 monster;

    save = (LbSaveView*)lobby_world_block;
    for (rank = (LbTitleRankRow*)lb_title_rank_rows; rank->rank_0x00 != 0xFFFF; rank++) {
        if (save->hunter_rank_0x3DE4 >= rank->rank_0x00 || rank->rank_0x00 == 0) {
            for (i = 0; i < rank->count_0x04; i++) {
                lb_title_bit_set(save, rank->title_0x02 + i);
            }
        }
    }
    for (hunt = (LbTitleHuntRow*)lb_title_hunt_rows; (monster = hunt->monster_0x00) != 0xFFFF; hunt++) {
        u16 total;

        if ((u32)(monster - 24) <= 1 || (s32)monster == 20) {
            need = &lb_title_hunt_need[3];
        } else {
            need = lb_title_hunt_need;
        }
        total = save->kill_count_0x39C0[monster] + save->capture_count_0x3AC0[monster];
        if (total >= need[0]) {
            lb_title_bit_set(save, hunt->title_0x02 + 2);
        } else if (total >= need[1]) {
            lb_title_bit_set(save, hunt->title_0x02 + 1);
        } else if (total >= need[2]) {
            lb_title_bit_set(save, hunt->title_0x02);
        }
    }
}

/* Wears box entry `box` and shows the message for a class change it caused. */
u32 lb_equip_entry_apply(u16 box) {
    u8 change = 0;
    u32 result;

    lobby_w.param_0x12D = 1;
    result = equip_record_wear(lobby_world_block, box, (s8*)&change);
    switch (change) {
    case 1:
        hud_msg_push(0, (const char*)LbStr(3, 481));
        break;
    case 2:
        hud_msg_push(0, (const char*)LbStr(3, 480));
        break;
    }
    return result;
}

/* Whether the lobby kitchen is idle. */
u32 stage_work_idle_ck(void) {
    return lobby_w.kitchen_busy_0x054 == 0;
}

/* Whether the two menu kinds belong together (kind 22 with 1, kind 21 with the decorations 5-7). */
s32 lb_menu_kind_pair_ck(s32 type, s32 value) {
    switch ((u8)type) {
    case 22:
        if ((u8)value == 1) {
            return 1;
        }
        break;
    case 21:
        if ((u32)(u8)value - 5U <= 2U) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Whether menu kind 21 carries the value 4. */
s32 lb_menu_kind_deco_ck(s32 type, s32 value) {
    if ((u8)type == 21) {
        if ((u8)value == 4) {
            return 1;
        }
    }
    return 0;
}

/* Whether the talk state's first flag bit is set. */
s32 lb_special_bgm_ck(void) {
    LbTalkState* state = &lb_talk_state;
    u8 slot_set;

    if (state == 0) {
        return 0;
    }
    slot_set = state->flags_0x09 & 1;
    return slot_set != 0;
}

/* The NPC kinds the camera turns to when a talk opens, zero-terminated. */
u16 lb_npc_talk_cam_ids[30] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0D, 0x13, 0x17, 0x1B, 0x1D, 0x26,
                         0x27, 0x28, 0x29, 0x2A, 0x30, 0x31, 0x32, 0x33, 0x35, 0x38, 0x39, 0x40, 0x49, 0, 0 };

/* The equipment records a kind-6 talk reward hands out. */
_EQUIP lb_talk_reward_equips[1] = { { 6, 0, 0x0014, { 0x0011 }, { 0, 0, 0 } } };

/* Ends an NPC talk: advances the NPC's talk cursor, re-picks every NPC's talk entries and opens the screen the
 * NPC's kind runs, or closes the panel. */
void lb_npc_talk_screen_open(void) {
    LbTalkState* state = &lb_talk_state;
    _LB_NPC* npc;
    s32 i;
    u8 kind;

    if ((state->npc_0x04->field_0x227 & 1) == 0) {
        if (++state->npc_0x04->field_0x226 >= 3) {
            state->npc_0x04->field_0x226 = 0;
        }
    }
    for (i = 0, npc = lb_npc; i < 18; npc++, i++) {
        if (npc->field_0x000 != 0) {
            lb_npc_talk_select(npc);
        }
    }
    state->flags_0x09 = 0;
    kind = state->npc_0x04->field_0x002;
    switch (kind) {
    case 3:
        if (lb_unlock_cond_ck(6) == 1) {
            lb_quest_accept_screen_open((struct LbQuestBoardData*)state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 49:
        if (lb_unlock_cond_ck(29) == 1) {
            lb_quest_accept_screen_open((struct LbQuestBoardData*)state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 51:
        if (lb_unlock_cond_ck(29) == 1) {
            lb_quest_join_screen_open((struct LbQuestBoardData*)state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 5:
    case 39:
    case 41:
    case 50:
        lb_shop_open(state->player_0x00, state->npc_0x04);
        return;
    case 7:
    case 57:
        lb_eqshop_open(state->player_0x00, (s32)state->npc_0x04);
        return;
    case 8:
        if (lb_unlock_cond_ck(1) != 0) {
            lb_smith_open(state->player_0x00, (u8*)state->npc_0x04);
            return;
        }
        break;
    case 56:
        lb_smith_open(state->player_0x00, (u8*)state->npc_0x04);
        return;
    case 4:
        lb_goods_sale_open(state->player_0x00, (s32)state->npc_0x04);
        return;
    case 6:
        if (userdata_progress_flag_ck(26) != 0) {
            lb_screen05_open(state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 9:
        if (kujira_event_over_ck() == 1 && state->entry_0x24.flag_0x08 != 184) {
            lb_barter_open((s32)state->player_0x00, (s32)state->npc_0x04);
            return;
        }
        break;
    case 13:
        if (lb_unlock_cond_ck(9) != 0 && lb_world_mode_get() != 2 && stage_work_idle_ck() == 1) {
            lb_kitchen_open(state->player_0x00, (s32)state->npc_0x04);
            return;
        }
        break;
    case 19:
        if (userdata_progress_flag_ck(25) != 0) {
            lb_box_screen_open((s32)state->player_0x00, (s32)state->npc_0x04);
            return;
        }
        break;
    case 20:
    case 21:
    case 22: {
        u16 seed = kind - 15;

        if ((ran_suu(1) & 0x1F) == 0 || userdata_progress_flag_ck(state->npc_0x04->field_0x002 + 76) == 0) {
            userdata_progress_flag_set(state->npc_0x04->field_0x002 + 76);
            name_menu_open(state->player_0x00, state->npc_0x04);
            lobby_w.seed_0x036[seed] = rand_lcg_step(lobby_w.seed_0x036[seed]);
            return;
        }
        break;
    }
    case 1:
        if (lb_unlock_cond_ck(4) != 0) {
            lb_screen13_open(state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 38:
        lb_trade_open((s32)state->player_0x00, (LbTradeSource*)state->npc_0x04);
        return;
    case 73:
        if (state->player_0x00->field_0xB03 != 0) {
            break;
        }
    case 23:
        if ((ran_suu(1) & 0x1F) == 0 || userdata_progress_flag_ck(95) == 0) {
            userdata_progress_flag_set(95);
            name_menu_open(state->player_0x00, state->npc_0x04);
            lobby_w.seed_0x036[8] = rand_lcg_step(lobby_w.seed_0x036[8]);
            return;
        }
        pig_menu_open(state->player_0x00, state->npc_0x04);
        return;
    case 42:
        if (lb_unlock_cond_ck(29) == 1) {
            note_trade_open(state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 71:
        if (state->player_0x00->field_0xB03 != 0) {
            break;
        }
    case 25:
        lb_screen24_open(state->player_0x00, (_PLW*)state->npc_0x04);
        return;
    case 40:
        if (lb_unlock_cond_ck(29) == 1) {
            lb_screen26_open(state->player_0x00, state->npc_0x04);
            return;
        }
        break;
    case 64:
        if (userdata_progress_flag_ck(74) == 0 && userdata_progress_flag_ck(457) == 1) {
            userdata_progress_flag_set(74);
        }
        if (lb_unlock_cond_ck(33) == 1) {
            lb_screen30_open(state->player_0x00, (_PLW*)state->npc_0x04);
            return;
        }
        break;
    case 27:
        lb_screen34_open(state->player_0x00, (_PLW*)state->npc_0x04);
        return;
    case 29:
        lb_community_select_open((u32)state->player_0x00, (u32)state->npc_0x04);
        return;
    }
    lb_panel_close();
}

/* Whether the unlock conditions the bits of `mask` name all fail: bit 0/1 the item-list selection, bits 3-25 a
 * stage of the unlock ladder reached but the next one not, bits 22/26 the whale event's state. */
u32 lb_talk_cond_mask_ck(u32 mask) {
    u32 ok = 1;

    if (mask == 0) {
        return 1;
    }
    if ((mask & 1) != 0 && getItemListSelection() == 0) {
        ok = 0;
    }
    if ((mask & 2) != 0 && getItemListSelection() == 1) {
        ok = 0;
    }
    if ((mask & 8) != 0 && lb_unlock_cond_ck(0) == 1 && lb_unlock_cond_ck(6) == 0) {
        ok = 0;
    }
    if ((mask & 0x10) != 0 && lb_unlock_cond_ck(6) == 1 && lb_unlock_cond_ck(9) == 0) {
        ok = 0;
    }
    if ((mask & 0x20) != 0 && lb_unlock_cond_ck(9) == 1 && lb_unlock_cond_ck(12) == 0) {
        ok = 0;
    }
    if ((mask & 0x40) != 0 && lb_unlock_cond_ck(12) == 1 && lb_unlock_cond_ck(16) == 0) {
        ok = 0;
    }
    if ((mask & 0x80) != 0 && lb_unlock_cond_ck(16) == 1 && lb_unlock_cond_ck(20) == 0) {
        ok = 0;
    }
    if ((mask & 0x100) != 0 && lb_unlock_cond_ck(20) == 1 && lb_unlock_cond_ck(23) == 0) {
        ok = 0;
    }
    if ((mask & 0x200) != 0 && lb_unlock_cond_ck(23) == 1 && lb_unlock_cond_ck(25) == 0) {
        ok = 0;
    }
    if ((mask & 0x400) != 0 && lb_unlock_cond_ck(25) == 1 && lb_unlock_cond_ck(26) == 0) {
        ok = 0;
    }
    if ((mask & 0x800) != 0 && lb_unlock_cond_ck(26) == 1 && lb_unlock_cond_ck(27) == 0) {
        ok = 0;
    }
    if ((mask & 0x1000) != 0 && lb_unlock_cond_ck(27) == 1) {
        ok = 0;
    }
    if ((mask & 0x2000) != 0 && lb_unlock_cond_ck(28) == 1 && lb_unlock_cond_ck(29) == 0) {
        ok = 0;
    }
    if ((mask & 0x4000) != 0 && lb_unlock_cond_ck(29) == 1 && lb_unlock_cond_ck(30) == 0) {
        ok = 0;
    }
    if ((mask & 0x8000) != 0 && lb_unlock_cond_ck(30) == 1 && lb_unlock_cond_ck(31) == 0) {
        ok = 0;
    }
    if ((mask & 0x10000) != 0 && lb_unlock_cond_ck(31) == 1 && lb_unlock_cond_ck(32) == 0) {
        ok = 0;
    }
    if ((mask & 0x20000) != 0 && lb_unlock_cond_ck(32) == 1 && lb_unlock_cond_ck(33) == 0) {
        ok = 0;
    }
    if ((mask & 0x40000) != 0 && lb_unlock_cond_ck(33) == 1 && lb_unlock_cond_ck(34) == 0) {
        ok = 0;
    }
    if ((mask & 0x80000) != 0 && lb_unlock_cond_ck(34) == 1 && lb_unlock_cond_ck(35) == 0) {
        ok = 0;
    }
    if ((mask & 0x100000) != 0 && lb_unlock_cond_ck(35) == 1 && lb_unlock_cond_ck(36) == 0) {
        ok = 0;
    }
    if ((mask & 0x200000) != 0 && lb_unlock_cond_ck(36) == 1 && lb_unlock_cond_ck(37) == 0) {
        ok = 0;
    }
    if ((mask & 0x800000) != 0 && lb_unlock_cond_ck(37) == 1 && lb_unlock_cond_ck(38) == 0) {
        ok = 0;
    }
    if ((mask & 0x1000000) != 0 && lb_unlock_cond_ck(38) == 1 && lb_unlock_cond_ck(39) == 0) {
        ok = 0;
    }
    if ((mask & 0x2000000) != 0 && lb_unlock_cond_ck(39) == 1) {
        ok = 0;
    }
    if ((mask & 0x400000) != 0 && LbCheckKujiraEvent() == 1) {
        ok = 0;
    }
    if ((mask & 0x4000000) != 0 && LbCheckKujiraEvent() == 0) {
        ok = 0;
    }
    return ok;
}

/* Whether conditional talk row `row` applies: not done yet, unlocked, and its progress flags in the state `cond`
 * asks for. */
u32 lb_talk_cond_row_ck(LbTalkCondRow* row) {
    if (userdata_progress_flag_ck(row->done_flag_0x08) == 1) {
        return 0;
    }
    if (row->unlock_0x0A == 0xFF || lb_unlock_cond_last_ck(row->unlock_0x0A) == 1) {
        switch (row->cond_0x03) {
        case 0:
            return 1;
        case 1:
            return userdata_progress_flag_ck(row->flag_a_0x04);
        case 2:
            if (userdata_progress_flag_ck(row->flag_a_0x04) != 0 && userdata_progress_flag_ck(row->flag_b_0x06) != 0) {
                return 1;
            }
            break;
        case 3:
            if (userdata_progress_flag_ck(row->flag_a_0x04) != 0 || userdata_progress_flag_ck(row->flag_b_0x06) != 0) {
                return 1;
            }
            break;
        case 4:
            if (userdata_progress_flag_ck(row->flag_a_0x04) != 0 && userdata_progress_flag_ck(row->flag_b_0x06) == 0) {
                return 1;
            }
            break;
        case 5:
            if (userdata_progress_flag_ck(row->flag_a_0x04) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* Fills `entry` from the unlock row of `rows` whose page is `page`. */
void lb_talk_unlock_entry_get(LbTalkEntry* entry, LbTalkUnlockRow* rows, u16 page) {
    for (; rows->page_0x06 != 0xFFFF; rows++) {
        if (rows->page_0x06 == page) {
            memset(entry, 0, 10);
            entry->next_yes_0x00 = rows->next_yes_0x02;
            entry->next_no_0x01 = rows->next_no_0x03;
            entry->page_0x06 = page;
            entry->value_0x04 = rows->value_0x04;
            return;
        }
    }
}

/* Fills `entry` from the random row of `rows` whose page is `page`. */
void lb_talk_rand_entry_get(LbTalkEntry* entry, LbTalkRandRow* rows, u16 page) {
    for (; rows->page_0x08 != 0xFFFF; rows++) {
        if (rows->page_0x08 == page) {
            memset(entry, 0, 10);
            entry->next_yes_0x00 = rows->next_yes_0x04;
            entry->next_no_0x01 = rows->next_no_0x05;
            entry->page_0x06 = page;
            entry->value_0x04 = rows->value_0x06;
            return;
        }
    }
}

/* Returns the page of the first conditional row that applies (0xFFFF for none), filling `entry` from it when given
 * and `marked` with the row's mark bit. */
u16 lb_talk_cond_entry_find(LbTalkEntry* entry, LbTalkCondRow* rows, u8* marked) {
    LbTalkCondRow* row = rows;

    for (; row->page_0x0E != 0xFFFF; row++) {
        if (lb_talk_cond_row_ck(row) == 1) {
            if (entry != NULL) {
                memset(entry, 0, 10);
                entry->next_yes_0x00 = row->next_yes_0x01;
                entry->next_no_0x01 = row->next_no_0x02;
                entry->page_0x06 = row->page_0x0E;
                entry->cond_0x02 = 1;
                if ((row->flags_0x00 & 4) != 0) {
                    *marked = 1;
                } else {
                    *marked = 0;
                }
                entry->flag_0x08 = row->done_flag_0x08;
                entry->value_0x04 = row->value_0x0C;
            } else if ((row->flags_0x00 & 4) != 0) {
                *marked = 1;
            } else {
                *marked = 0;
            }
            return row->page_0x0E;
        }
    }
    return 0xFFFF;
}

/* Returns the page of the first unlock row that is unlocked and whose condition mask passes (0xFFFF for none). */
u16 lb_talk_unlock_row_find(LbTalkUnlockRow* rows) {
    for (; rows->page_0x06 != 0xFFFF; rows++) {
        if ((rows->unlock_0x01 == 0xFF || lb_unlock_cond_last_ck_c1(rows->unlock_0x01) == 1)
            && lb_talk_cond_mask_ck(rows->mask_0x00) == 1) {
            return rows->page_0x06;
        }
    }
    return 0xFFFF;
}

/* Collects into `pages` (128 at most) the pages of the random rows whose condition mask passes; returns the count. */
u16 lb_talk_rand_rows_collect(LbTalkRandRow* rows, u16* pages) {
    u16 count;

    memset(pages, 0, 0x100);
    count = 0;
    for (; rows->page_0x08 != 0xFFFF; rows++) {
        if (lb_talk_cond_mask_ck(rows->mask_0x00) != 0) {
            pages[count] = rows->page_0x08;
            count++;
            if (count >= 128) {
                break;
            }
        }
    }
    if (count == 0) {
        return 0;
    }
    return count;
}

/* Picks the page the talk opens on and fills the state's entry from its row: the conditional row while the NPC
 * has one, else the NPC's current unlock or random page. */
u32 lb_talk_entry_pick(LbTalkState* state, LbNpcTalkData* talk) {
    _LB_NPC* npc = state->npc_0x04;
    u8 marked;
    u16 page;

    if ((npc->field_0x227 & 1) != 0) {
        page = lb_talk_cond_entry_find(&state->entry_0x24, talk->cond_rows_0x00, &marked);
    } else if (npc->talk_kinds_0x222[npc->field_0x226] == 1) {
        lb_talk_unlock_entry_get(&state->entry_0x24, talk->unlock_rows_0x04, npc->talk_ids_0x21C[npc->field_0x226]);
        page = npc->talk_ids_0x21C[npc->field_0x226];
    } else {
        lb_talk_rand_entry_get(&state->entry_0x24, talk->rand_rows_0x08, npc->talk_ids_0x21C[npc->field_0x226]);
        page = npc->talk_ids_0x21C[npc->field_0x226];
    }
    return page;
}

/* Re-picks NPC `npc`'s talk: whether a conditional row applies (and is marked), then its three pages from the
 * unlock rows and the shuffled random rows. */
void lb_npc_talk_select(_LB_NPC* npc) {
    LbTalkEntry entry;
    u16 pages[128];
    u16 i;
    LbNpcTalkData* talk;
    u8 marked;
    s8 slot;
    u16 seed;

    if (lobby_w.talk_tbl_0x0C0 != NULL && npc != NULL) {
        talk = lobby_w.talk_tbl_0x0C0[npc->field_0x002];
        npc->field_0x227 = 0;
        marked = 0;
        if (lb_talk_cond_entry_find(&entry, talk->cond_rows_0x00, &marked) != 0xFFFF) {
            npc->field_0x227 |= 1;
        }
        if (marked != 0) {
            npc->field_0x227 |= 0x80;
        }
        slot = 0;
        i = lb_talk_unlock_row_find(talk->unlock_rows_0x04);
        if (i != 0xFFFF) {
            npc->talk_kinds_0x222[0] = 1;
            npc->talk_ids_0x21C[0] = i;
            slot = 1;
        }
        seed = lobby_w.seed_0x036[2];
        for (i = 0; i < npc->field_0x002; i++) {
            seed = rand_lcg_step(seed);
        }
        i = lb_talk_rand_rows_collect(talk->rand_rows_0x08, pages);
        for (; slot < 3; slot++) {
            if (i != 0) {
                u16 pick;

                npc->talk_kinds_0x222[slot] = 0;
                seed = rand_lcg_step(seed);
                pick = seed % i;
                npc->talk_ids_0x21C[slot] = pages[pick];
                if (i > 1) {
                    memcpy(&pages[pick], &pages[pick + 1], (128 - (pick + 1)) * 2);
                    i--;
                }
            } else {
                npc->talk_kinds_0x222[slot] = 0;
                npc->talk_ids_0x21C[slot] = talk->rand_rows_0x08->page_0x08;
            }
        }
    }
}

/* Whether NPC kind `id` is one the camera turns to. */
s32 lb_npc_talk_id_ck(u16 id) {
    const u16* p = lb_npc_talk_cam_ids;

    while (*p != 0) {
        if (id == *p) {
            return 1;
        }
        p++;
    }
    return 0;
}

/* Opens a talk between `player` and NPC `npc`: picks its page, sets its text up and the lobby's talk mode. */
void lb_npc_talk_open(_PLW* player, _LB_NPC* npc) {
    LbNpcTalkData** tbl = lobby_w.talk_tbl_0x0C0;
    LbTalkState* state = &lb_talk_state;

    memset(state, 0, 0x134);
    state->npc_0x04 = npc;
    lb_talk_state.player_0x00 = player;
    state->talk_0x30 = tbl[npc->field_0x002];
    state->text_id_0x18 = lb_talk_entry_pick(state, state->talk_0x30);
    if (state->text_id_0x18 == 0xFFFF) {
        lb_npc_talk_select(npc);
        return;
    }
    lb_talk_text_set(state->text_id_0x18);
    if (lb_npc_talk_id_ck(npc->field_0x002) != 0) {
        camera_lobby_npc_set(npc);
    }
    lobby_w.state_0x000 = 7;
    lobby_w.active_0x008 = 1;
    lb_npc_talk_reply(state->npc_0x04, state->entry_0x24.flag_0x08, state->text_id_0x18);
    if (npc->field_0x002 == 2) {
        lb_progress_unlock(99);
        if ((u32)(state->entry_0x24.flag_0x08 - 157) <= 1 || (s32)state->entry_0x24.flag_0x08 == 117
            || (s32)state->entry_0x24.flag_0x08 == 152 || (s32)state->entry_0x24.flag_0x08 == 154
            || (s32)state->entry_0x24.flag_0x08 == 45) {
            state->flags_0x09 |= 1;
        }
    }
}

/* Clears the talk state and points it at NPC `npc`'s talk data. */
void lb_menu_open(_LB_NPC* npc) {
    LbNpcTalkData** tbl = lobby_w.talk_tbl_0x0C0;

    memset(&lb_talk_state, 0, 0x134);
    lb_talk_state.npc_0x04 = npc;
    lb_talk_state.talk_0x30 = tbl[npc->field_0x002];
}

/* Returns a random word of word list `list` among those whose condition mask passes. */
char* lb_talk_word_pick(LbTalkWordList* list) {
    s32 picks[128];
    LbTalkWord* word;
    s32* pick;
    LbTalkWord* words;
    s16 count;
    s16 i;
    s16 n;

    memset(picks, 0, 0x200);
    count = 0;
    words = list->words_0x00;
    i = 0;
    word = words;
    pick = picks;
    for (; i < list->count_0x04; word++, i++) {
        if (lb_talk_cond_mask_ck(word->mask_0x00) != 0) {
            *pick++ = i;
            count++;
        }
    }
    if (count == 0) {
        n = 0;
    } else {
        n = ran_suu(1) % count;
    }
    return words[picks[n]].text_0x04;
}

/* Opens talk page `page` of the NPC's talk data: its text and its yes/no links. */
void lb_talk_page_open(s8 page) {
    LbTalkState* state = &lb_talk_state;

    memset(&state->entry_0x24, 0, 10);
    lb_talk_text_set(state->talk_0x30->pages_0x0C[page].text_0x06);
    state->value_0x14 = 0;
    state->flags_0x09 = 0;
    state->typed_0x1C = 0;
    state->shown_0x20 = 0;
    state->entry_0x24.next_yes_0x00 = state->talk_0x30->pages_0x0C[page].next_yes_0x01;
    state->entry_0x24.next_no_0x01 = state->talk_0x30->pages_0x0C[page].next_no_0x02;
    state->entry_0x24.page_0x06 = state->talk_0x30->pages_0x0C[page].text_0x06;
    state->entry_0x24.value_0x04 = state->talk_0x30->pages_0x0C[page].value_0x04;
    lb_talk_page_mode_set(0);
}

/* Sets talk text `id` up: its type, its kind, and its string with the `~S00`-`~S03` tags replaced by words. */
void lb_talk_text_set(u32 id) {
    LbTalkState* state = &lb_talk_state;

    state->text_id_0x18 = id;
    state->text_kind_0x0A = state->talk_0x30->texts_0x14[id].kind_0x01;
    state->text_type_0x0C = state->talk_0x30->texts_0x14[id].type_0x00;
    state->typed_0x1C = 0;
    state->choice_0x10 = 0;
    strcpy(state->text_0x34, state->talk_0x30->texts_0x14[id].text_0x04);
    lb_talk_tag_replace(&state->talk_0x30->words_0x18[0], state->text_0x34, "~S00");
    lb_talk_tag_replace(&state->talk_0x30->words_0x18[1], state->text_0x34, "~S01");
    lb_talk_tag_replace(&state->talk_0x30->words_0x18[2], state->text_0x34, "~S02");
    lb_talk_tag_replace(&state->talk_0x30->words_0x18[3], state->text_0x34, "~S03");
}

/* Replaces every `tag` in `text` with a word picked from `list`. */
void lb_talk_tag_replace(LbTalkWordList* list, char* text, const char* tag) {
    char buf[0x100];
    char* at;
    s32 len;

    at = findSubstring(text, tag);
    if (at != NULL) {
        buf[0] = 0;
        while (at != NULL) {
            len = at - text;
            if (len != 0) {
                memcpy(buf, text, len);
                buf[len] = 0;
            }
            strcat(buf, lb_talk_word_pick(list));
            strcat(buf, text + len + 4);
            at = findSubstring(at + 4, tag);
        }
        strcpy(text, buf);
    }
}

/* Runs the talk state's tick: the first tick plays the window sound, the second ends the talk once the page step
 * reports a choice. */
void lb_screen07_step(void) {
    s8 state = lb_talk_state.state_0x08;

    switch (state) {
    case 0:
        lb_talk_state.state_0x08++;
        sysSE_window_open_play();
        break;
    case 1:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            lb_npc_talk_screen_open();
        }
        break;
    }
}

/* Gives the talk's reward (points, an item into or out of the pouch, or an equipment piece) and unlocks the
 * entry's progress flag; returns 0 when the reward could not be given. */
s32 lb_talk_reward_give(LbTalkState* state) {
    LbTalkReward* reward;
    char buf[0x80];
    s32 result = 0;

    if (state->entry_0x24.value_0x04 != 0) {
        reward = &state->talk_0x30->rewards_0x10[state->entry_0x24.value_0x04];
        switch (reward->kind_0x00) {
        case 1:
            score_add_clamped(reward->id_0x02, &((LbSaveView*)lobby_world_block)->points_0x18);
            sprintf(buf, (const char*)LbStr(0, 517), reward->id_0x02);
            hud_msg_push(0, buf);
            sysSE_req(9);
            break;
        case 2:
            if (reward->count_0x04 <= eft052_page_count_ck(reward->id_0x02, 0)) {
                u32 name = ItemName(reward->id_0x02);

                sprintf(buf, (const char*)LbStr(0, 513), name);
                hud_msg_push(0, buf);
                eft052_page_take(state->player_0x00, reward->id_0x02, reward->count_0x04, 0);
                result = 2;
            } else if (reward->count_0x04 <= eft052_page_count_ck(reward->id_0x02, 1)) {
                u32 name = ItemName(reward->id_0x02);

                sprintf(buf, (const char*)LbStr(0, 515), name);
                hud_msg_push(0, buf);
                eft052_page_put(state->player_0x00, reward->id_0x02, reward->count_0x04, 0);
                result = 1;
            } else {
                hud_msg_push(0, (const char*)LbStr(0, 514));
                return 0;
            }
            break;
        case 3:
            if (userdata_equip_box_free_count() != 0) {
                u16 box;
                u32 name;

                if (reward->id_0x02 == 6) {
                    _EQUIP* equip = &lb_talk_reward_equips[reward->count_0x04];

                    box = userdata_equip_box_add((struct Q_UserData*)lobby_world_block, equip);
                    name = GetEquipName(equip->kind, equip->item_id);
                } else {
                    box = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block, reward->id_0x02, reward->count_0x04);
                    name = GetEquipName(reward->id_0x02, reward->count_0x04);
                }
                if (box != 0xFFFF) {
                    sprintf(buf, (const char*)LbStr(0, 518), name);
                    hud_msg_push(0, buf);
                    if (state->npc_0x04->field_0x002 == 9 && reward->id_0x02 == 14) {
                        bgm_lobby_return_req();
                    }
                    result = 1;
                    break;
                }
                hud_msg_push(0, (const char*)LbStr(0, 519));
                return 0;
            }
            hud_msg_push(0, (const char*)LbStr(0, 519));
            return 0;
        }
    }
    if (state->entry_0x24.flag_0x08 != 0) {
        lb_progress_unlock(state->entry_0x24.flag_0x08);
    }
    return result;
}

/* Returns the talk's last chosen value. */
u32 lb_talk_page_value_get(void) {
    return lb_talk_state.value_0x14;
}

/* Sets the talk's choice and its chosen value. */
void lb_talk_page_value_set(s32 value) {
    lb_talk_state.choice_0x10 = (u8)value;
    lb_talk_state.value_0x14 = (u8)value;
}

/* Answers NPC `npc`'s talk entry `flag` when it switches the NPC's mode (the pig farmer's 184, the trader's 208). */
void lb_npc_talk_reply(_LB_NPC* npc, s32 flag, u32 page) {
    if (npc == 0) {
        return;
    }
    switch (npc->field_0x002) {
    case 9:
        if ((u16)flag != 184) {
            return;
        }
        lb_npc_talk_mode_set(npc, 2);
        return;
    case 15:
        if ((u16)flag != 208) {
            return;
        }
        lb_npc_talk_mode_set(npc, 0);
        return;
    default:
        return;
    }
}

/* Advances the talk text's typing by one step unless it is complete; returns 0 once it is. */
s32 lb_talk_text_step(char* text, s32 typed, s32 type, s32 choice) {
    if (lb_talk_state.flags_0x09 & 2) {
        return 0;
    }
    lb_talk_state.shown_0x20 = lb_talk_state.typed_0x1C / 2 + 1;
    lb_talk_state.typed_0x1C = lb_talk_state.typed_0x1C + 1;
    return 1;
}

/* One step of the talk page: types the text, answers the A/B buttons and the yes/no cursor, follows the page links
 * (playing `se_yes`/`se_no`, a negative id stops that sound) and draws the page; returns 1 when the talk ended
 * (the reward is given), 2 when it was cancelled. */
s32 lb_menu_step(u8 active, s32 se_close, s32 se_yes, s32 se_no) {
    s32 result = 0;
    LbTalkState* state = &lb_talk_state;

    menu_pad_map_set(0, 2);
    switch (lb_talk_text_step(state->text_0x34, state->typed_0x1C, state->text_type_0x0C, state->choice_0x10)) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0 && active != 0) {
            switch (state->talk_0x30->texts_0x14[state->text_id_0x18].type_0x00) {
            case 3:
                lb_talk_text_set(state->text_id_0x18 + 1);
                break;
            case 1:
                if (state->choice_0x10 != 1) {
                    if (state->entry_0x24.next_yes_0x00 != 0) {
                        lb_talk_text_set(state->entry_0x24.next_yes_0x00 + state->entry_0x24.page_0x06);
                        if (se_yes >= 0) {
                            sysSE_req(se_yes);
                        } else {
                            sysSE_stop(-se_yes);
                        }
                    } else {
                        result = 1;
                        if (se_close != -1) {
                            sysSE_req(se_close);
                        }
                    }
                } else if (state->entry_0x24.next_no_0x01 != 0) {
                    lb_talk_text_set(state->entry_0x24.next_no_0x01 + state->entry_0x24.page_0x06);
                    if (se_no >= 0) {
                        sysSE_req(se_no);
                    } else {
                        sysSE_stop(-se_no);
                    }
                } else {
                    result = 1;
                    if (se_close != -1) {
                        sysSE_req(1);
                    }
                }
                break;
            default:
                result = 1;
                break;
            }
            state->typed_0x1C = 0;
            state->shown_0x20 = 0;
        } else if (lb_cmd_pressed_ck(0x20) != 0 && active != 0) {
            if ((u32)(state->text_type_0x0C - 1) <= 1) {
                if (state->choice_0x10 != 1) {
                    sysSE_req(1);
                    state->choice_0x10 = 1;
                    state->value_0x14 = 1;
                } else {
                    if (state->entry_0x24.next_no_0x01 != 0) {
                        if (se_no >= 0) {
                            sysSE_req(se_no);
                        } else {
                            sysSE_stop(-se_no);
                        }
                        lb_talk_text_set(state->entry_0x24.next_no_0x01 + state->entry_0x24.page_0x06);
                    } else {
                        if (se_close != -1) {
                            sysSE_req(1);
                        }
                        result = 1;
                    }
                    state->typed_0x1C = 0;
                    state->shown_0x20 = 0;
                }
            } else if (state->entry_0x24.cond_0x02 == 0) {
                result = 2;
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(0xC) != 0 && active != 0 && (u32)(state->text_type_0x0C - 1) <= 1) {
            if (lb_cmd_repeat_ck(4) != 0) {
                if (state->choice_0x10 != 0) {
                    sysSE_req(3);
                    state->choice_0x10 = 0;
                    state->value_0x14 = 0;
                }
            } else if (lb_cmd_repeat_ck(8) != 0 && state->choice_0x10 != 1) {
                sysSE_req(3);
                state->choice_0x10 = 1;
                state->value_0x14 = 1;
            }
        }
        break;
    case 1:
        if ((lb_cmd_pressed_ck(0x10) != 0 || lb_cmd_pressed_ck(0x20) != 0) && active != 0) {
            state->shown_0x20 = font_str_char_count(state->text_0x34);
        }
        break;
    default:
        if (lb_cmd_pressed_ck(0x20) != 0 && active != 0 && state->entry_0x24.cond_0x02 == 0) {
            result = 2;
            sysSE_req(1);
        }
        break;
    }
    if (result == 0) {
        subTransSet((u32)lb_talk_page_draw, 0, 0);
    }
    if (result == 1) {
        lb_talk_reward_give(state);
    }
    return result;
}

/* Switches the talk page to menu mode `mode`. */
s32 lb_talk_page_mode_set(s32 mode) {
    return lb_menu_step((u8)mode, 0, 0, 1);
}

/* Switches the talk page back to menu mode 1. */
s32 lb_talk_page_mode_reset(void) {
    return lb_menu_step(1, 0, 0, 1);
}

/* Draws the talk page: the speaker's name, the typed text and, once it is complete, the yes/no panel or the
 * waiting icon. */
void lb_talk_page_draw(void) {
    LbTalkState* state = &lb_talk_state;
    _mh_ivec2_ pos;

    draw_help_window(state->entry_0x24.cond_0x02 != 0);
    note_box_anchor_upper(&pos);
    font_set_size(16, 16);
    font_print_ex(pos.x, pos.y, 5, lb_speaker_name_get(state->npc_0x04->field_0x002));
    note_box_anchor_lower(&pos);
    font_set_size(18, 18);
    if (font_print_typed(pos.x, pos.y, 0, state->shown_0x20, state->text_0x34) == 0) {
        state->flags_0x09 &= ~2;
    } else if ((state->flags_0x09 & 2) == 0) {
        state->flags_0x09 |= 2;
        sysSE_req(34);
    }
    if ((state->flags_0x09 & 2) != 0) {
        if ((u32)(state->text_type_0x0C - 1) <= 1) {
            lb_panel_yes_no_draw(0x1879, state->choice_0x10);
            return;
        }
        draw_spin_icon(pos.x + 0x15B, pos.y + 0x32, 0);
    }
}

/* Whether NPC `npc`'s talk is set (bit 0 of its talk flags). */
s32 lb_npc_talk_flag_ck(_LB_NPC* npc) {
    u8 filled;

    if (npc == 0) {
        return 0;
    }
    filled = npc->field_0x227 & 1;
    return filled != 0;
}

/* Whether NPC `npc` shows the new-talk mark (bit 7 of its talk flags), never for the Poogie keeper while the local
 * player has the 0xB03 flag. */
s32 lb_npc_talk_mark_ck(_LB_NPC* npc) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2);

    plw = plw + (s8)my_player_no();

    if (npc == NULL) {
        return 0;
    }
    if (npc->field_0x002 == 73 && plw->field_0xB03 != 0) {
        return 0;
    }
    if ((npc->field_0x227 & 0x80) != 0) {
        return 1;
    }
    return 0;
}

/* Clears the row marks and copies the positions of area `area`'s placements into the scratch block. */
void lb_area_view_set(u8 area) {
    LbMenuScratch* scratch = &lb_menu_scratch;
    u8 first;
    u8 count;
    u8 i;

    lb_menu_row_marks_clear();
    get_option_pair(area, &first, &count);
    if (count != 0) {
        for (i = 0; i < count; i++) {
            vec_to_mh_vec3(&scratch->slots_0x160[i], &lb_area_view_pos_tbl[first + i].pos);
        }
    }
}

/* Publishes one selected slot's byte in the selection table. */
void lb_menu_row_mark_set(s32 index, u8 value) {
    lb_menu_scratch.flags_0x156[(u8)index] = value;
}

/* Marks the area's row `row` as selected (bit 2) and clears that bit on the other rows. */
void lb_menu_row_mark_select(u8 row) {
    LbMenuScratch* scratch = &lb_menu_scratch;
    u8 first;
    u8 count;
    u8 i;

    get_option_pair(lobby_w.area_0x002, &first, &count);
    for (i = 0; i < 8; i++) {
        if (i == row - first) {
            scratch->flags_0x156[i] |= 4;
        } else if (scratch->flags_0x156[i] != 0) {
            scratch->flags_0x156[i] &= ~4;
        }
    }
}

/* Clears the selection table. */
void lb_menu_row_marks_clear(void) {
    memset(lb_menu_scratch.flags_0x156, 0, 8);
}

/* Clears the scratch block, empties its rows and attaches the local player's target marker and ring effect. */
void lb_menu_scratch_reset(void) {
    LbMenuScratch* scratch = &lb_menu_scratch;
    s32 i;

    memset(scratch, 0, 0x1C0);
    for (i = 0; i < 20; i++) {
        scratch->rows_0x00C[i].table_0x00 = 0;
        scratch->rows_0x00C[i].model_0x04 = 0;
        scratch->rows_0x00C[i].dist_0x08 = 0.0f;
        scratch->rows_0x00C[i].rank_0x0C = -1;
    }
    scratch->index_0x14C = -1;
    scratch->plw_0x000 = (_PLW*)get_move_work_adrs(2);
    scratch->plw_0x000 = scratch->plw_0x000 + (s8)my_player_no();
    menu_target_marker_set(scratch);
    eft_em_model_ring_attach(scratch);
}

/* Empties the scratch block's rows (and its selection when `full`). */
void lb_menu_scratch_init(s16 full) {
    LbMenuScratch* scratch = &lb_menu_scratch;
    s16 i;

    if (full != 0) {
        scratch->index_0x14C = -1;
    }
    scratch->fixed_a_0x004 = 0;
    scratch->fixed_b_0x008 = 0;
    scratch->count_0x154 = 0;
    for (i = 0; i < 20; i++) {
        scratch->rows_0x00C[i].table_0x00 = 0;
        scratch->rows_0x00C[i].model_0x04 = 0;
        scratch->rows_0x00C[i].dist_0x08 = 0.0f;
        scratch->rows_0x00C[i].rank_0x0C = -1;
    }
}

/* Steps the lobby target marker: hides it while a menu is up or outside the free lobby states, rebuilds its rows
 * and queues its draw. */
void lb_menu_marker_step(void) {
    LbMenuScratch* scratch = &lb_menu_scratch;
    u32 free_state = 0;

    if ((s32)lobby_w.state_0x000 == 0 || (s32)lobby_w.state_0x000 == 10 || (s32)lobby_w.state_0x000 == 20) {
        free_state = 1;
    }
    if (menu_busy_ck() == 1 || scratch->plw_0x000->flag_0x30E == 0 || free_state == 0) {
        scratch->hidden_0x155 = 1;
    } else {
        scratch->hidden_0x155 = 0;
    }
    scratch->timer_0x150++;
    if (event_demo_ck() == 0) {
        if ((lobby_w.sub_0x0B1 == 2 && lobby_w.page_0x052 != 0) || lobby_w.sub_0x0B1 == 1 || lobby_w.sub_0x0B1 == 3) {
            if (scratch->index_0x14C != -2) {
                sysSE_stop(2);
            }
            scratch->index_0x14C = -2;
            lb_menu_scratch_init(0);
        } else if (menu_busy_ck() == 0 && free_state == 1
                   && (scratch->plw_0x000->flag_0x30E != 0 || Pl_act_ck(scratch->plw_0x000, 0, 25) == 1)) {
            lb_menu_marker_update(scratch);
        }
    } else {
        scratch->hidden_0x155 = 1;
        lb_menu_scratch_init(0);
    }
    if (scratch->index_0x14C != -1 && scratch->hidden_0x155 == 0) {
        subTransSet((u32)lb_menu_marker_draw, 0, 0);
    }
}

/* Inserts `row` into the 20 sorted rows of `rows` (by rank, then by distance: nearest first below rank 3,
 * farthest first from it); returns its index. */
s32 lb_menu_row_insert(LbMenuRow* row, LbMenuRow* rows) {
    s32 i;
    s32 at;
    LbMenuRow* cur;

    if (rows->table_0x00 == 0 && rows->model_0x04 == 0) {
        at = 0;
    } else {
        at = 0;
        cur = rows;
        while (cur->table_0x00 != 0 || cur->model_0x04 != 0) {
            if (cur->rank_0x0C > row->rank_0x0C) {
                break;
            }
            if (row->rank_0x0C == cur->rank_0x0C) {
                if (row->rank_0x0C < 3) {
                    if (cur->dist_0x08 > row->dist_0x08) {
                        break;
                    }
                } else if (cur->dist_0x08 < row->dist_0x08) {
                    break;
                }
            }
            cur++;
            at++;
            if (at >= 19) {
                break;
            }
        }
        for (i = 19, cur = &rows[19]; i >= at; cur--, i--) {
            if (i != 0) {
                lb_menu_row16_copy(cur, &rows[i - 1]);
            }
        }
    }
    rows[at].table_0x00 = row->table_0x00;
    rows[at].model_0x04 = row->model_0x04;
    rows[at].dist_0x08 = row->dist_0x08;
    rows[at].rank_0x0C = row->rank_0x0C;
    return at;
}

/* Copies a 0x10-byte menu row field by field. */
void lb_menu_row16_copy(LbMenuRow* dst, const LbMenuRow* src) {
    dst->table_0x00 = src->table_0x00;
    dst->model_0x04 = src->model_0x04;
    dst->dist_0x08 = src->dist_0x08;
    dst->rank_0x0C = src->rank_0x0C;
}

/* The repeat check of `buttons`, while the game is ready or progress flag 99 is set. */
s32 lb_cmd_repeat_ready_ck(u16 buttons) {
    if (game_ready_ck() == 0 && userdata_progress_flag_ck(99) == 0) {
        return 0;
    }
    return lb_cmd_repeat_ck(buttons);
}

/* Ranks angle `angle` inside the facing cone `limit` on a 0-6 scale (3 per side), mirrored by the held stick
 * direction (4 or 8) so the rows on that side come first. */
s32 lb_menu_angle_rank_get(u32 angle, u32 limit) {
    if (lb_cmd_repeat_ready_ck(0xC) != 0) {
        if (lb_cmd_repeat_ready_ck(4) != 0) {
            if (angle < limit) {
                return (f32)angle * (1.0f / ((f32)limit / 3.0f));
            }
            return 6.0f - (s16)((f32)(-1 - angle) * (1.0f / ((f32)limit / 3.0f))) - 1.0f;
        }
        if (angle < limit) {
            return 6.0f - (s16)((f32)angle * (1.0f / ((f32)limit / 3.0f))) - 1.0f;
        }
        return (f32)(-1 - angle) * (1.0f / ((f32)limit / 3.0f));
    }
    if (angle < limit) {
        return (f32)angle * (1.0f / ((f32)limit / 3.0f));
    }
    return (f32)(-1 - angle) * (1.0f / ((f32)limit / 3.0f));
}

/* Returns the angle from `plw` to `pos` relative to `plw`'s heading. */
u32 lb_menu_actor_angle_get(_PLW* plw, VEC3* pos) {
    return calcVecAng2(&plw->vec_0x03C, pos) - plw->field_0x058;
}

/* The talk state (defined after its uses, so each use keeps its own address pair). */
LbTalkState lb_talk_state;

} /* extern "C" */
