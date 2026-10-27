/*
 * lobby/lb_companion_ui.cpp - the lobby companion/status UI band: the act dispatchers and their per-act handlers, the
 *   tutorial page block, the item-combine rules, the delivery menu, the page/dialog chain and its drawing helpers,
 *   and the quest cockpit's map icon tables.
 * RANGE. .text 0x80338808-0x8033F270 (133 functions); .data 0x805E27D4-0x805E7410 (from `lb_act_dispatch`'s switch
 *   table `jumptable_805E27D4`; the icon tables 0x805E6A58-0x805E70F8), .bss 0x806BE340-0x806BF0A0, .sdata
 *   0x80792D38-0x8079308C (the icon id lists of 8 bytes or less and their pointer arrays, 0x80792D80-0x80792F8C), .sbss
 *   0x80794B90-0x80794BE0, .sdata2 0x8079B2B8-0x8079B2C0, extab, extabindex.  The seam is undecidable from the DOL
 *   (docs/lobby.md).  The icon tables are defined ahead of the code; the lists `hud/cockpit_quest.cpp` and
 *   `ef/eft035.cpp` read at 0x805E6EF8-0x805E70C8 are not emitted, so the slot and flash id lists sit 0x1F0 bytes
 *   early.  The combine tables (.data 0x80603EA0-0x80604638, .sdata 0x80793A28-0x80793A67) are inside
 *   `menu/menu_placeinfo.cpp`'s claim and are read through `menu/lb_combo_recipe_tbl.h`.
 * FLAGS. `cflags_lobby`, file-scope `#pragma peephole off` (retail keeps `clrlwi` + `slwi` and
 *   `lobby_world_block + (i >> 3)` in a register; measured in docs/lobby.md) and `#pragma pool_data off` (retail
 *   addresses each delivery/screen-13 table with its own `lis`/`addi`; pooled, `lb_deli_window_draw` loads them off one
 *   base).
 * NAMES. The defined functions are named from their bodies.  GUESSes: the link-gated senders whose caller is
 *   unwritten, after the protocol sub-command they build (`lb_sub0a_send`, ...), and the icon tables (`<table>_area<n>`
 *   is entry n).  GUESS: `lb_sub16_send` (builds sub-command 0x16).  GUESS: `lobby_hunter_cards` (.bss 0x806BE340, ten
 *   0x130-byte records keyed by a hunter id at +0x03 with the hunter's 0x100-byte card at +0x24, which
 *   `updatePeerCardBlock` refreshes).  `hud_key_lookup` (0x8033A0BC) is the map's.  The dump's
 *   `homebutton::MotorCallback(OSAlarm...)` for 0x8033C4D8/0x8033C570 is not adopted: both are 0x24-byte refresh
 *   wrappers without an argument.  The game-root dispatcher at 0x80432154 calls `lb_act_dispatch`/`lb_act_dispatch_ex`
 *   as `(u8 index, LbActReq* req)`.  Module `lobby`: `lobby_w`, `lb_param_w`, `lb_deli_data`, 11 `LbStr` calls.
 *   GUESS (from each body and its callers): `lb_combo_item_known_ck`, `lb_combo_recipe_find`, `lb_page_row_pair_get`,
 *   `lb_combo_rate_get`, `lb_combo_recipe_known_ck`, `lb_combo_exec`, `lb_combo_rate_sprintf`, `lb_deli_list_build`,
 *   `lb_deli_menu_step`, `lb_deli_menu_draw`, `lb_combo_index_find`, `lb_combo_pair_recipe_find`,
 *   `lb_combo_recipe_open_ck`, `lb_combo_rate_find`, `lb_combo_recipe_learn`, `lb_tutorial_step`,
 *   `lb_tutorial_page_draw`, `lb_tutorial_note_draw`, `lb_tutorial_entry_find`, `lb_tutorial_page_text_get`,
 *   `lb_tutorial_work`, `lb_tutorial_entry_lists`, `tutorial_page_texts`, `tutorial_page_sets`,
 *   `tutorial_kind_sprite_ids`, `tutorial_end_sprite_ids`, `tutorial_end_sprite_ids_alt`.  Registry GUESSes for other
 *   units' rows: `pad_hold_set`/`pad_hold_clear`, `pl_my_lock_timer_set`, `pl_skill_have_ck`, the `lb_combo_*` tables.
 *   GUESS (the delivery menu, 0x8033B6C0-0x8033C36C): `lb_deli_entry_copy`, `lb_deli_item_have_ck`,
 *   `lb_deli_order_ready_ck`, `lb_deli_order_pay`, `lb_deli_cost_draw`, `lb_deli_window_draw`,
 *   `lb_area_select_screen_draw`, `lb_deli_work`, the `lb_deli_*` tables.  GUESS (screen 13, the companion page):
 *   `lb_item_gets_*`, `lb_skill_*`, `lb_entry_list_*`, `lb_dance_list_*` (the two "dances" are the pair at
 *   `LbDataBlock` +0x51A4, which `ef/eft050.cpp` reads as a clock - the reading is not settled), `lb_status_preview_update`,
 *   the `lb_s13_*` draw helpers and tables.  Registry GUESSes for other units' rows: `place_rec_item_set`,
 *   `place_rec_zenny_set`, `lb_npc_choice_draw`, `ai_npc_behaviour_get`, `lb_npc_entry_flag_set`, `lb_screen13_draw`,
 *   `ai_npc_skill_group_get`, `ai_npc_pair_values_get`, `ai_npc_level_rank_get`.
 * RESIDUALS. Every row is written.
 *  - `lb_entry_flags_clear`: retail materialises both `.sbss` bases before the first store, ours folds the first into
 *    its `stb`'s sda21 operand (a `u8* seen = lbl_80794B90;` local does not change it);
 *  - `lb_act_value_apply`, `lb_act_limit_set`, `lb_act_slot_write`: retail reads the request's +0x4 (+0x9) where ours
 *    reads +0x8 (+0x7);
 *  - `lb_act_row_apply`: retail loads +0x6 with `lhz`, ours `lbz` + `clrlwi` (a field width);
 *  - `lb_act_award_handover`, `lb_act_slot_write`: ours emits a `clrlwi` or `extsb` retail does not;
 *  - `lb_act_announce`: ours adds one `b`; `lb_act_handover`: ours saves one register fewer and its branch targets
 *    shift; `lb_name_tail_copy`: the word copies' load/store order;
 *  - `lb_entry_start_send`, `lb_act_entry_start`: the companion work's parameter bytes and the request's +0x18..+0x1B;
 *  - `lb_entry_model_publish`: a byte against a word store;
 *  - `lb_combo_rate_get`: `userdata_item_slot_count`'s `s16` result is sign-extended where retail keeps it (the owner
 *    header's width), and the item box capacity's `clrlwi` is scheduled after the item load;
 *  - `lb_skill_unlock_announce`: retail walks `lb_skill_unlock_items` with two pointers (one for the `need_0x00` test,
 *    one for the other fields), ours with one (an indexed loop and `s16`/`s32`/`u8` counters measure the same or worse);
 *  - `lb_s13_dance_values_draw`: retail keeps the pair index in its own register beside the `lb_s13_value_pairs` base,
 *    ours strength-reduces it into a pointer (a `[4][2]` table measures worse);
 *  - `lb_s13_skill_list_draw`, `lb_s13_entry_list_draw`: one argument load scheduled before the flag test, and the
 *    callee-saved colours (the best declaration orders are applied);
 *  - `lb_seen_pad_ck`, `lb_act_row_update`, `lb_act_best_keep`, `lb_triplet_value2_get`, `lb_triplet_value1_get`,
 *    `lb_page_bits_set`, `lb_tutorial_note_draw`, `lb_dance_list_step`, `lb_s13_slot_list_draw`: register colouring.
 *  - `lb_deli_cost_draw`'s `"%d"` (retail `lbl_80793018`) merges with `lb_combo_rate_sprintf`'s (`lbl_80793008`) in
 *    ours, so `.sdata` carries one copy where retail has two;
 *  - `lb_status_preview_update` builds `LbCompanionSetup`, the layout of `ai/ai_npc.h`'s `AINPCSetup`, because that
 *    header's `_HIT_W` collides with `Pl/hit_w.h`'s beside this unit's includes.
 *   flipcheck: `.bss`/`.sbss`/`.sdata2` claimed, not emitted (only `lb_deli_work`); `.data`/`.sdata` short of the
 *   claim (the delivery, screen-13 and draw tables are emitted, byte-identical; the act switch tables, the icon lists
 *   0x805E6EF8-0x805E70C8 and `lb_s13_status_icon_ids` are not); `.text`/extab/extabindex short of the claim; the
 *   `.sdata`/`.sdata2` pool is shared with `hud/cockpit_quest.cpp` and `menu/menu_placeinfo.cpp` (a low-confidence fold
 *   candidate); the source still spells `my_player_no`, `enemy_data_find`, `eft_slot_persist_ck` and `eft_slot_spawn`
 *   by their old stems `fn_800CF384`, `fn_803438E4`, `fn_80343B44`, `fn_80343B74`.
 * SHAPES. Declaration order decides the callee-saved colours of `lb_combo_rate_get`/`lb_combo_exec` (playbook 63) and
 *   of the screen-13 helpers; `lb_combo_exec`'s yield roll is a `for (;;)` with the test first (retail does not rotate
 *   it); a draw helper declares its layout base before the row position (retail puts the base at +0xC); the two-row
 *   loops are `for` loops (a `do`/`while` keeps an `extsh`/`clrlwi` on the test).
 */

#include "types.h"
#include "hud/cockpit_icon_data.h"

/* ---- .sdata ---- */
u16 quest_icon_ids_map1_area0[] = { 0x1087, 0xFFFF };
u16 quest_icon_ids_map1_area1[] = { 0x1088, 0x108C, 0xFFFF };
u16 quest_icon_ids_quest1_area2[] = { 0x1085, 0x1089, 0xFFFF };
u16 quest_icon_ids_quest2_area2[] = { 0x1085, 0x1089, 0x109D, 0xFFFF };
u16 quest_icon_ids_quest1_area3[] = { 0x1082, 0x1084, 0x108D, 0xFFFF };
u16 quest_icon_ids_map1_area4[] = { 0x108F, 0x109B, 0x10A1, 0xFFFF };
u16 quest_icon_ids_quest2_area4[] = { 0x108F, 0x109B, 0xFFFF };
u16 quest_icon_ids_map1_area5[] = { 0x108A, 0x109A, 0x10A4, 0xFFFF };
u16 quest_icon_ids_quest2_area5[] = { 0x108A, 0x109A, 0xFFFF };
u16 quest_icon_ids_map1_area6[] = { 0x1097, 0x10A7, 0xFFFF };
u16 quest_icon_ids_map1_area7[] = { 0x10A2, 0x10A8, 0x10AE, 0xFFFF };
u16 quest_icon_ids_map1_area8[] = { 0x10AC, 0xFFFF };
u16 quest_icon_ids_map1_area9[] = { 0x1092, 0x1095, 0x109E, 0xFFFF };
u16 quest_icon_ids_quest2_area9[] = { 0x1095, 0x109E, 0xFFFF };
u16 quest_icon_ids_map1_area10[] = { 0x1096, 0x109F, 0x10AA, 0xFFFF };
u16 quest_icon_ids_map1_area11[] = { 0x108E, 0x1099, 0xFFFF };
u16 quest_icon_ids_map1_area12[] = { 0x10A5, 0x10AB, 0xFFFF };
u16 quest_icon_ids_empty[] = { 0xFFFF };
u16 quest_icon_ids_map2_area0[] = { 0x10C0, 0xFFFF };
u16 quest_icon_ids_map2_area1[] = { 0x10BE, 0x10C4, 0x10CB, 0xFFFF };
u16 quest_icon_ids_map2_area2[] = { 0x10C1, 0x10C8, 0x10DA, 0xFFFF };
u16 quest_icon_ids_map2_area3[] = { 0x10C5, 0x10CC, 0xFFFF };
u16 quest_icon_ids_map2_area11[] = { 0x10CE, 0x10D2, 0x10DC, 0xFFFF };
u16 quest_icon_ids_map2_area6[] = { 0x10CF, 0x10E5, 0x10E9, 0xFFFF };
u16 quest_icon_ids_map2_area7[] = { 0x10D4, 0x10E1, 0x10EA, 0xFFFF };
u16 quest_icon_ids_map2_area10[] = { 0x10DE, 0x10EB, 0xFFFF };
u16 quest_icon_ids_map3_area0[] = { 0x10FE, 0xFFFF };
u16 quest_icon_ids_map3_area5[] = { 0x10FF, 0x110A, 0xFFFF };
u16 quest_icon_ids_map3_area2[] = { 0x1100, 0x110B, 0xFFFF };
u16 quest_icon_ids_map3_area4[] = { 0x110C, 0x1113, 0xFFFF };
u16 quest_icon_ids_map3_area8[] = { 0x1110, 0x111C, 0xFFFF };
u16 quest_icon_ids_map3_area6[] = { 0x1102, 0x1115, 0x1120, 0xFFFF };
u16 quest_icon_ids_map3_area9[] = { 0x1119, 0x111D, 0x1123, 0xFFFF };
u16 quest_icon_ids_map3_area10[] = { 0x1121, 0xFFFF };
u16 quest_icon_ids_map4_area0[] = { 0x1133, 0xFFFF };
u16 quest_icon_ids_map4_area1[] = { 0x1131, 0x1137, 0x113C, 0xFFFF };
u16 quest_icon_ids_map4_area6[] = { 0x1139, 0x113E, 0x114C, 0xFFFF };
u16 quest_icon_ids_map4_area4[] = { 0x1148, 0x114D, 0xFFFF };
u16 quest_icon_ids_map4_area7[] = { 0x1145, 0x114E, 0xFFFF };
u16 quest_icon_ids_map5_area0[] = { 0x115E, 0x1162, 0xFFFF };
u16 quest_icon_ids_map5_area2[] = { 0x115B, 0x1163, 0x1166, 0xFFFF };
u16 quest_icon_ids_map5_area1[] = { 0x115C, 0x115F, 0x116A, 0xFFFF };
u16 quest_icon_ids_map5_area3[] = { 0x1160, 0x116B, 0x116E, 0xFFFF };
u16 quest_icon_ids_map5_area6[] = { 0x1164, 0x1167, 0x1177, 0xFFFF };
u16 quest_icon_ids_map5_area5[] = { 0x116F, 0x1179, 0xFFFF };
u16 quest_icon_ids_map5_area8[] = { 0x1171, 0x117A, 0x1180, 0xFFFF };
u16 quest_icon_ids_map5_area10[] = { 0x117E, 0x1183, 0xFFFF };
u16 quest_icon_ids_map5_area9[] = { 0x1172, 0x1181, 0xFFFF };
u16 quest_icon_ids_map6_area0[] = { 0x1192, 0xFFFF };
u16 quest_icon_ids_map7_area0[] = { 0x119C, 0xFFFF };
u16 quest_icon_ids_map7_area1[] = { 0x119A, 0x119F, 0xFFFF };
u16 quest_icon_ids_map7_area2[] = { 0x119D, 0x11A2, 0xFFFF };
u16 quest_icon_ids_map7_area3[] = { 0x11A0, 0xFFFF };
u16 quest_icon_ids_map8_area0[] = { 0x11AA, 0xFFFF };
u16* quest_area_icon_lists_map8[2] = { quest_icon_ids_map8_area0, 0 };
u16 quest_icon_ids_map10_area0[] = { 0x11BC, 0xFFFF };
u16* quest_area_icon_lists_map10[2] = { quest_icon_ids_map10_area0, 0 };
u16 quest_icon_ids_map11_area0[] = { 0x11C2, 0xFFFF };
u16* quest_area_icon_lists_map11[2] = { quest_icon_ids_map11_area0, 0 };
u16 quest_map_icon_ids_map6[] = { 0x1190, 0x1191, 0x1193, 0xFFFF };
u16 quest_map_icon_ids_map8[] = { 0x11A8, 0x11A9, 0xFFFF };
u16 quest_pit_area_icon_ids_area0[] = { 0x11AE, 0x11AF, 0xFFFF };
u16 quest_pit_area_icon_ids_area1[] = { 0x11B4, 0x11B5, 0xFFFF };
u16 quest_map_icon_ids_map10[] = { 0x11BA, 0x11BB, 0xFFFF };
u16 quest_map_icon_ids_map11[] = { 0x11C0, 0x11C1, 0xFFFF };
u16* quest_pit_area_icon_lists[2] = { quest_pit_area_icon_ids_area0, quest_pit_area_icon_ids_area1 };
u16 quest_area_sprite_ids_map6[] = { 0x1195, 0x1196, 0x1197 };
u16 quest_area_sprite_ids_map7[] = { 0x11A3, 0x11A4, 0x11A5, 0x11A6 };
u16 quest_area_sprite_ids_map8[] = { 0x11AB, 0x11AC };
u16 quest_area_sprite_ids_map10[] = { 0x11BD, 0x11BE };
u16 quest_area_sprite_ids_map11[] = { 0x11C3, 0x11C4 };
/* ---- .data ---- */
u16 quest_icon_ids_map1_area2[] = { 0x1085, 0x1089, 0x1091, 0x109D, 0xFFFF };
u16 quest_icon_ids_map1_area3[] = { 0x1082, 0x1084, 0x108D, 0x1094, 0xFFFF };
u16* quest_area_icon_lists_map1[13] = { quest_icon_ids_map1_area0, quest_icon_ids_map1_area1, quest_icon_ids_map1_area2, quest_icon_ids_map1_area3, quest_icon_ids_map1_area4, quest_icon_ids_map1_area5, quest_icon_ids_map1_area6, quest_icon_ids_map1_area7, quest_icon_ids_map1_area8, quest_icon_ids_map1_area9, quest_icon_ids_map1_area10, quest_icon_ids_map1_area11, quest_icon_ids_map1_area12 };
u16* quest_area_icon_lists_quest1[13] = { quest_icon_ids_map1_area0, quest_icon_ids_map1_area1, quest_icon_ids_quest1_area2, quest_icon_ids_quest1_area3, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty };
u16* quest_area_icon_lists_quest2[13] = { quest_icon_ids_map1_area0, quest_icon_ids_map1_area1, quest_icon_ids_quest2_area2, quest_icon_ids_map1_area3, quest_icon_ids_quest2_area4, quest_icon_ids_quest2_area5, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_quest2_area9, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty };
u16* quest_area_icon_lists_quest3[13] = { quest_icon_ids_map1_area0, quest_icon_ids_map1_area1, quest_icon_ids_map1_area2, quest_icon_ids_map1_area3, quest_icon_ids_quest2_area4, quest_icon_ids_quest2_area5, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_empty, quest_icon_ids_map1_area9, quest_icon_ids_empty, quest_icon_ids_map1_area11, quest_icon_ids_empty };
u16 quest_icon_ids_map2_area4[] = { 0x10C2, 0x10C9, 0x10D1, 0x10D6, 0x10E2, 0xFFFF };
u16 quest_icon_ids_map2_area5[] = { 0x10CD, 0x10D8, 0x10DB, 0x10E4, 0xFFFF };
u16 quest_icon_ids_map2_area9[] = { 0x10C6, 0x10D3, 0x10D7, 0x10E8, 0xFFFF };
u16 quest_icon_ids_map2_area8[] = { 0x10DD, 0x10E0, 0x10E6, 0x10ED, 0xFFFF };
u16* quest_area_icon_lists_map2[12] = { quest_icon_ids_map2_area0, quest_icon_ids_map2_area1, quest_icon_ids_map2_area2, quest_icon_ids_map2_area3, quest_icon_ids_map2_area4, quest_icon_ids_map2_area5, quest_icon_ids_map2_area6, quest_icon_ids_map2_area7, quest_icon_ids_map2_area8, quest_icon_ids_map2_area9, quest_icon_ids_map2_area10, quest_icon_ids_map2_area11 };
u16 quest_icon_ids_map3_area1[] = { 0x10FC, 0x1104, 0x1107, 0x1117, 0x111B, 0xFFFF };
u16 quest_icon_ids_map3_area3[] = { 0x1105, 0x1108, 0x110F, 0x1112, 0x1118, 0xFFFF };
u16 quest_icon_ids_map3_area7[] = { 0x1101, 0x110D, 0x1114, 0x111F, 0xFFFF };
u16* quest_area_icon_lists_map3[11] = { quest_icon_ids_map3_area0, quest_icon_ids_map3_area1, quest_icon_ids_map3_area2, quest_icon_ids_map3_area3, quest_icon_ids_map3_area4, quest_icon_ids_map3_area5, quest_icon_ids_map3_area6, quest_icon_ids_map3_area7, quest_icon_ids_map3_area8, quest_icon_ids_map3_area9, quest_icon_ids_map3_area10 };
u16 quest_icon_ids_map4_area3[] = { 0x1134, 0x113D, 0x1140, 0x114B, 0xFFFF };
u16 quest_icon_ids_map4_area2[] = { 0x1135, 0x1138, 0x1141, 0x1144, 0xFFFF };
u16 quest_icon_ids_map4_area5[] = { 0x113A, 0x1142, 0x1146, 0x1149, 0xFFFF };
u16* quest_area_icon_lists_map4[10] = { quest_icon_ids_map4_area0, quest_icon_ids_map4_area1, quest_icon_ids_map4_area2, quest_icon_ids_map4_area3, quest_icon_ids_map4_area4, quest_icon_ids_map4_area5, quest_icon_ids_map4_area6, quest_icon_ids_map4_area7, 0, 0 };
u16 quest_icon_ids_map5_area4[] = { 0x1168, 0x1175, 0x1178, 0x117C, 0xFFFF };
u16 quest_icon_ids_map5_area7[] = { 0x116C, 0x1170, 0x1174, 0x117D, 0xFFFF };
u16* quest_area_icon_lists_map5[11] = { quest_icon_ids_map5_area0, quest_icon_ids_map5_area1, quest_icon_ids_map5_area2, quest_icon_ids_map5_area3, quest_icon_ids_map5_area4, quest_icon_ids_map5_area5, quest_icon_ids_map5_area6, quest_icon_ids_map5_area7, quest_icon_ids_map5_area8, quest_icon_ids_map5_area9, quest_icon_ids_map5_area10 };
u16* quest_area_icon_lists_map6[3] = { quest_icon_ids_map6_area0, 0, 0 };
u16* quest_area_icon_lists_map7[4] = { quest_icon_ids_map7_area0, quest_icon_ids_map7_area1, quest_icon_ids_map7_area2, quest_icon_ids_map7_area3 };
u16** quest_area_icon_lists_by_map[21] = { 0, quest_area_icon_lists_map1, quest_area_icon_lists_map2, quest_area_icon_lists_map3, quest_area_icon_lists_map4, quest_area_icon_lists_map5, quest_area_icon_lists_map6, quest_area_icon_lists_map7, quest_area_icon_lists_map8, 0, quest_area_icon_lists_map10, quest_area_icon_lists_map11, quest_area_icon_lists_map1, quest_area_icon_lists_map2, quest_area_icon_lists_map3, quest_area_icon_lists_map4, quest_area_icon_lists_map5, quest_area_icon_lists_map6, quest_area_icon_lists_map7, quest_area_icon_lists_map8, quest_area_icon_lists_map11 };
u16** quest_area_icon_lists_by_quest[3] = { quest_area_icon_lists_quest1, quest_area_icon_lists_quest2, quest_area_icon_lists_quest3 };
u16 quest_map_icon_ids_map1[] = { 0x1081, 0x1083, 0x108B, 0x1086, 0x109C, 0x1093, 0x10A3, 0x10A9, 0x10AD, 0x1098, 0x10A0, 0x1090, 0x10A6, 0xFFFF };
u16 quest_map_icon_ids_quest1[] = { 0x1081, 0x1083, 0x108B, 0x1086, 0xFFFF };
u16 quest_map_icon_ids_quest2[] = { 0x1081, 0x1083, 0x108B, 0x1086, 0x109C, 0x1093, 0x0000, 0x0000, 0x0000, 0x1098, 0xFFFF };
u16 quest_map_icon_ids_quest3[] = { 0x1081, 0x1083, 0x108B, 0x1086, 0x109C, 0x1093, 0x0000, 0x0000, 0x0000, 0x1098, 0x0000, 0x1090, 0xFFFF };
u16 quest_map_icon_ids_map2[] = { 0x10BD, 0x10BF, 0x10C3, 0x10C7, 0x10CA, 0x10D0, 0x10DF, 0x10E3, 0x10E7, 0x10D9, 0x10EC, 0x10D5, 0xFFFF };
u16 quest_map_icon_ids_map3[] = { 0x10FB, 0x10FD, 0x1106, 0x1109, 0x110E, 0x1103, 0x111A, 0x1116, 0x1111, 0x111E, 0x1122, 0xFFFF };
u16 quest_map_icon_ids_map4[] = { 0x1130, 0x1132, 0x113B, 0x1136, 0x1143, 0x114A, 0x113F, 0x1147, 0xFFFF };
u16 quest_map_icon_ids_map5[] = { 0x115A, 0x1161, 0x115D, 0x1165, 0x116D, 0x1173, 0x1169, 0x1176, 0x117B, 0x1182, 0x117F, 0xFFFF };
u16 quest_map_icon_ids_map7[] = { 0x1199, 0x119B, 0x119E, 0x11A1, 0xFFFF };
u16* quest_map_icon_list_by_map[21] = { 0, quest_map_icon_ids_map1, quest_map_icon_ids_map2, quest_map_icon_ids_map3, quest_map_icon_ids_map4, quest_map_icon_ids_map5, quest_map_icon_ids_map6, quest_map_icon_ids_map7, quest_map_icon_ids_map8, 0, quest_map_icon_ids_map10, quest_map_icon_ids_map11, quest_map_icon_ids_map1, quest_map_icon_ids_map2, quest_map_icon_ids_map3, quest_map_icon_ids_map4, quest_map_icon_ids_map5, quest_map_icon_ids_map6, quest_map_icon_ids_map7, quest_map_icon_ids_map8, quest_map_icon_ids_map11 };
u16* quest_map_icon_list_by_quest[3] = { quest_map_icon_ids_quest1, quest_map_icon_ids_quest2, quest_map_icon_ids_quest3 };
u16 quest_area_sprite_ids_map1[] = { 0x10AF, 0x10B0, 0x10B2, 0x10B1, 0x10B6, 0x10B4, 0x10B8, 0x10BA, 0x10BB, 0x10B5, 0x10B7, 0x10B3, 0x10B9 };
u16 quest_area_sprite_ids_map2[] = { 0x10EE, 0x10EF, 0x10F0, 0x10F1, 0x10F2, 0x10F3, 0x10F6, 0x10F7, 0x10F8, 0x10F5, 0x10F9, 0x10F4 };
u16 quest_area_sprite_ids_map3[] = { 0x1124, 0x1125, 0x1127, 0x1128, 0x112A, 0x1126, 0x112C, 0x112B, 0x1129, 0x112D, 0x112E };
u16 quest_area_sprite_ids_map4[] = { 0x114F, 0x1150, 0x1152, 0x1151, 0x1154, 0x1156, 0x1153, 0x1155, 0x1157, 0x1158 };
u16 quest_area_sprite_ids_map5[] = { 0x1184, 0x1186, 0x1185, 0x1187, 0x1189, 0x118A, 0x1188, 0x118B, 0x118C, 0x118E, 0x118D };
u16* quest_area_sprite_list_by_map[21] = { 0, quest_area_sprite_ids_map1, quest_area_sprite_ids_map2, quest_area_sprite_ids_map3, quest_area_sprite_ids_map4, quest_area_sprite_ids_map5, quest_area_sprite_ids_map6, quest_area_sprite_ids_map7, quest_area_sprite_ids_map8, 0, quest_area_sprite_ids_map10, quest_area_sprite_ids_map11, quest_area_sprite_ids_map1, quest_area_sprite_ids_map2, quest_area_sprite_ids_map3, quest_area_sprite_ids_map4, quest_area_sprite_ids_map5, quest_area_sprite_ids_map6, quest_area_sprite_ids_map7, quest_area_sprite_ids_map8, quest_area_sprite_ids_map11 };
u16 cockpit_slot_row_sprite_ids[] = { 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25 };
u16 cockpit_slot_column_sprite_ids[] = { 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0x0E4F, 0x0E50, 0x0E51, 0xFFFF };
u16 cockpit_flash_anim_ids[] = { 0x0BCF, 0x0BD0, 0x0BD1, 0x0BD3, 0x0BD4, 0xFFFF };
#include "lobby/lb_companion_ui.h"
#include "quest/quest_result_enter.h"  /* quest_reward_faint_penalty, quest_result_enter, quest_start_enter (rule 2) */
#include "quest/quest_item_slot.h"     /* quest_item_pair_copy_row (rule 2) */
#include "menu/quest_str_tbl_35_get.h"  /* quest_str_tbl_35_get (rule 2) */
#include "ef/eft052.h"               /* hud_msg_push (rule 2) */
#include "Pl/pl_item_add.h"          /* pl_item_add (rule 2) */
#include "lobby/lb_quest_screen.h"   /* quest_time_limit_set, quest_element_done_mark (rule 2) */
#include "enemy/em_pop.h"            /* quest_score_deduct (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "mh3_pad/Psw.h"              /* Psw (rule 2) */
#include "lobby/event_demo_running_ck.h" /* event_demo_running_ck (rule 2) */
#include "pad_hold_set.h"             /* pad_hold_set, pad_hold_clear (rule 2) */
#include "Pl/pl_input_lock_set.h"  /* pl_my_lock_timer_set (rule 2) */
#include "menu/note_slot_cursor_step.h" /* note_slot_cursor_step, note_slot_draw_row (rule 2) */
#include "menu/item_page_draw_option_frame.h" /* item_page_draw_option_frame (rule 2) */
#include "ai/fn_802D44F4.h"           /* ainpc_entry_update_all, ainpc_entry_draw_all (rule 2) */
#include "fn_80047398.h"              /* subTransSetPrio (rule 2) */
#include "hud/draw_spin_icon.h"       /* draw_spin_icon (rule 2) */
#include "userdata_progress_flag_ck.h" /* userdata_progress_flag_ck (rule 2) */
#include "menu/menu_item.h"           /* GetItemData, ItemDataRecord (rule 2) */
#include "menu/lb_combo_recipe_tbl.h"  /* the combine tables (rule 2) */
#include "menu/lb_combo_yield_tbl.h"   /* lb_combo_yield_tbl (rule 2) */
#include "item_box_take.h"            /* item_box_slot_take, item_box_take (rule 2) */
#include "Pl/pl_skill_have_ck.h"      /* pl_skill_have_ck (rule 2) */
#include "Pl/pl_skill.h"              /* Pl_cat_skill_ck, pl_item_slot_find (rule 2) */
#include "MSL_C/printf.h"             /* sprintf */
#include "ef/GameMode_ck.h"          /* GameMode_ck (rule 2) */
#include "sound/se_guide_voice_play.h" /* se_guide_voice_play, sysSE_window_open_play (rule 2) */
#include "Pl/quest_move_idle_ck.h"    /* quest_move_idle_ck (rule 2) */
#include "lobby/lb_event_request.h"   /* this unit's own leaf header */
#include "ai/ainpc_move.h"           /* ai_skill_mask_get (rule 2) */
#include "lobby/lb_deli_list_build.h" /* this unit's own leaf header */
#include "lobby/lb_list_init.h"       /* lb_choice_init, lb_choice_step, lb_panel_*, lb_page_arrow_draw (rule 2) */
#include "lobby/lb_cmd_pressed_ck.h"  /* lb_cmd_pressed_ck, lb_cmd_repeat_ck, lobby_cmd_trig_get (rule 2) */
#include "lobby/lb_talk_page_open.h"  /* lb_talk_page_open, lb_talk_page_mode_reset (rule 2) */
#include "lobby/lb_unlock_cond_ck.h"  /* lb_unlock_cond_ck (rule 2) */
#include "lobby/lb_progress_unlock.h" /* lb_progress_unlock (rule 2) */
#include "lobby/lb_npc_choice_draw.h" /* lb_npc_choice_draw (rule 2) */
#include "menu/menu_message.h"        /* menu_cursor_step_forward, toggle_word_step (rule 2) */
#include "menu/menu_row.h"            /* PlaceRec, place_rec_alloc_idx (rule 2) */
#include "menu/place_rec_item_set.h"  /* place_rec_item_set, place_rec_zenny_set (rule 2) */
#include "menu/sysmsg_text.h"         /* lb_deli_data, lb_deli_list_data (rule 2) */
#include "camera/camera.h"            /* camera_talk_lock_set (rule 2) */
#include "hud/ainpc_page_mark_b_draw.h" /* ainpc_page_mark_b_draw (rule 2) */
#include "Runtime.PPCEABI.H/memset.h" /* memset */
#include "Runtime.PPCEABI.H/memcpy.h" /* memcpy */
#include "lobby/lobby_w.h"            /* lobby_w, LbLobbyWork (rule 2) */
#include "lobby/lb_screen13_step.h"   /* this unit's own leaf header */
#include "lobby/lb_screen13_open.h"   /* this unit's own leaf header */
#include "lobby/lb_menu_step.h"       /* lb_menu_step (rule 2) */
#include "lobby/lb_npc_entry_flag_set.h" /* lb_npc_entry_flag_set (rule 2) */
#include "lobby/lb_npc.h"             /* lb_item_get_data (rule 2) */
#include "ef/eft052_box_open.h"       /* eft052_box_open (rule 2) */
#include "ef/eft052_item_get_open.h"  /* eft052_item_get_step (rule 2) */
#include "ef/lb_screen13_draw.h"      /* lb_screen13_draw (rule 2) */
#include "hud/ainpc_page_hold_set.h"  /* ainpc_page_hold_set (rule 2) */
#include "ai/ai_npc_status_preview.h" /* ai_npc_status_preview (rule 2) */
#include "ai/ai_npc_behaviour_get.h"  /* ai_npc_behaviour_get (rule 2) */
#include "ai/ai_npc_skill_group_get.h" /* ai_npc_skill_group_get (rule 2) */
#include "ai/ai_npc_pair_values_get.h" /* ai_npc_pair_values_get (rule 2) */
#include "ai/ai_npc_level_rank_get.h" /* ai_npc_level_rank_get (rule 2) */
#include "lobby/lb_group_row_draw.h"  /* lb_group_row_draw, lb_group_color_get (rule 2) */
#include "userdata_item.h"            /* userdata_item_count_total (rule 2) */
#include "get_FqResult_work.h"        /* userdata_zenny_add (rule 2) */
#include "sound/fn_800D7F54.h"        /* sysSE_stop (rule 2) */

/* ---- the delivery menu's tables (.data, .sdata) and work block (.bss) ---- */
LbChoiceTmpl lb_deli_choice_tmpl = { 1, 0, 0, 0x117, 4, NULL, 0, 3 };
u16 lb_deli_cost_sprite_ids[6] = { 0x1E9B, 0x1E9C, 0x1EA3, 0x1E9A, 0x1EA2, 0xFFFF };
u16 lb_deli_window_sprite_ids[28] = {
    0x1E7D, 0x1E7E, 0x1E7F, 0x1E80, 0x1E81, 0x1E82, 0x1E83, 0x1E84, 0x1E85, 0x1E86, 0x1E87, 0x1E88,
    0x1E89, 0x1E72, 0x1E73, 0x1E74, 0x1E75, 0x1E76, 0x1E77, 0x1E78, 0x1E79, 0x1E7A, 0x1E7B, 0x1E7C, 0xFFFF,
};
u16 lb_deli_title_sprite_ids[8] = { 0x1E8B, 0x1E8F, 0x1E90, 0x1E91, 0x1E92, 0x1E93, 0x1E94, 0xFFFF };
u16 lb_deli_cost_frame_sprite_ids[16] = {
    0x1EA8, 0x1EA9, 0x1EAA, 0x1EAB, 0x1EAC, 0x1EAD, 0x1EAE, 0x1EAF, 0x1EB0, 0x1EA5, 0x1EA6, 0x1EA7, 0xFFFF,
};
u16 lb_deli_cost_layout_ids[4] = { 0x1EB1, 0x1EB2, 0x1EB3, 0x1EB4 };
LbDeliWork lb_deli_work;

/* ---- screen 13's tables (.data, .sdata) ---- */
LbSkillFlag lb_skill_unlock_flags[19] = {
    { 0x115, 0x01 }, { 0x116, 0x06 }, { 0x117, 0x07 }, { 0x118, 0x08 }, { 0x119, 0x09 }, { 0x11A, 0x0A },
    { 0x11B, 0x0B }, { 0x11C, 0x0C }, { 0x11D, 0x0D }, { 0x11E, 0x0E }, { 0x11F, 0x0F }, { 0x120, 0x10 },
    { 0x121, 0x11 }, { 0x122, 0x12 }, { 0x127, 0x17 }, { 0x128, 0x18 }, { 0x129, 0x19 }, { 0x12A, 0x1A },
    { 0xFFFF, 0 },
};
LbSkillUnlock lb_skill_unlock_items[6] = {
    { 0x0B, 0x124, 0x14 }, { 0x09, 0x123, 0x13 }, { 0x0C, 0x126, 0x16 }, { 0x0A, 0x125, 0x15 }, { 0x0D, 0x12B, 0x1B },
    { 0xFFFF, 0, 0 },
};
u16 lb_entry_unlock_flags[8] = { 0xFFFF, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C };
u8 lb_skill_group_a[12] = { 2, 3, 4, 5, 6, 8, 9, 10 };
u8 lb_skill_needs_7[8] = { 2, 3, 4, 5, 6 };
u8 lb_skill_needs_11[4] = { 8, 9, 10 };
u8 lb_skill_group_b[4] = { 0x13, 0x14 };
u8 lb_skill_group_c[4] = { 0x15, 0x16 };

/* ---- screen 13's draw tables (.data, .sdata) ---- */
u16 lb_subpage_arrow_sprite_ids[6] = { 0x1EB7, 0x1EB8, 0x1EBA, 0x1EBB, 0x1EB9, 0xFFFF };
u16 lb_s13_frame_sprite_ids[10] = { 0x1EFA, 0x1EFB, 0x1EFC, 0x1EF8, 0x1EF9, 0x1EF4, 0x1EF5, 0x1EF6, 0x1EF7, 0xFFFF };
u16 lb_s13_row_layout_ids[8] = { 0x1F14, 0x1F15, 0x1F16, 0x1F17, 0x1F18, 0x1F19, 0x1F1A };
u16 lb_s13_slot_frame_sprite_ids[16] = {
    0x1EFD, 0x1EFE, 0x1EFF, 0x1F00, 0x1F01, 0x1F02, 0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07, 0x1EF9, 0x1EF6, 0x1EF7, 0xFFFF,
};
u16 lb_s13_help_sprite_ids[26] = {
    0x1F2B, 0x1F2C, 0x1F2D, 0x1F2E, 0x1F2F, 0x1F30, 0x1F31, 0x1F32, 0x1F33, 0x1F34, 0x1F35, 0x1F36, 0x1F37,
    0x1F38, 0x1F39, 0x1F3A, 0x1F3B, 0x1F3C, 0x1F3D, 0x1F3E, 0x1F3F, 0x1F40, 0x1F41, 0x1F42, 0x1F43, 0xFFFF,
};
u16 lb_s13_status_sprite_ids[50] = {
    0x1F57, 0x1F58, 0x1F59, 0x1F5A, 0x1F5B, 0x1F5C, 0x1F5D, 0x1F5E, 0x1F5F, 0x1F60, 0x1F61, 0x1F82, 0x1F83,
    0x1F84, 0x1F69, 0x1F68, 0x1F62, 0x1F63, 0x1F64, 0x1F65, 0x1F66, 0x1F67, 0x1F6A, 0x1F6B, 0x1F6C, 0x1F6D,
    0x1F6E, 0x1F6F, 0x1F70, 0x1F71, 0x1F72, 0x1F73, 0x1F74, 0x1F75, 0x1F76, 0x1F77, 0x1F78, 0x1F79, 0x1F7A,
    0x1F7B, 0x1F7C, 0x1F7D, 0x1F7E, 0x1F7F, 0x1F80, 0x1F81, 0x1F54, 0x1F55, 0xFFFF,
};
u16 lb_s13_skill_badge_sprite_ids[6] = { 0x1ED7, 0x1ED8, 0x1EDA, 0x1EDB, 0xFFFF };
u16 lb_s13_pick_sprite_ids[32] = {
    0x1FA5, 0x1F90, 0x1F8F, 0x1F91, 0x1F8C, 0x1F8D, 0x1F8E, 0x1F8A, 0x1F8B, 0x1F92, 0x1F93, 0x1F94, 0x1F95,
    0x1F96, 0x1F97, 0x1F98, 0x1F99, 0x1F9A, 0x1F9B, 0x1F9C, 0x1F9D, 0x1F9E, 0x1F9F, 0x1FA0, 0x1FA1, 0x1FA2,
    0x1FA4, 0x1FA6, 0x1FA7, 0x1FA8, 0x1F89, 0xFFFF,
};
u16 lb_s13_value_sprite_ids[14] = {
    0x1FC5, 0x1FC6, 0x1FBB, 0x1FBC, 0x1FBD, 0x1FBE, 0x1FBF, 0x1FC0, 0x1FC1, 0x1FC2, 0x1FC3, 0x1FC4, 0xFFFF,
};
u16 lb_s13_value_row_sprite_ids[10] = { 0x1FB5, 0x1FB6, 0x1FB7, 0x1FB8, 0x1FB9, 0x1FB2, 0x1FB3, 0x1FB4, 0xFFFF };
u16 lb_s13_option_sprite_ids[6] = { 0x1EE2, 0x1EE3, 0x1EE4, 0x1EE6, 0xFFFF };
u16 lb_s13_confirm_sprite_ids[8] = { 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0xFFFF };
u16 lb_s13_action_sprite_ids[12] = {
    0x1F45, 0x1F46, 0x1F47, 0x1F48, 0x1F49, 0x1F4A, 0x1F4B, 0x1F4C, 0x1F4D, 0x1F4E, 0xFFFF,
};
u16 lb_s13_info_sprite_ids[22] = {
    0x1FCD, 0x1FCE, 0x1FCF, 0x1FD0, 0x1FD1, 0x1FD2, 0x1FD3, 0x1FD4, 0x1FD5, 0x1FD6, 0x1FD7, 0x1FD8, 0x1FD9,
    0x1FDA, 0x1FDB, 0x1FDC, 0x1FDD, 0x1FDE, 0xFFFF,
};
u16 lb_s13_skill_layout_ids[8] = { 0x1F1F, 0x1F20, 0x1F21, 0x1F22, 0x1F23, 0x1F24, 0x1F25, 0x1F26 };
u16 lb_s13_status_slot_layout_ids[3] = { 0x1F85, 0x1F86, 0x1F87 };
u16 lb_s13_skill_cursor_sprite_ids[4] = { 0x1EDC, 0x1EDD, 0x1EDE, 0xFFFF };
u16 lb_s13_pick_layout_ids[4] = { 0x1FA9, 0x1FAA };
u16 lb_s13_value_layout_ids[4] = { 0x1FC7, 0x1FC8, 0x1FC9, 0x1FCA };
u16 lb_s13_rank_sprite_ids[4] = { 0x1FAF, 0x1FAE, 0x1FAD };
u8 lb_s13_value_pairs[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };
u16 lb_s13_option_frame_sprite_ids[4] = { 0x1EE5, 0x1EE7, 0xFFFF };
u16 lb_s13_option_cursor_sprite_ids[4] = { 0x1EE8, 0x1EE9, 0x1EEA, 0xFFFF };
u16 lb_s13_slot_layout_ids[4] = { 0x1F21, 0x1F22, 0x1F23 };
u16 lb_s13_action_layout_ids[2] = { 0x1F4F, 0x1F50 };

extern "C" {

/* This unit's own forward declarations (the definitions follow in address order). */
void lb_seen_bit_set(u8 bit, u8 index);
void lb_handled_set(u8 unused, u8 index);
void lb_sub0d_send(u8 index, LbActReq* req, s8 flag);
void lb_sub0e_send(u8 index, u16 id, u8 flag, u16 value);
void lb_name_tail_copy(LbNameTail* dst, LbNameTail* src);
s8* lb_tutorial_page_text_get(u8 kind, u8 set, u8 page);
u8 lb_triplet_value2_get(u8 a, u8 b);
u8 lb_triplet_value0_get(u8 a, u8 b);
u8 lb_triplet_value1_get(u8 a, u8 b);
LbComboIndex* lb_combo_index_find(u16 item);
LbComboRecipe* lb_combo_recipe_find(u16 a, u16 b, s32 mode);
u32 lb_combo_recipe_open_ck(_PLW* plw, LbComboRecipe* recipe);
s8 lb_combo_rate_get(LbComboRecipe* recipe, s32 mode);
void lb_tutorial_page_draw(void);
void lb_tutorial_note_draw(void);
s32 lb_tutorial_entry_find(LbTutorialEntry* entry);
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd);
void lb_deli_entry_copy(LbDeliEntry* dst, LbDeliEntry* src);
void lb_area_select_screen_draw(void);
void lb_item_gets_box_open(LbScreen13Work* work);
void lb_status_preview_update(LbScreen13Work* work);
void lb_entry_list_init(LbScreen13Work* work);
s32 lb_entry_list_step(LbScreen13Work* work);
void lb_dance_list_init(LbScreen13Work* work);
s32 lb_dance_list_step(LbScreen13Work* work);
void lb_skill_entry_list_init(LbScreen13Work* work);
s32 lb_skill_menu_step(LbScreen13Work* work);
void lb_skill_pick_init(LbScreen13Work* work);
void lb_skill_clash_remove(LbScreen13Work* work);
void lb_skill_page_build(LbScreen13Work* work, s16 page);
s32 lb_skill_pick_step(LbScreen13Work* work);
void lb_s13_skill_row_draw(u8 skill, bool enabled, bool sel, bool active, bool marked, _mh_ivec2_* pos);
}

#pragma peephole off
#pragma pool_data off

extern "C" {

/* Gates the request (`Pl_net_can_send`, `my_player_no`), looks its entry up (`enemy_data_find`/`eft_slot_spawn`) and
 * switches on `act_0x03` 1..8 into the per-act handlers. */
void lb_act_dispatch(u8 index, LbActReq* req) {
    void* entry;

    if (Pl_net_can_send() != 0 && (s32)req->index_0x01 != fn_800CF384()) {
        entry = fn_803438E4(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
        if (entry == NULL) {
            entry = fn_80343B74(req->sel_0x04.bytes_0x00.b_0x01, req->sel_0x04.bytes_0x00.a_0x00,
                                req->sel_0x04.bytes_0x00.c_0x02);
        }
        if (fn_80343B44(entry) != 0) {
            switch (req->act_0x03) {
            case 1:
                eft_net_recv_state((EftSlot*)entry, (NetEftStateMsg*)req);
                return;
            case 2:
                eft_net_recv_step((EftSlot*)entry, (NetEftStepMsg*)req);
                return;
            case 3:
                eft_net_recv_live((EftSlot*)entry, (NetEftLiveMsg*)req);
                return;
            case 4:
                eft_net_recv_pos((EftSlot*)entry, (NetEftPosMsg*)req);
                return;
            case 5:
                eft_net_recv_work((EftSlot*)entry, (NetEftWorkMsg*)req, fn_800CF384());
                return;
            case 6:
                eft_net_recv_mark((EftSlot*)entry, (NetEftMarkMsg*)req);
                return;
            case 7:
                eft_net_recv_bind((EftSlot*)entry, (NetEftBindMsg*)req);
                return;
            case 8:
                eft_net_recv_release((EftSlot*)entry, (NetEftReleaseMsg*)req);
                break;
            }
        }
    }
}

/* Sends the header-only sub-0x05 packet ("entry +0x08 selected", id = the request's mask byte) when the link is up;
 * the only sender with no payload. */
void lb_entry_selected_send(LbActReq* req) {
    LbCmdSub05 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(req->mask_0x08.byte_0x00, 0xD, 5);
        broadcastSessionCommand(&cmd, 4);
    }
}

/* The same header-only packet with sub-command 6+index and the entry id in its pad field. */
void lb_entry_notify_send(s32 index, u8 entry) {
    LbCmdSub05 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(entry, 0xD, (u8)(index + 6));
        broadcastSessionCommand(&cmd, 4);
    }
}

/* Acts 6 and 7: 6 announces the companion work's entry when its step byte is not 4, 7 forwards the index to
 * `quest_reward_faint_penalty`. */
void lb_act_announce(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    switch (req->act_0x03) {
    case 6:
        if (isReadyCountOne() != 0) {
            work = get_move_work_adrs(0);
            if (work != NULL) {
                companion = work->companion_0xDC;
                if (companion != NULL && (s8)companion->step_0x2C != 4) {
                    lb_entry_notify_send(1, req->index_0x01);
                    return;
                }
            }
        }
        return;
    case 7:
        quest_reward_faint_penalty(req->index_0x01);
        break;
    }
}

/* Zeroes the two 8-entry per-entry byte arrays `lbl_80794B90`/`lbl_80794B98`; no caller in this file. */
void lb_entry_flags_clear(void) {
    u8* seen = lbl_80794B90;
    u8* handled = lbl_80794B98;

    seen[0] = 0;
    handled[0] = 0;
    seen[1] = 0;
    handled[1] = 0;
    seen[2] = 0;
    handled[2] = 0;
    seen[3] = 0;
    handled[3] = 0;
    seen[4] = 0;
    handled[4] = 0;
    seen[5] = 0;
    handled[5] = 0;
    seen[6] = 0;
    handled[6] = 0;
    seen[7] = 0;
    handled[7] = 0;
}

/* Sends the index-selected sub-0x0A command with a u8 payload.  GUESS name: the caller is unwritten, so the protocol
 * slot is the evidence. */
void lb_sub0a_send(u8 index, u8 value) {
    LbCmdSub0A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 10);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 8);
    }
}

/* Act 10: sets this entry's seen bit in `lbl_80794B90` (one `lb_seen_bit_set` call with the request's word). */
void lb_act_seen_set(u8 bit, LbActReq* req) {
    lb_seen_bit_set(bit, (u8)req->sel_0x04.word_0x00);
}

/* The sub-0x0B twin of `lb_sub0a_send`.  GUESS name: caller unwritten. */
void lb_sub0b_send(u8 index, u8 value) {
    LbCmdSub0A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 11);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 8);
    }
}

/* Act 11: marks this entry handled in `lbl_80794B98` (one `lb_handled_set` call with the request's word). */
void lb_act_handled_set(u8 bit, LbActReq* req) {
    lb_handled_set(bit, (u8)req->sel_0x04.word_0x00);
}

/* Act 9: writes the entry's id into `system_w.ring_0x18[3]` unless the pad owns it. */
void lb_act_entry_publish(u8 unused, LbActReq* req) {
    if (isReadyCountOne() != 1) {
        system_w.ring_0x18[3] = (u16)req->sel_0x04.word_0x00;
    }
}

/* Sets bit `bit` of the per-entry byte `lbl_80794B90[index]`. */
void lb_seen_bit_set(u8 bit, u8 index) {
    lbl_80794B90[index] |= 1 << bit;
}

/* Writes 1 into the per-entry byte `lbl_80794B98[index]` (the first parameter is unused). */
void lb_handled_set(u8 unused, u8 index) {
    lbl_80794B98[(u8)index] = 1;
}

/* Whether every set bit of `lbl_80794B90[index]`'s low nibble belongs to a present pad: their count against
 * `countOccupiedServerSlots()`. */
s32 lb_seen_pad_ck(u8 index) {
    s32 count;
    u8 bits;
    u8 i;

    count = 0;
    bits = lbl_80794B90[index] & 0xF;
    i = 0;
    do {
        if ((bits & 1) != 0 && isServerSlotOccupied(i) != 0) {
            count += 1;
        }
        bits = (u8)((s32)bits >> 1);
        i += 1;
    } while ((s32)i < 4);
    return count == countOccupiedServerSlots();
}

/* Whether the per-entry byte `lbl_80794B98[index]` is set. */
s32 lb_handled_ck(u8 index) {
    return lbl_80794B98[index] != 0;
}

/* Act 8: sel 5/3 hand the award screen to act 5's or act 3's entry state (`work->state_0xFA`) and start the
 * `quest_time_limit_set` delay scaled by `Screen_w`. */
void lb_act_award_handover(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    if (isServerSelectState() != 0) {
        work = get_move_work_adrs(0);
        if (work != NULL) {
            companion = work->companion_0xDC;
            if (companion != NULL) {
                if (req->sel_0x04.word_0x00 == 5) {
                    work->state_0xFA = 5;
                    companion->value_0x20 = companion->value_0x24;
                    companion->step_0x2C = companion->step_0x2C + 1;
                    quest_time_limit_set((s32)(lbl_8079B2B8 * Screen_w.scale_0x14));
                }
                if (req->sel_0x04.word_0x00 == 3) {
                    menu_item_close();
                    work->state_0xFA = 3;
                    companion->value_0x20 = companion->value_0x24;
                    companion->step_0x2C = companion->step_0x2C + 1;
                    quest_time_limit_set((s32)(lbl_8079B2BC * Screen_w.scale_0x14));
                    snd_quest_start_bgm_set();
                }
            }
        }
    }
}

/* Sends the sub-0x0C/0x01 "hand this entry over" packet (act 12) from the companion work's two mask words. */
void lb_entry_handover_send(u8 kind, u8 index, u8 value) {
    LbCmdSub010C cmd;
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL && isServerSelectState() != 0) {
            if (kind == 1) {
                ((NetMsgHeader*)&cmd)->fill(index, 0xD, 1);
                cmd.pad_index_0x04 = index;
                cmd.flag_0x05 = 0;
                cmd.value_0x07 = value;
            } else {
                ((NetMsgHeader*)&cmd)->fill(index, 0xD, 12);
                cmd.pad_index_0x04 = fn_800CF384();
                cmd.entry_0x06 = index;
                cmd.flag_0x05 = 1;
                cmd.value_0x07 = value;
                cmd.mask_0x08 = companion->bits_0x684[0];
                cmd.mask_0x0C = companion->bits_0x684[1];
            }
            broadcastSessionCommand(&cmd, 0x10);
        }
    }
}

/* Acts 1 and 12: records this entry's bit in `companion->bits_0x684` and announces it. */
void lb_act_handover(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbMoveEntry* moves;
    LbCompanionWork* companion;
    LbCompanionPair* pair;
    u8 index;
    u32 bit;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (req->sel_0x04.bytes_0x00.b_0x01 == 0) {
                if (isReadyCountOne() == 1) {
                    index = req->sel_0x04.bytes_0x00.d_0x03;
                    bit = 1 << (index & 0x1F);
                    if ((companion->bits_0x684[index >> 5] & bit) != 0) {
                        index = 0xFF;
                    } else {
                        companion->bits_0x684[index >> 5] =
                            companion->bits_0x684[index >> 5] | bit;
                        index = req->sel_0x04.bytes_0x00.d_0x03;
                    }
                    lb_entry_handover_send(12, req->sel_0x04.bytes_0x00.a_0x00, index);
                }
            } else {
                index = req->sel_0x04.bytes_0x00.c_0x02;
                if ((s32)index == fn_800CF384()) {
                    moves = (LbMoveEntry*)get_move_work_adrs(2);
                    if (moves == NULL) {
                        return;
                    }
                    moves[index].flag_0x659 = 0;
                    if (req->sel_0x04.bytes_0x00.d_0x03 != 0xFF) {
                        pair = &companion->pairs_0x5E2[req->sel_0x04.bytes_0x00.d_0x03];
                        pl_item_add((_PLW*)&moves[index], pair->id_0x00, pair->value_0x02);
                        cockpit_pointer_target_set(pair->id_0x00, (s8)req->sel_0x04.bytes_0x00.d_0x03);
                    }
                }
                companion->bits_0x684[0] = companion->bits_0x684[0] | req->mask_0x08.word_0x00;
                companion->bits_0x684[1] = companion->bits_0x684[1] | req->mask_0x0C.word_0x00;
            }
        }
    }
}

/* Sends the sub-0x0D command with the request's three bytes and its halfword; the act-13 row update calls it. */
void lb_sub0d_send(u8 index, LbActReq* req, s8 flag) {
    LbCmdSub0D cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xD);
        cmd.value_0x04 = req->sel_0x04.bytes_0x00.b_0x01;
        cmd.index_0x05 = req->sel_0x04.bytes_0x00.c_0x02;
        cmd.flag_0x06 = flag;
        cmd.word_0x08 = req->mask_0x08.half_0x00;
        broadcastSessionCommand(&cmd, 0xA);
    }
}

/* Act 13: looks the row up (`serial_find`), then either starts it in state 4 (`lb_sub0d_send`) or hands the word to
 * `serial_state_set_word`. */
void lb_act_row_update(u8 unused, LbActReq* req) {
    ShellSerialEntry* row;

    row = serial_find(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
    if (row != NULL) {
        if (isReadyCountOne() == 1 && req->sel_0x04.bytes_0x00.c_0x02 == 3 && row->state_0x07 <= 3) {
            row->word_0x08 = req->mask_0x08.half_0x00;
            row->state_0x07 = 4;
            lb_sub0d_send(row->player_0x05, req, 4);
            return;
        }
        serial_state_set_word(row, req->sel_0x04.bytes_0x00.c_0x02, req->mask_0x08.half_0x00);
    }
}

/* Sends the sub-0x0E command, or hands the two ids to the local row writer `quest_score_deduct` when the link is down. */
void lb_sub0e_send(u8 index, u16 id, u8 flag, u16 value) {
    LbCmdSub0E cmd;
    u8 set = flag;

    if (isServerSelectState() == 0) {
        quest_score_deduct(id, value);
        return;
    }
    if ((s32)set == 0) {
        set = 1;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xE);
    cmd.value_0x04 = set;
    cmd.flag_0x05 = 0;
    cmd.id_0x06 = id;
    cmd.word_0x08 = value;
    broadcastSessionCommand(&cmd, 0xA);
}

/* Act 14: kind 1 sends the row with `lb_sub0e_send` when this pad owns it; anything else updates it locally. */
void lb_act_row_apply(u8 unused, LbActReq* req) {
    u8 kind;

    kind = req->sel_0x04.bytes_0x00.a_0x00;
    if ((s32)kind != 0) {
        if (kind == 1) {
            if (isReadyCountOne() == 1) {
                lb_sub0e_send(0, req->sel_0x04.bytes_0x00.c_0x02, 2, req->mask_0x08.half_0x00);
            }
        } else {
            quest_score_deduct(req->sel_0x04.bytes_0x00.c_0x02, req->mask_0x08.half_0x00);
        }
    }
}

/* Act 15: sends the 0x2C-byte sub-0x0F text packet built from the caller's block; with the link down, `em_small_spawn_stat`. */
void lb_sub0f_send(u8 index, u8 value, void* text, u16 id, u8 flag, f32 scale) {
    LbCmdSub0F cmd;

    lb_sub0f_init(&cmd);
    if (isServerSelectState() == 0) {
        em_small_spawn_stat(value, text, id, flag, scale);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xF);
    cmd.value_0x04 = value;
    lb_name_tail_copy(&cmd.text_0x0C, (LbNameTail*)text);
    cmd.id_0x06 = id;
    cmd.flag_0x05 = flag;
    cmd.scale_0x08 = scale;
    broadcastSessionCommand(&cmd, 0x2C);
}

/* Copies the 0x20-byte `LbNameTail`: its first 8 bytes a byte at a time, the rest word-wise (MWCC's idiom for the two
 * members' alignments). */
void lb_name_tail_copy(LbNameTail* dst, LbNameTail* src) {
    u8 i;

    for (i = 0; i < 8; i++) {
        dst->bytes_0x00[i] = src->bytes_0x00[i];
    }
    for (i = 0; i < 6; i++) {
        dst->words_0x08[i] = src->words_0x08[i];
    }
}

/* Clears the sub-0x0F packet's text payload (`em_ground_rec_clear`) and returns the packet. */
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd) {
    em_ground_rec_clear(cmd->text_0x0C.bytes_0x00);
    return cmd;
}

/* The link-down half of act 15: hands the packet's own fields straight to the text writer `em_small_spawn_stat`. */
void lb_text_apply(u8 unused, LbCmdSub0F* cmd) {
    void* text = cmd->text_0x0C.bytes_0x00;

    em_small_spawn_stat(cmd->value_0x04, text, cmd->id_0x06, cmd->flag_0x05, cmd->scale_0x08);
}

/* Sends the sub-0x10 command with one signed byte, or hands it to `em_res_peer_count_inc` when the link is down. */
void lb_sub10_send(u8 index, s8 value) {
    LbCmdSub10 cmd;

    if (isServerSelectState() == 0) {
        em_res_peer_count_inc(value, index);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x10);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 16: hands the request's byte and the pad index to `em_res_peer_count_inc`. */
void lb_act_byte_apply(u8 unused, LbActReq* req) {
    em_res_peer_count_inc((s8)req->sel_0x04.bytes_0x00.a_0x00, req->index_0x01);
}

/* Sends the sub-0x11 command with two signed bytes.  GUESS name: caller unwritten. */
void lb_sub11_send(s8 a, s8 b) {
    LbCmdSub11 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x11);
        cmd.value_0x04 = a;
        cmd.value_0x05 = b;
        broadcastSessionCommand(&cmd, 6);
    }
}

/* Act 17: hands the request's two bytes to `fn_802B09B8`. */
void lb_act_pair_apply(u8 unused, LbActReq* req) {
    stage_area_flag_set(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
}

/* Sends the sub-0x12 command with one byte, or hands it to `fn_802B45F4` when the link is down. */
void lb_sub12_send(u8 value) {
    LbCmdSub12 cmd;

    if (isServerSelectState() == 0) {
        stage_area_timer_arm(value);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x12);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 18: hands the request's byte to `fn_802B45F4`. */
void lb_act_index_apply(u8 unused, LbActReq* req) {
    stage_area_timer_arm(req->sel_0x04.bytes_0x00.a_0x00);
}

/* Sends the sub-0x13 command: the value byte, two halfwords and this pad's index.  GUESS name: caller unwritten. */
void lb_sub13_send(s16 first, s16 second, u8 value) {
    LbCmdSub13 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x13);
        cmd.value_0x04 = value;
        cmd.first_0x08 = first;
        cmd.second_0x0A = second;
        cmd.pad_index_0x0C = fn_800CF384();
        broadcastSessionCommand(&cmd, 0x10);
    }
}

/* Act 19: with the request's word clear, hands its three bytes to the value writer `quest_arena_data_step`. */
void lb_act_value_apply(u8 unused, LbActReq* req) {
    if (req->mask_0x08.word_0x00 == 0) {
        quest_arena_data_step(req->mask_0x0C.byte_0x00, req->mask_0x08.halves.low_0x00,
                    req->mask_0x08.halves.high_0x02);
    }
}

/* Sends the sub-0x14 command with one signed byte.  GUESS name: caller unwritten. */
void lb_sub14_send(s8 value) {
    LbCmdSub10 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x14);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 5);
    }
}

/* Act 20: sets `companion->flag_0x6A29` once the request's word reaches `limit_0x6A2A`. */
void lb_act_limit_set(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL && (s32)req->mask_0x08.word_0x00 >= (s8)companion->limit_0x6A2A) {
            companion->flag_0x6A29 = 1;
        }
    }
}

/* Sends the header-only sub-0x15 command.  GUESS name: caller unwritten. */
void lb_sub15_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x15);
    broadcastSessionCommand(&cmd, 4);
}

/* Increments the companion work's tick counter `count_0x69A4`. */
void lb_companion_tick(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            companion->count_0x69A4++;
        }
    }
}

/* Sends the header-only sub-0x1C command.  GUESS name: caller unwritten. */
void lb_sub1c_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x1C);
    broadcastSessionCommand(&cmd, 4);
}

/* Sends the sub-0x16 command with a word and two signed bytes; act 21 builds the same packet inline. */
void lb_sub16_send(s32 value, u8 flag) {
    LbCmdSub16 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x16);
        cmd.flag_0x08 = flag;
        cmd.value_0x04 = value;
        cmd.value_0x09 = 0;
        broadcastSessionCommand(&cmd, 0xC);
    }
}

/* Act 21: sends the row over when this pad owns it, else `quest_element_done_mark` writes the value into the companion slot. */
void lb_act_slot_write(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    LbCmdSub16 cmd;
    u8 index;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (req->sel_0x04.bytes_0x00.d_0x03 == 0) {
                if (isReadyCountOne() != 0 && quest_sub_state_end_ck(1) == 0 &&
                    quest_element_pick_ck((QuestWork*)companion, req->mask_0x08.byte_0x00, 1) != 1 &&
                    (s8)companion->step_0x2C != 4) {
                    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x16);
                    cmd.flag_0x08 = req->mask_0x08.byte_0x00;
                    cmd.value_0x04 = req->sel_0x04.word_0x00;
                    cmd.value_0x09 = 1;
                    broadcastSessionCommand(&cmd, 0xC);
                }
            } else if (work->state_0xFA <= 2 && quest_sub_state_end_ck(1) == 0 &&
                       quest_element_pick_ck((QuestWork*)companion, req->mask_0x08.byte_0x00, 1) != 1) {
                index = req->mask_0x08.byte_0x00;
                quest_element_done_mark((QuestWork*)companion, (QuestElement*)&companion->slots_0x94[index],
                                        (u16)index, 0);
            }
        }
    }
}

/* Sends the sub-0x17 command with a halfword and three signed bytes.  GUESS name: caller unwritten. */
void lb_sub17_send(s16 value, s8 first, s8 second, s8 third) {
    LbCmdSub17 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x17);
    cmd.value_0x04 = value;
    cmd.first_0x06 = first;
    cmd.second_0x07 = second;
    cmd.third_0x08 = third;
    broadcastSessionCommand(&cmd, 0xA);
}

/* Act 22: keeps the companion work's high score `best_0x8F`, then hands the row on (`quest_item_pair_copy_row`,
 * `hud_msg_push`). */
void lb_act_best_keep(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    u8 value;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            value = req->sel_0x04.bytes_0x00.c_0x02;
            if (companion->best_0x8F < (s8)value) {
                companion->best_0x8F = value;
                companion->value_0x8B = req->sel_0x04.bytes_0x00.d_0x03;
            }
            quest_item_pair_copy_row((Q_ItemPair*)companion->pairs_0x5E2,
                                     req->sel_0x04.half_0x00,
                                     (s8)companion->best_0x8F, req->mask_0x08.byte_0x00);
            hud_msg_push(1, quest_str_tbl_35_get(0x14));
        }
    }
}

/* Sends the header-only sub-0x18 command.  GUESS name: caller unwritten. */
void lb_sub18_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x18);
    broadcastSessionCommand(&cmd, 4);
}

/* Puts the companion work into mode 3 (`mode_0x6978`), the area-change announcement. */
void lb_companion_mode_set(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            companion->mode_0x6978 = 3;
        }
    }
}

/* Sends the sub-0x19 entry-start command and records the start (`started_0x6A40`); while the act is already
 * announced the caller's flag is dropped. */
void lb_entry_start_send(LbCompanionWork* companion, u8 value, s32 arg, u8 flag) {
    LbCmdSub19 cmd;
    u8 started;

    started = flag;
    if (flag == 1) {
        if (isReadyCountOne() == 0) {
            started = 0;
        } else if (companion->started_0x6A40 != 0) {
            return;
        }
    }
    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x19);
    cmd.value_0x04 = arg;
    cmd.value_0x08 = companion->slots_0x94[0].value_0x00;
    cmd.value_0x0C = companion->slots_0x94[1].value_0x00;
    cmd.value_0x10 = companion->slots_0x94[2].value_0x00;
    cmd.value_0x16 = companion->param_0x6A3C;
    cmd.index_0x15 = value;
    cmd.value_0x17 = companion->param_0x6A3A;
    cmd.value_0x18 = companion->param_0x6A3B;
    cmd.value_0x19 = companion->param_0x6A3D;
    cmd.value_0x1A = companion->param_0x6A3E;
    cmd.flag_0x14 = companion->param_0x6A3F;
    if (started == 1) {
        companion->started_0x6A40 = 1;
        cmd.started_0x1B = 1;
    } else {
        cmd.started_0x1B = 0;
    }
    broadcastSessionCommand(&cmd, 0x1C);
    companion->index_0x6A68 = value;
    companion->value_0x20 = arg;
    companion->step_0x2C = 4;
}

/* The act-19 announcement: `lb_entry_start_send` with the default flag 0. */
void lb_entry_start_default(LbCompanionWork* companion, u8 value, s32 arg) {
    lb_entry_start_send(companion, value, arg, 0);
}

/* Act 23: copies the request's fields into the companion work, then sends or applies the entry start. */
void lb_act_entry_start(u8 unused, LbCmdSub19* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    LbCmdSub19 cmd;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (companion->index_0x6A68 == 0) {
                if (isReadyCountOne() != 0 && companion->started_0x6A40 == 0) {
                    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x19);
                    cmd.value_0x04 = req->value_0x04;
                    cmd.value_0x08 = req->value_0x08;
                    cmd.value_0x0C = req->value_0x0C;
                    cmd.value_0x10 = req->value_0x10;
                    cmd.flag_0x14 = req->flag_0x14;
                    cmd.index_0x15 = req->index_0x15;
                    cmd.value_0x16 = req->value_0x16;
                    cmd.value_0x17 = req->value_0x17;
                    cmd.started_0x1B = 1;
                    broadcastSessionCommand(&cmd, 0x1C);
                    companion->started_0x6A40 = 1;
                }
            } else if (work->state_0xFA <= 2) {
                companion->value_0x24 = req->value_0x04;
                companion->slots_0x94[0].value_0x00 = req->value_0x08;
                companion->slots_0x94[1].value_0x00 = req->value_0x0C;
                companion->slots_0x94[2].value_0x00 = req->value_0x10;
                work->state_0xFA = req->flag_0x14;
                companion->index_0x6A68 = req->index_0x15;
                companion->param_0x6A3A = req->value_0x16;
                companion->param_0x6A3B = req->value_0x17;
                work->value_0xFD = req->value_0x18;
                work->value_0xFC = req->value_0x19;
                work->value_0xFB = req->value_0x1A;
                companion->param_0x6A3C = work->state_0xFA;
                companion->param_0x6A3E = work->value_0xFC;
                companion->param_0x6A3F = work->value_0xFB;
                if (work->state_0xFA == 5) {
                    quest_result_enter((Q_ItemWork*)companion, (Q_MoveWork*)work, companion->index_0x6A68);
                    return;
                }
                quest_start_enter((Q_ItemWork*)companion, (Q_MoveWork*)work);
            }
        }
    }
}

/* Sends the sub-0x1A command with one signed byte.  GUESS name: caller unwritten. */
void lb_sub1a_send(u8 index, s8 value) {
    LbCmdSub1A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x1A);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 5);
    }
}

/* Act 24: hands the pad index and the request's byte to the page handler `fn_802BC000`. */
void lb_act_page_apply(u8 index, LbActReq* req) {
    camera_slot_bit_set((u8)index, req->sel_0x04.bytes_0x00.a_0x00);
}

/* Sends the header-only sub-0x1B command, or sets `work->flag_0x22E3` locally when the link is down. */
void lb_area_change_send(u8 index) {
    LbCmdSub1B cmd;
    LbMoveWork* work;

    if (isServerSelectState() == 0) {
        work = get_move_work_adrs(0);
        if (work != NULL) {
            work->flag_0x22E3 = 1;
        }
    } else {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x1B);
        broadcastSessionCommand(&cmd, 4);
    }
}

/* Flags the area change in the move work (`work->flag_0x22E3 = 1`) without sending anything. */
void lb_area_change_flag(u8 unused, LbActReq* req) {
    LbMoveWork* work;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        work->flag_0x22E3 = 1;
    }
}

/* Sends the sub-0x1D command with two bytes.  GUESS name: caller unwritten. */
void lb_sub1d_send(u8 value, s8 flag) {
    LbCmdSub1D cmd;

    ((NetMsgHeader*)&cmd)->fill(value, 0xD, 0x1D);
    cmd.value_0x04 = value;
    cmd.value_0x05 = flag;
    broadcastSessionCommand(&cmd, 8);
}

/* Act 25: hands the request's two bytes to `arena_other_player_eq_set`. */
void lb_act_pad_apply(u8 unused, LbActReq* req) {
    arena_other_player_eq_set(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
}

/* The second act dispatcher: switches on `act_0x03` 1..29 into the per-act handlers; acts 5, 19 and 24 apply only to
 * another pad's request. */
void lb_act_dispatch_ex(u8 index, LbActReq* req) {
    u8 pad;

    switch (req->act_0x03) {
    case 1:
    case 12:
        lb_act_handover(index, req);
        break;
    case 5:
        pad = req->index_0x01;
        if ((s32)pad != fn_800CF384()) {
            quest_slot_arm_all_a((s8)pad);
        }
        break;
    case 6:
    case 7:
        lb_act_announce(index, req);
        break;
    case 8:
        lb_act_award_handover(index, req);
        break;
    case 9:
        lb_act_entry_publish(index, req);
        break;
    case 10:
        lb_act_seen_set(index, req);
        break;
    case 11:
        lb_act_handled_set(index, req);
        break;
    case 13:
        lb_act_row_update(index, req);
        break;
    case 14:
        lb_act_row_apply(index, req);
        break;
    case 15:
        lb_text_apply(index, (LbCmdSub0F*)req);
        break;
    case 16:
        lb_act_byte_apply(index, req);
        break;
    case 17:
        lb_act_pair_apply(index, req);
        break;
    case 18:
        lb_act_index_apply(index, req);
        break;
    case 19:
        if ((s32)req->index_0x01 != fn_800CF384()) {
            lb_act_value_apply(index, req);
        }
        break;
    case 20:
        lb_act_limit_set(index, req);
        break;
    case 21:
    case 28:
        lb_companion_tick(index, req);
        break;
    case 22:
        lb_act_slot_write(index, req);
        break;
    case 23:
        lb_act_best_keep(index, req);
        break;
    case 24:
        if ((s32)req->index_0x01 != fn_800CF384()) {
            lb_companion_mode_set(index, req);
        }
        break;
    case 25:
        lb_act_entry_start(index, (LbCmdSub19*)req);
        break;
    case 26:
        lb_act_page_apply(index, req);
        break;
    case 27:
        lb_area_change_flag(index, req);
        break;
    case 29:
        lb_act_pad_apply(index, req);
        break;
    }
}

/* Scans the ten 0x130-byte page records for the six-byte key the caller points at; 0xFF on a hole.  The name is the
 * map's (`hud/move_work_update.cpp` calls it by it). */
u8 hud_key_lookup(u8* key) {
    u8 i;

    for (i = 0; i < 0xA; i++) {
        if (memcmp(key, &lobby_hunter_cards[i].key_0x03, 6) == 0) {
            return i;
        }
    }
    return 0xFF;
}

/* Resets the tutorial/quest announcement block `lb_tutorial_work`'s four fields and stores the kind byte. */
void lb_quest_work_init(u16 kind) {
    lb_tutorial_work.active_0x00 = 0;
    lb_tutorial_work.flag_0x08 = 0;
    lb_tutorial_work.kind_0x06 = kind;
    lb_tutorial_work.ptr_0x0C = NULL;
}

/* Steps the tutorial block: 0 opens a pending request's pages (area-gated per event), 1 pages through them, 2 runs
 * the entry's note list, 3 closes and re-requests event 15. */
void lb_tutorial_step(void) {
    LbQuestWork* work = &lb_tutorial_work;
    PlayerPad* pad = Psw;
    u16 pressed;

    work->cursor_0x04 = 0;
    if ((u8)event_demo_running_ck() != 1 && quest_move_idle_ck() != 0 &&
        (quest_sub_state_end_ck(1) == 0 || work->mode_0x0A == 13)) {
        switch (work->active_0x00) {
        case 0:
            if (work->flag_0x08 != 0) {
                work->flag_0x08 = 0;
                switch (work->mode_0x0A) {
                case 26:
                    if (get_now_areano() != 0) {
                        work->ptr_0x0C = NULL;
                        return;
                    }
                    break;
                case 18:
                    if (get_now_areano() != 1) {
                        work->ptr_0x0C = NULL;
                        return;
                    }
                    break;
                case 2:
                    if (get_now_areano() != 3) {
                        work->ptr_0x0C = NULL;
                        return;
                    }
                    break;
                case 3:
                    if (get_now_areano() != 11) {
                        work->ptr_0x0C = NULL;
                        return;
                    }
                    break;
                case 27:
                    if (get_now_areano() != 12) {
                        work->ptr_0x0C = NULL;
                        return;
                    }
                    break;
                }
                work->active_0x00 = 1;
                work->page_0x01 = 0;
                work->page_count_0x02 = lb_triplet_value2_get(work->kind_0x06, work->index_0x07);
                work->note_row_0x03 = 0;
                work->last_0x09 = 0;
                work->timer_0x0B = 10;
                sysSE_window_open_play();
                switch (lb_triplet_value0_get(work->kind_0x06, work->index_0x07)) {
                case 0:
                    se_guide_voice_play(2, lb_triplet_value1_get(work->kind_0x06, work->index_0x07));
                    break;
                case 1:
                    se_guide_voice_play(1, lb_triplet_value1_get(work->kind_0x06, work->index_0x07));
                    break;
                case 2:
                    se_guide_voice_play(0, lb_triplet_value1_get(work->kind_0x06, work->index_0x07));
                    break;
                }
                if (work->ptr_0x0C != NULL) {
                    userdata_progress_flag_set(work->ptr_0x0C->flag_0x00);
                }
                pad_hold_set();
            }
            break;
        case 1:
            if (work->timer_0x0B != 0) {
                work->timer_0x0B--;
            } else if (work->last_0x09 != 0) {
                pressed = pad->button_0x2C0.pressed_0x04;
                if ((pressed & 0x10) != 0) {
                    if (work->ptr_0x0C->note_0x05 >= 0) {
                        work->active_0x00 = 2;
                        work->note_row_0x03 = 0;
                        work->last_0x09 = 0;
                        ainpc_entry_update_all();
                        subTransSetPrio(7, (u32)lb_tutorial_note_draw, 0, 0);
                        subTransSetPrio(7, (u32)ainpc_entry_draw_all, 0, 0);
                        sysSE_req(6);
                        return;
                    }
                    work->active_0x00 = 3;
                    pl_input_lock_set(10);
                    sysSE_req(1);
                } else if ((pressed & 0x4000) != 0) {
                    work->page_0x01 = 0;
                    work->last_0x09 = 0;
                    work->cursor_0x04 = 0;
                    sysSE_req(12);
                }
            } else if ((pad->button_0x2C0.pressed_0x04 & 0x10) != 0) {
                if (++work->page_0x01 >= work->page_count_0x02) {
                    work->last_0x09 = 1;
                }
                sysSE_req(6);
            }
            subTransSetPrio(7, (u32)lb_tutorial_page_draw, 0, 0);
            break;
        case 2:
            if (work->last_0x09 == 0) {
                if ((u32)note_slot_cursor_step(work->ptr_0x0C->note_0x05, &work->note_row_0x03,
                                               pad->button_0x2C0.pressed_0x04, &work->cursor_0x04) == 1) {
                    ainpc_entry_update_all();
                    subTransSetPrio(7, (u32)lb_tutorial_note_draw, 0, 0);
                    subTransSetPrio(7, (u32)ainpc_entry_draw_all, 0, 0);
                    return;
                }
                work->last_0x09 = 1;
                subTransSetPrio(7, (u32)lb_tutorial_page_draw, 0, 0);
                return;
            }
            pressed = pad->button_0x2C0.pressed_0x04;
            if ((pressed & 0x10) != 0) {
                work->active_0x00 = 3;
                sysSE_req(1);
                pl_input_lock_set(10);
                sysSE_req(1);
            } else if ((pressed & 0x4000) != 0) {
                work->note_row_0x03 = 0;
                work->last_0x09 = 0;
                sysSE_req(12);
                ainpc_entry_update_all();
                subTransSetPrio(7, (u32)lb_tutorial_note_draw, 0, 0);
                subTransSetPrio(7, (u32)ainpc_entry_draw_all, 0, 0);
                return;
            }
            subTransSetPrio(7, (u32)lb_tutorial_page_draw, 0, 0);
            break;
        case 3:
            work->active_0x00 = 0;
            pad_hold_clear();
            work->ptr_0x0C = NULL;
            lb_event_request(15);
            break;
        }
    }
}

/* Draws the tutorial page: the option frame, the kind's title, the page text (or the closing page's buttons and
 * strings), the kind's sprite and the waiting spinner. */
void lb_tutorial_page_draw(void) {
    LbQuestWork* work = &lb_tutorial_work;
    _mh_ivec2_ pos;
    _mh_ivec2_ icon;
    u8 kind;

    kind = 5;
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    font_flush();
    if (work->last_0x09 != 0) {
        kind |= 8;
    }
    item_page_draw_option_frame((s8)work->page_0x01, (s8)work->page_count_0x02, work->cursor_0x04, 5, kind, &pos);
    if (work->active_0x00 <= 1) {
        kind = lb_triplet_value0_get(work->kind_0x06, work->index_0x07);
        draw_font_idx(0x514, tutorial_name[kind], 4, &pos);
    }
    if (work->last_0x09 != 0) {
        if (get_option_cfg(7) == 0) {
            draw_sprite_ary(tutorial_end_sprite_ids, &pos);
        } else {
            draw_sprite_ary(tutorial_end_sprite_ids_alt, &pos);
        }
        draw_font_idx(0x526, tutorial_end[0], 0, &pos);
        draw_font_idx(0x529, tutorial_end[1], 0, &pos);
    } else {
        draw_font_anim_idx(0x516, 5, lb_tutorial_page_text_get(work->kind_0x06, work->index_0x07, work->page_0x01), 0, &pos);
    }
    if (work->active_0x00 <= 1) {
        get_lsp_data(0x726, &icon);
        draw_sprite_idx(tutorial_kind_sprite_ids[kind], &icon);
    }
    if (work->last_0x09 == 0) {
        icon.x = pos.x + 375;
        icon.y = pos.y + 82;
        draw_spin_icon(icon.x, icon.y, 0);
    }
}

/* Draws the entry's note list and the waiting spinner beside it. */
void lb_tutorial_note_draw(void) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;

    LbQuestWork* work;

    font_flush();
    work = &lb_tutorial_work;
    note_slot_draw_row(work->ptr_0x0C->note_0x05, work->note_row_0x03, work->cursor_0x04);
    sprite_frame_apply(&spr, 0x4FE, 5, &pos);
    draw_spin_icon(pos.x + 375, pos.y + 82, 0);
}
/* Whether the announcement block is active (`lb_tutorial_work.active_0x00 != 0`). */
s32 lb_quest_work_active_ck(void) {
    return lb_tutorial_work.active_0x00 != 0;
}

/* Scans an event's entry list for the first row the tutorial kind and the progress flags admit, records it and
 * returns its page set; -1 when none does. */
s32 lb_tutorial_entry_find(LbTutorialEntry* entry) {
    LbQuestWork* work;

    if (entry == NULL) {
        return -1;
    }
    work = &lb_tutorial_work;
    for (; entry->flag_0x00 >= 0; entry++) {
        if (entry->kind_0x02 == work->kind_0x06 && userdata_progress_flag_ck(entry->flag_0x00) == 0 &&
            (entry->need_0x06 == -1 || userdata_progress_flag_ck(entry->need_0x06) == 1) &&
            (entry->block_0x08 == -1 || userdata_progress_flag_ck(entry->block_0x08) == 0)) {
            work->ptr_0x0C = entry;
            return entry->page_0x04;
        }
    }
    return -1;
}
/* Requests tutorial event `id`: offline single-player only, with a kind set and nothing pending; 1 when an entry
 * row was found. */
u32 lb_event_request(u16 id) {
    s32 page;

    if (isServerSelectState() == 1) {
        return 0;
    }
    if (GameMode_ck() != 1) {
        return 0;
    }
    if (lb_tutorial_work.kind_0x06 == 0) {
        return 0;
    }
    if (lb_tutorial_work.flag_0x08 != 0) {
        return 0;
    }
    page = lb_tutorial_entry_find(lb_tutorial_entry_lists[id]);
    if ((s8)page >= 0) {
        lb_tutorial_work.flag_0x08 = 1;
        lb_tutorial_work.index_0x07 = page;
        lb_tutorial_work.mode_0x0A = (s8)id;
        return 1;
    }
    return 0;
}

/* The text of page `page` of the tutorial kind's page set `set` (`tutorial_page_texts[kind][set][page]`). */
s8* lb_tutorial_page_text_get(u8 kind, u8 set, u8 page) {
    return tutorial_page_texts[kind][set][page];
}

/* The third byte of the 12-byte `LbTriplet` row at `tutorial_page_sets[a][b]`. */
u8 lb_triplet_value2_get(u8 a, u8 b) {
    return tutorial_page_sets[a][b].value_0x02;
}

/* The first byte of that row. */
u8 lb_triplet_value0_get(u8 a, u8 b) {
    return tutorial_page_sets[a][b].value_0x00;
}

/* The second byte of that row. */
u8 lb_triplet_value1_get(u8 a, u8 b) {
    return tutorial_page_sets[a][b].value_0x01;
}

/* The tutorial quest's name pointer (`*tutorial_quest_name`). */
s32 lb_quest_name_get(void) {
    return *tutorial_quest_name;
}

/* The tutorial message pointer for one entry (`tutorial_quest_msg[index]`). */
s32 lb_quest_msg_get(u8 index) {
    return *(tutorial_quest_msg + index);
}

/* The `lb_combo_index_tbl` row of first ingredient `item`; NULL when the item starts no recipe. */
LbComboIndex* lb_combo_index_find(u16 item) {
    LbComboIndex* row;

    for (row = lb_combo_index_tbl; row->item_0x00 != 0xFFFF; row++) {
        if (row->item_0x00 == item) {
            return row;
        }
    }
    return NULL;
}

/* 1 when the player can see a recipe that uses `item`, as the first ingredient or as another item's partner. */
s32 lb_combo_item_known_ck(u16 item) {
    ItemDataRecord* data;
    LbMoveEntry* moves;
    _PLW* plw;
    LbComboIndex* index;
    LbComboRecipe* recipe;
    s32 i;
    s32 j;

    data = GetItemData(item);
    moves = (LbMoveEntry*)get_move_work_adrs(2);
    if (moves == NULL) {
        return 0;
    }
    plw = (_PLW*)&moves[fn_800CF384()];
    if ((data->field_0x002 & 0x20) != 0) {
        index = lb_combo_index_find(item);
        if (index != NULL) {
            recipe = &lb_combo_recipe_tbl[index->first_0x03];
            for (i = 0; i < index->count_0x02; i++, recipe++) {
                if (lb_combo_recipe_open_ck(plw, recipe) == 1) {
                    return 1;
                }
            }
        }
        for (i = 0; i < item; i++) {
            if ((GetItemData(i)->field_0x002 & 0x20) != 0) {
                index = lb_combo_index_find(i);
                if (index != NULL) {
                    recipe = &lb_combo_recipe_tbl[index->first_0x03];
                    for (j = 0; j < index->count_0x02; j++, recipe++) {
                        if (recipe->partner_0x00 == item && lb_combo_recipe_open_ck(plw, recipe) == 1) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* The recipe combining `a` and `b` (in either order); with bit 0 of `mode` clear it must also be open to the
 * player.  NULL when there is none. */
LbComboRecipe* lb_combo_recipe_find(u16 a, u16 b, s32 mode) {
    LbComboIndex* index;
    LbComboRecipe* recipe;
    LbMoveEntry* moves;
    u16 swap;
    s32 count;

    if (a > b) {
        swap = b;
        b = a;
        a = swap;
    }
    if ((GetItemData(a)->field_0x002 & 0x20) == 0) {
        return NULL;
    }
    if ((GetItemData(b)->field_0x002 & 0x20) == 0) {
        return NULL;
    }
    index = lb_combo_index_find(a);
    if (index != NULL) {
        recipe = &lb_combo_recipe_tbl[index->first_0x03];
        for (count = index->count_0x02; count > 0; count--, recipe++) {
            if (recipe->partner_0x00 == b) {
                if ((mode & 1) != 0) {
                    return recipe;
                }
                moves = (LbMoveEntry*)get_move_work_adrs(2);
                if (lb_combo_recipe_open_ck((_PLW*)&moves[fn_800CF384()], recipe) == 1) {
                    return recipe;
                }
                break;
            }
        }
    }
    return NULL;
}

/* The recipe of the combine list's pair `index` (`lb_combo_recipe_find` with `mode`). */
LbComboRecipe* lb_combo_pair_recipe_find(u16 index, s32 mode) {
    return lb_combo_recipe_find(lb_combo_pair_tbl[index].first_0x00, lb_combo_pair_tbl[index].second_0x02, mode);
}

/* Writes the combine list's pair `index` out when its recipe applies (0 and 0 when not); returns the recipe. */
LbComboRecipe* lb_page_row_pair_get(u16 index, u16* item_a, u16* item_b, s32 mode) {
    LbComboRecipe* recipe;

    recipe = lb_combo_pair_recipe_find(index, mode);
    if (recipe != NULL) {
        *item_a = lb_combo_pair_tbl[index].first_0x00;
        *item_b = lb_combo_pair_tbl[index].second_0x02;
    } else {
        *item_b = 0;
        *item_a = 0;
    }
    return recipe;
}

/* 1 when the recipe is not hidden (bit 0 of its flags clear); the player is not consulted. */
u32 lb_combo_recipe_open_ck(_PLW* plw, LbComboRecipe* recipe) {
    if (recipe != NULL && (recipe->flags_0x07 & 1) == 0) {
        return 1;
    }
    return 0;
}

/* The success rate of combining `a` and `b`; -1 when there is no such recipe. */
s8 lb_combo_rate_find(u16 a, u16 b, s32 mode) {
    LbComboRecipe* recipe;

    recipe = lb_combo_recipe_find(a, b, 0);
    if (recipe == NULL) {
        return -1;
    }
    return lb_combo_rate_get(recipe, mode);
}

/* The success rate of `recipe` in percent: 100 with a granting skill, else the class's base rate moved by the
 * skills and by the bonus books held (mode 0: carried, mode 2: pouches or box), clamped to 0..100; mode 1 is 100. */
s8 lb_combo_rate_get(LbComboRecipe* recipe, s32 mode) {
    LbMoveEntry* moves;
    _PLW* plw;
    u16* item;
    s16 rate;
    s32 i;
    s32 count;

    if (mode == 0 || mode == 2) {
        moves = (LbMoveEntry*)get_move_work_adrs(2);
        if (moves == NULL) {
            return -1;
        }
        plw = (_PLW*)&moves[fn_800CF384()];
        if (lb_combo_recipe_open_ck(plw, recipe) == 0) {
            return -1;
        }
        if ((recipe->flags_0x07 & 2) != 0 && pl_skill_have_ck(plw, 0x43) == 1) {
            return 100;
        }
        if ((recipe->flags_0x07 & 4) != 0 && pl_skill_have_ck(plw, 0xB8) == 1) {
            return 100;
        }
        if ((recipe->flags_0x07 & 8) != 0 && Pl_cat_skill_ck(plw, 0x2A) == 1) {
            return 100;
        }
        if ((recipe->flags_0x07 & 0x10) != 0 &&
            (pl_skill_have_ck(plw, 0xCC) == 1 || Pl_cat_skill_ck(plw, 0x2D) == 1 || Pl_cat_skill_ck(plw, 0x15) == 1)) {
            return 100;
        }
        rate = lb_combo_base_rates[recipe->rate_0x04];
        if (pl_skill_have_ck(plw, 0x9C) == 1) {
            rate += 45;
        } else if (pl_skill_have_ck(plw, 0x9B) == 1) {
            rate += 25;
        } else if (pl_skill_have_ck(plw, 0x9A) == 1) {
            rate += 15;
        } else if (pl_skill_have_ck(plw, 0x9D) == 1) {
            rate -= 5;
        } else if (pl_skill_have_ck(plw, 0x9E) == 1) {
            rate -= 10;
        } else if (pl_skill_have_ck(plw, 0x9F) == 1) {
            rate -= 15;
        }
        if (Pl_cat_skill_ck(plw, 0xC) == 1) {
            rate += 20;
        }
        if (Pl_cat_skill_ck(plw, 0xD) == 1) {
            rate += 10;
        }
        for (i = 0, item = lb_combo_bonus_items; i < 5; item++, i++) {
            if (mode == 0) {
                if (GameMode_ck() == 2) {
                    count = userdata_item_slot_count(lobby_world_block);
                    if (item_pair_index_find(*item, (IdValue*)userdata_equip_item_slots_get(lobby_world_block), count) < 0) {
                        break;
                    }
                } else if (pl_item_slot_find(plw, *item) == 0xFFFF) {
                    break;
                }
            } else if (mode == 2) {
                count = userdata_pouch_size(0);
                if (item_pair_index_find(*item, (IdValue*)userdata_pouch_get(lobby_world_block, 0), count) < 0) {
                    count = userdata_pouch_size(1);
                    if (item_pair_index_find(*item, (IdValue*)userdata_pouch_get(lobby_world_block, 1), count) < 0 &&
                        (s32)item_slots_count_sum(*item, lobby_world_block->box_0x180,
                                                  userdata_box_capacity(lobby_world_block)) <= 0) {
                        break;
                    }
                }
            }
        }
        rate += lb_combo_bonus_rates[i];
    } else if (mode == 1) {
        return 100;
    } else {
        rate = lb_combo_base_rates[recipe->rate_0x04];
    }
    if (rate > 100) {
        rate = 100;
    } else if (rate < 0) {
        rate = 0;
    }
    return rate;
}

/* Marks `recipe` known: sets its bit in `lobby_world_block->bits_0x3960`. */
void lb_combo_recipe_learn(LbComboRecipe* recipe) {
    u8 bit = recipe->known_bit_0x06;

    lobby_world_block->bits_0x3960[bit >> 3] |= 1 << (bit & 7);
}

/* 1 when `recipe` is known (its bit in `bits_0x3960` is set); with bit 0 of `mode` clear it must also be open. */
u32 lb_combo_recipe_known_ck(LbComboRecipe* recipe, s32 mode) {
    LbMoveEntry* moves;
    u8 bit;

    if ((mode & 1) == 0) {
        moves = (LbMoveEntry*)get_move_work_adrs(2);
        if (moves == NULL) {
            return 0;
        }
        if (lb_combo_recipe_open_ck((_PLW*)&moves[fn_800CF384()], recipe) != 1) {
            return 0;
        }
    }
    bit = recipe->known_bit_0x06;
    if (((1 << (bit & 7)) & lobby_world_block->bits_0x3960[bit >> 3]) == 0) {
        return 0;
    }
    return 1;
}

/* Combines `item_a` and `item_b`: rolls the recipe's success rate, writes the yield (the fixed yield in mode 1 or
 * with skill 0xA0, else rolled down the odds list) and takes one of each ingredient from the player (mode 0), the
 * user data (mode 1) or the box (mode 2, by slot when the slot still holds it).  Returns the result item, -1 on a
 * failure or when there is no recipe. */
s16 lb_combo_exec(u8* yield, u16 item_a, u16 item_b, s32 mode, u16 slot_a, u16 slot_b) {
    LbComboRecipe* recipe;
    LbMoveEntry* moves;
    _PLW* plw;
    u8 roll;
    u8* odds;
    u8* step;
    s32 rate;
    s16 result;

    recipe = lb_combo_recipe_find(item_a, item_b, 0);
    if (recipe == NULL) {
        return -1;
    }
    rate = lb_combo_rate_find(item_a, item_b, mode);
    if (lb_combo_recipe_known_ck(recipe, 0) == 0) {
        rate = 100;
    }
    if ((s32)(ran_suu(1) % 100) < rate) {
        result = recipe->result_0x02;
        if (mode == 1) {
            *yield = lb_combo_yield_tbl[recipe->yield_0x05];
        } else {
            moves = (LbMoveEntry*)get_move_work_adrs(2);
            if (pl_skill_have_ck((_PLW*)&moves[fn_800CF384()], 0xA0) == 1) {
                *yield = lb_combo_yield_tbl[recipe->yield_0x05];
            } else {
                roll = ran_suu(1) % 100;
                odds = lb_combo_yield_odds[recipe->yield_0x05];
                *yield = odds[0];
                step = odds + 1;
                for (;;) {
                    if (roll < *step) {
                        break;
                    }
                    (*yield)++;
                    roll -= *step;
                    step++;
                }
            }
        }
    } else {
        result = -1;
        *yield = 0;
    }
    switch (mode) {
    case 0:
        if (GameMode_ck() == 2) {
            userdata_item_give(lobby_world_block, item_a, -1, 1);
            userdata_item_give(lobby_world_block, item_b, -1, 1);
        } else {
            moves = (LbMoveEntry*)get_move_work_adrs(2);
            if (moves != NULL) {
                plw = (_PLW*)&moves[fn_800CF384()];
                pl_item_add(plw, item_a, -1);
                pl_item_add(plw, item_b, -1);
            }
        }
        break;
    case 1:
        userdata_item_consume(lobby_world_block, item_a, -1);
        userdata_item_consume(lobby_world_block, item_b, -1);
        break;
    case 2:
        if (slot_a != 0xFFFF && item_a == lobby_world_block->box_0x180[slot_a].id) {
            item_box_slot_take(lobby_world_block->box_0x180, slot_a, -1);
        } else {
            item_box_take(item_a, -1);
        }
        if (slot_b != 0xFFFF && item_b == lobby_world_block->box_0x180[slot_b].id) {
            item_box_slot_take(lobby_world_block->box_0x180, slot_b, -1);
        } else {
            item_box_take(item_b, -1);
        }
        break;
    }
    return result;
}

/* Prints the recipe's yield into `text`: "n" when the odds list cannot raise it, else "min-max". */
void lb_combo_rate_sprintf(char* text, LbComboRecipe* recipe) {
    u8 group = recipe->yield_0x05;
    u8 low = lb_combo_yield_odds[group][0];
    u8 high = lb_combo_yield_tbl[group];

    if (low == high) {
        sprintf(text, "%d", low, high);
        return;
    }
    sprintf(text, "%d-%d", low, high);
}

/* Rebuilds the delivery list for `plw` and the NPC record `npc`: every `lb_deli_list_data` row whose unlock condition
 * and progress flags allow it, at most ten; returns the count. */
s16 lb_deli_list_build(struct _PLW* plw, u8* npc) {
    LbDeliWork* work = &lb_deli_work;
    LbDeliEntry* entry = (LbDeliEntry*)lb_deli_list_data;

    memset(work, 0, sizeof(LbDeliWork));
    work->plw_0xD4 = plw;
    work->npc_0xD0 = npc;
    work->count_0x0A = 0;
    for (; entry->cond_0x00 != 0xFFFF; entry++) {
        if (lb_unlock_cond_ck(entry->cond_0x00) == 1
            && (entry->need_0x02 == 0xFFFF || userdata_progress_flag_ck(entry->need_0x02) == 1)
            && (entry->need_0x04 == 0xFFFF || userdata_progress_flag_ck(entry->need_0x04) == 1)
            && (entry->flag_0x08 == -1 || userdata_progress_flag_ck(entry->flag_0x08) == 0)) {
            lb_deli_entry_copy(&work->entries_0x30[work->count_0x0A], entry);
            if (++work->count_0x0A >= 10) {
                break;
            }
        }
    }
    return work->count_0x0A;
}

/* Copies one 0x10-byte delivery list row field by field. */
void lb_deli_entry_copy(LbDeliEntry* dst, LbDeliEntry* src) {
    dst->cond_0x00 = src->cond_0x00;
    dst->need_0x02 = src->need_0x02;
    dst->need_0x04 = src->need_0x04;
    dst->order_0x06 = src->order_0x06;
    dst->flag_0x08 = src->flag_0x08;
    dst->talk_0x0C = src->talk_0x0C;
}

/* A tail call of `lb_panel_close`, the area-name/quest path helper the tutorial block drives. */
void lb_area_name_apply(void) {
    lb_panel_close();
}

/* 1 when the player can pay one cost of a delivery: enough zenny (item 0xFFFF), an empty slot, or enough of the item. */
s32 lb_deli_item_have_ck(LbItemCount* item) {
    if (item->item_0x00 == 0xFFFF) {
        if (item->count_0x02 <= lobby_world_block->zenny_0x3F04) {
            return 1;
        }
    } else if (item->item_0x00 == 0) {
        return 1;
    } else if (item->count_0x02 <= userdata_item_count_total(item->item_0x00, (u8*)lobby_world_block)) {
        return 1;
    }
    return 0;
}

/* 1 when every one of the delivery's four costs can be paid. */
s32 lb_deli_order_ready_ck(LbDeliOrder* order) {
    LbItemCount* item;
    s32 i;

    for (i = 0, item = order->items_0x00; i < 4; item++, i++) {
        if (lb_deli_item_have_ck(item) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Pays the delivery of list row `entry` (zenny and items) and unlocks its progress flag. */
void lb_deli_order_pay(LbDeliEntry* entry) {
    LbItemCount* item;
    s32 i;

    i = 0;
    item = ((LbDeliOrder*)lb_deli_data)[entry->order_0x06].items_0x00;
    for (; i < 4; item++, i++) {
        switch (item->item_0x00) {
        case 0xFFFF:
            userdata_zenny_add(-item->count_0x02);
            break;
        case 0:
            break;
        default:
            eft052_page_count_add(item->item_0x00, item->count_0x02);
            break;
        }
    }
    if (entry->flag_0x08 != -1) {
        lb_progress_unlock(entry->flag_0x08);
    }
}

/* One input step of the delivery menu: picks a row, confirms it, pays and opens its talk page; 2 on cancel, 1 once the
 * talk page has closed. */
s32 lb_deli_menu_step(void) {
    LbDeliWork* work = &lb_deli_work;
    s32 result = 0;

    work->moved_0x04 = 0;
    if (work->anim_0x02 == 7) {
        work->anim_0x02 = 8;
    }
    switch (work->menu_state_0x01) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (lb_deli_order_ready_ck(&((LbDeliOrder*)lb_deli_data)[work->entries_0x30[work->cursor_0x08].order_0x06])
                != 0) {
                work->menu_state_0x01 = 1;
                work->confirm_0x0C = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->cursor_0x08 = menu_cursor_step_forward(work->cursor_0x08, work->count_0x0A, lb_cmd_repeat_get(), 4, 8,
                                                         6, &work->moved_0x04);
        }
        break;
    case 1:
        switch (toggle_word_step(&work->confirm_0x0C, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->menu_state_0x01 = 2;
            lb_deli_order_pay(&work->entries_0x30[work->cursor_0x08]);
            lb_talk_page_open(work->entries_0x30[work->cursor_0x08].talk_0x0C);
            work->anim_0x02 = 7;
            sysSE_stop(0x14);
            break;
        case 2:
            work->menu_state_0x01 = 0;
            break;
        }
        break;
    case 2:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            result = 1;
            lb_area_name_apply();
        }
        break;
    }
    return result;
}

/* The area-select screen step: opens the choice, runs the delivery menu or closes, then queues the screen's draw. */
void lb_area_select_screen_step(void) {
    LbDeliWork* work = &lb_deli_work;

    switch (work->state_0x00) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->state_0x00++;
            lb_choice_init(&work->list_0x10, (const LbListTmpl*)&lb_deli_choice_tmpl, 0, 0);
            sysSE_req(5);
            camera_talk_lock_set(1);
        }
        break;
    case 1:
        switch (lb_choice_step(&work->list_0x10)) {
        case 1:
            work->state_0x00 = 2;
            work->menu_state_0x01 = 0;
            break;
        case 2:
            work->state_0x00 = 3;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        switch (lb_deli_menu_step()) {
        case 1:
            return;
        case 2:
            work->state_0x00 = 0;
            break;
        }
        break;
    case 3:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_area_name_apply();
        }
        break;
    }
    subTransSet((u32)lb_area_select_screen_draw, 0, 0);
}

/* Draws one cost line of the delivery window at layout `layout` moved by `offset` (zenny or an item, greyed when it
 * cannot be paid) and files its help record for the NPC `npc`. */
void lb_deli_cost_draw(LbItemCount* item, u16 layout, _mh_ivec2_* offset, u8 npc) {
    _mh_ivec2_ pos;
    PlaceRec rec;
    s8 text[0x20];
    _SPR_DATA_ spr;
    s32 have;

    if (item->item_0x00 != 0) {
        get_lsp_data(layout, &pos);
        pos.x += offset->x;
        pos.y += offset->y;
        if (item->item_0x00 == 0xFFFF) {
            draw_sprite_anim_ary(lb_deli_cost_sprite_ids, 1, &pos);
            sprite_frame_apply(&spr, 0x1E9D, 1, NULL);
            if (lobby_world_block->zenny_0x3F04 < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, (s8*)LbStr(0, 6), 0, &pos);
            sprintf((char*)text, "%d", item->count_0x02);
            sprite_frame_apply(&spr, 0x1E9E, 1, NULL);
            if (lobby_world_block->zenny_0x3F04 < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, text, 2, &pos);
            sprite_frame_apply(&spr, 0x1E9F, 1, NULL);
            if (lobby_world_block->zenny_0x3F04 < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, (s8*)LbStr(1, 0x2C), 0, &pos);
            sprintf((char*)text, "%d", lobby_world_block->zenny_0x3F04);
            sprite_frame_apply(&spr, 0x1EA0, 1, NULL);
            if (lobby_world_block->zenny_0x3F04 < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, text, 2, &pos);
            sprite_frame_apply(&spr, 0x1EA1, 1, NULL);
            if (lobby_world_block->zenny_0x3F04 < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, (s8*)LbStr(1, 0x2C), 0, &pos);
            place_rec_zenny_set(&rec, npc, 0, item->count_0x02);
        } else {
            have = userdata_item_count_total(item->item_0x00, (u8*)lobby_world_block);
            draw_sprite_anim_ary(lb_deli_cost_sprite_ids, 0, &pos);
            draw_itemicon_item_id_idx(0x1E99, item->item_0x00, &pos);
            spr_data_copy(&spr, get_lsp_data(0x1E96, NULL));
            if (have < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, (s8*)ItemName(item->item_0x00), 0, &pos);
            sprintf((char*)text, "%d", item->count_0x02);
            spr_data_copy(&spr, get_lsp_data(0x1E97, NULL));
            if (have < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, text, 2, &pos);
            sprintf((char*)text, "%d", have);
            spr_data_copy(&spr, get_lsp_data(0x1E98, NULL));
            if (have < item->count_0x02) {
                spr.color = 0x505050FF;
            }
            draw_font(spr, text, 2, &pos);
            place_rec_item_set(&rec, npc, item->item_0x00, item->count_0x02);
        }
        place_rec_alloc_idx(&rec, layout, offset, -4, 9);
    }
}

/* Draws the delivery window of `lb_deli_data` row `order_index`: its frame, texts, four cost lines and talk mark. */
void lb_deli_window_draw(LbDeliWork* work, s16 order_index, u8 npc) {
    _mh_ivec2_ pos;
    LbDeliOrder* order;
    u16* layout;
    LbItemCount* item;
    s32 i;

    order = &((LbDeliOrder*)lb_deli_data)[order_index];
    get_lsp_data(0x1E71, &pos);
    draw_sprite_ary(lb_deli_window_sprite_ids, &pos);
    get_lsp_data(0x1E8A, &pos);
    draw_sprite_ary(lb_deli_title_sprite_ids, &pos);
    draw_font_idx(0x1E8C, order->title_0x14, 1, &pos);
    draw_font_idx(0x1E8D, order->client_0x18, 0, &pos);
    draw_font_idx(0x1E8E, order->text_0x10, 0, &pos);
    get_lsp_data(0x1EA4, &pos);
    draw_sprite_ary(lb_deli_cost_frame_sprite_ids, &pos);
    item = order->items_0x00;
    for (i = 0, layout = lb_deli_cost_layout_ids; i < 4; layout++, item++, i++) {
        lb_deli_cost_draw(item, *layout, &pos, npc);
    }
    if (work->anim_0x02 != 0) {
        get_lsp_data(0x1570, &pos);
        draw_sprite_anim_idx(0x1571, work->anim_0x02, &pos);
    }
    ainpc_page_mark_b_draw(0x1EB5, NULL);
}

/* Draws the delivery menu for its state: the list with its arrows and help line, the confirm, or the window alone. */
void lb_deli_menu_draw(void) {
    LbDeliWork* work = &lb_deli_work;

    switch (work->menu_state_0x01) {
    case 0:
        lb_deli_window_draw(work, work->entries_0x30[work->cursor_0x08].order_0x06, work->npc_0xD0[2]);
        lb_page_arrow_draw(0x1E70, work->cursor_0x08, work->count_0x0A, work->moved_0x04, 1);
        lb_panel_msg_draw(0x1877, 0x15D);
        if (lb_deli_order_ready_ck(&((LbDeliOrder*)lb_deli_data)[work->entries_0x30[work->cursor_0x08].order_0x06])
            == 0) {
            lb_panel_str_print(0x1877, 0x15E, 2);
        }
        break;
    case 1:
        lb_deli_window_draw(work, work->entries_0x30[work->cursor_0x08].order_0x06, work->npc_0xD0[2]);
        lb_page_arrow_draw(0x1E70, work->cursor_0x08, work->count_0x0A, work->moved_0x04, 0);
        lb_panel_msg_draw(0x1877, 0x15F);
        lb_panel_yes_no_draw(0x1879, work->confirm_0x0C);
        break;
    case 2:
        lb_deli_window_draw(work, work->entries_0x30[work->cursor_0x08].order_0x06, work->npc_0xD0[2]);
        break;
    }
}

/* The area-select screen's draw task: the choice list, or the delivery menu. */
void lb_area_select_screen_draw(void) {
    LbDeliWork* work = &lb_deli_work;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (lb_deli_work.state_0x00) {
    case 1:
        lb_npc_choice_draw(&work->list_0x10);
        break;
    case 2:
        lb_deli_menu_draw();
        break;
    }
}

/* The page block's id-table row `&lobby_world_block->ids_0x5180[index]` (the entry the companion page binds);
 * `ef/eft050.cpp` calls it. */
LbEntryId* lb_entry_id_get(u8 index) {
    return &lobby_world_block->ids_0x5180[index];
}

/* Clears the block's 0x80 "changed" bit and republishes the entry byte to `lb_param_w`. */
void lb_entry_changed_clr(void) {
    lobby_world_block->entry_0x3E03 = (u8)(lobby_world_block->entry_0x3E03 & 0x7F);
    lb_param_w.entry_0x08 = (u8)lobby_world_block->entry_0x3E03;
}

/* Republishes the selected entry's id and its five sub-values into `lb_param_w`. */
void lb_entry_publish(void) {
    LbEntryId* entry;
    u8 index;

    lb_param_w.entry_0x08 = lobby_world_block->entry_0x3E03;
    index = lobby_world_block->entry_0x3E03 & 0x7F;
    entry = lb_entry_id_get(index);
    lb_param_w.sub_0x30 = lobby_world_block->ids_0x51A6[index];
    lb_param_w.sub_0x26 = entry->skills_0x01[0];
    lb_param_w.sub_0x27 = entry->skills_0x01[1];
    lb_param_w.sub_0x28 = entry->skills_0x01[2];
    lb_param_w.sub_0x29 = lobby_world_block->byte_0x51A4;
    lb_param_w.sub_0x2A = lobby_world_block->byte_0x51A5;
    lb_param_w.sub_0x2C = lobby_world_block->word_0x51A0;
    lb_param_w.sub_0x2E = lobby_world_block->word_0x51A2;
}

/* Recomputes the block's page flags: ORs `ai_skill_mask_get(ai_npc_level_get(word_0x51A0))` into `flags_0x519C`. */
void lb_page_flags_update(void) {
    u32 mask = ai_skill_mask_get(ai_npc_level_get(lobby_world_block->word_0x51A0));
    lobby_world_block->flags_0x519C = lobby_world_block->flags_0x519C | mask;
}

/* Publishes the selected entry's model id (`fn_802D7C6C(ids_0x51A6[entry])`) into the block's id table. */
void lb_entry_model_publish(void) {
    u8 entry = lobby_world_block->entry_0x3E03 & 0x7F;

    lobby_world_block->ids_0x5180[entry].word_0x00 = fn_802D7C6C(lobby_world_block->ids_0x51A6[entry]);
}

/* Both of the block's refresh steps in order (`lb_page_flags_update`, `lb_entry_model_publish`). */
void lb_page_refresh(void) {
    lb_page_flags_update();
    lb_entry_model_publish();
}

/* Opens lobby screen 13 for `player` and the NPC `npc`: clears its work block, runs its first step and sets the input
 * delay. */
void lb_screen13_open(struct _PLW* player, struct _LB_NPC* npc) {
    LbLobbyWork* lobby = &lobby_w;
    LbScreen13Work* work = lobby->screen13_0x0AC;

    memset(work, 0, sizeof(LbScreen13Work));
    work->player_0x184 = player;
    work->npc_0x180 = npc;
    lobby_w.state_0x000 = 0xD;
    lobby->active_0x008 = 1;
    lb_screen13_step();
    work->timer_0x010 = 0x10;
}

/* Republishes the entry byte (`lb_entry_publish`) and then the page's name path (`lb_panel_close`). */
void lb_entry_republish(void) {
    lb_entry_publish();
    lb_panel_close();
}

/* 1 when an item is waiting in either of the block's collected-item runs. */
u32 lb_item_gets_pending_ck(void) {
    LbDataBlock* block;
    s16 i;

    i = 0;
    block = lobby_world_block;
    for (; i < 12; i++) {
        if (block->gets_0x51B4[i].item_0x00 != 0 && block->gets_0x51B4[i].count_0x02 != 0) {
            return 1;
        }
    }
    for (i = 0; i < 30; i++) {
        if (block->gets_0x51E4[i].item_0x00 != 0 && block->gets_0x51E4[i].count_0x02 != 0) {
            return 1;
        }
    }
    return 0;
}

/* Unlocks the skills whose progress flags are now set (`lb_skill_unlock_items`, `lb_skill_unlock_flags`) and the two
 * dances, announcing each on the message line. */
void lb_skill_unlock_announce(void) {
    s8 text[0x40];
    LbSkillUnlock* unlock;
    LbSkillFlag* flag;

    for (unlock = lb_skill_unlock_items; unlock->need_0x00 != 0xFFFF; unlock++) {
        if (userdata_progress_flag_ck(unlock->done_0x02) == 0 && userdata_progress_flag_ck(unlock->need_0x00) != 0) {
            lobby_world_block->flags_0x519C |= 1 << unlock->skill_0x04;
            userdata_progress_flag_set(unlock->done_0x02);
            sprintf((char*)text, (char*)LbStr(0, 0x1C7), get_item_name_str(unlock->skill_0x04));
            hud_msg_push(0, (char*)text);
        }
    }
    for (flag = lb_skill_unlock_flags; flag->flag_0x00 != 0xFFFF; flag++) {
        if (userdata_progress_flag_ck(flag->flag_0x00) == 0 && (lobby_world_block->flags_0x519C & (1 << flag->skill_0x02)) != 0) {
            userdata_progress_flag_set(flag->flag_0x00);
            sprintf((char*)text, (char*)LbStr(0, 0x1C7), get_item_name_str(flag->skill_0x02));
            hud_msg_push(0, (char*)text);
        }
    }
    if (userdata_progress_flag_ck(0x113) == 0 && userdata_progress_flag_ck(0x4B) != 0) {
        sprintf((char*)text, (char*)LbStr(0, 0x1C7), get_group2_name_str(2));
        hud_msg_push(0, (char*)text);
        userdata_progress_flag_set(0x113);
    }
    if (userdata_progress_flag_ck(0x114) == 0 && userdata_progress_flag_ck(0x4C) != 0) {
        sprintf((char*)text, (char*)LbStr(0, 0x1C7), get_group2_name_str(3));
        hud_msg_push(0, (char*)text);
        userdata_progress_flag_set(0x114);
    }
}

/* The screen-13 step: hands collected items over, runs the main choice and the entry, dance, skill and status pages,
 * then queues the screen's draw. */
void lb_screen13_step(void) {
    LbScreen13Work* work = lobby_w.screen13_0x0AC;

    switch (work->state_0x000) {
    case 0:
        if (lb_item_gets_pending_ck() == 1) {
            work->state_0x000 = 7;
            lb_item_gets_box_open(work);
        } else {
            work->state_0x000 = 1;
            lb_skill_unlock_announce();
        }
        memset(&work->menu_0x050, 0, sizeof(LbList));
        work->menu_0x050.se_ok_0x14 = -1;
        work->menu_0x050.flags_0x18 = 3;
        work->menu_0x050.count_0x06 = 5;
        sysSE_req(5);
        break;
    case 1:
        ainpc_page_hold_set();
        if (--work->timer_0x010 > 0) {
            break;
        }
        work->timer_0x010 = 0;
        if (work->help_0x003 == 1) {
            if (lb_cmd_pressed_ck(0x80) != 0) {
                work->help_0x003 = 0;
                sysSE_req(5);
            }
        } else if (lb_cmd_pressed_ck(0x80) != 0) {
            work->help_0x003 = 1;
            sysSE_req(1);
        } else {
            switch (lb_choice_step(&work->menu_0x050)) {
            case 1:
                switch (work->menu_0x050.cursor_0x00) {
                case 0:
                    work->state_0x000 = 2;
                    if ((lobby_world_block->entry_0x3E03 & 0x80) != 0) {
                        lb_talk_page_open(1);
                        work->talk_0x048 = -0x18;
                        work->talk_arg_0x04C = 1;
                    } else {
                        lb_talk_page_open(0);
                        work->talk_0x048 = -0x17;
                        work->talk_arg_0x04C = 1;
                    }
                    sysSE_req(0);
                    break;
                case 1:
                    work->state_0x000 = 3;
                    lb_entry_list_init(work);
                    sysSE_req(0x1B);
                    break;
                case 2:
                    work->state_0x000 = 4;
                    lb_dance_list_init(work);
                    sysSE_req(5);
                    break;
                case 3:
                    work->state_0x000 = 5;
                    lb_skill_entry_list_init(work);
                    sysSE_req(0x1B);
                    break;
                case 4:
                    work->state_0x000 = 6;
                    lb_status_preview_update(work);
                    sysSE_req(0x1B);
                    break;
                }
                break;
            case 2:
                work->state_0x000 = 8;
                break;
            }
        }
        break;
    case 7:
        ainpc_page_hold_set();
        if ((u32)(eft052_item_get_step() - 1) <= 1) {
            work->state_0x000 = 1;
            lb_skill_unlock_announce();
            memset(lobby_world_block->gets_0x51B4, 0, sizeof(lobby_world_block->gets_0x51B4));
            memset(lobby_world_block->gets_0x51E4, 0, sizeof(lobby_world_block->gets_0x51E4));
        }
        break;
    case 6:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20) != 0) {
            work->state_0x000 = 1;
            sysSE_req(1);
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if ((u32)(lb_entry_list_step(work) - 1) <= 1) {
            work->state_0x000 = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(lb_dance_list_step(work) - 1) <= 1) {
            work->state_0x000 = 1;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if ((u32)(lb_skill_menu_step(work) - 1) <= 1) {
            work->state_0x000 = 1;
        }
        break;
    case 2:
        if ((u32)(lb_menu_step(1, 0, work->talk_0x048, work->talk_arg_0x04C) - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                if ((lb_param_w.entry_0x08 & 0x80) == 0) {
                    lb_param_w.entry_0x08 = lobby_world_block->entry_0x3E03 | 0x80;
                } else {
                    lb_param_w.entry_0x08 = lb_param_w.entry_0x08 & 0x7F;
                }
            } else if ((lb_param_w.entry_0x08 & 0x80) == 0) {
                lb_param_w.entry_0x08 = lb_param_w.entry_0x08 & 0x7F;
            } else {
                lb_param_w.entry_0x08 = lobby_world_block->entry_0x3E03 | 0x80;
            }
            lb_npc_entry_flag_set(work->npc_0x180, lb_param_w.entry_0x08);
            lobby_world_block->entry_0x3E03 = lb_param_w.entry_0x08;
            lb_entry_republish();
        }
        break;
    case 8:
        lb_entry_republish();
        break;
    }
    subTransSet((u32)lb_screen13_draw, 0, 0);
}

/* Copies the collected items into the work block and opens the item box over them. */
void lb_item_gets_box_open(LbScreen13Work* work) {
    memset(work->gets_0x0B8, 0, 0xC8);
    memcpy(work->gets_0x0B8, lobby_world_block->gets_0x51B4, sizeof(lobby_world_block->gets_0x51B4));
    memcpy(work->gets_0x108, lobby_world_block->gets_0x51E4, sizeof(lobby_world_block->gets_0x51E4));
    eft052_box_open((Eft052BoxCell*)work->gets_0x0B8, NULL, (Eft052BoxGrid*)(lb_item_get_data + 0xC0));
}

/* Fills the status preview of the selected entry's setup (its skills, the two levels and the entry's level). */
void lb_status_preview_update(LbScreen13Work* work) {
    LbCompanionSetup setup;
    u8 index;
    LbEntryId* entry;

    index = lobby_world_block->entry_0x3E03 & 0x7F;
    setup.entry_0x0 = index;
    entry = lb_entry_id_get(index);
    setup.skills_0x1[0] = entry->skills_0x01[0];
    setup.skills_0x1[1] = entry->skills_0x01[1];
    setup.skills_0x1[2] = entry->skills_0x01[2];
    setup.attack_0x4 = lobby_world_block->word_0x51A0;
    setup.gauge_0x6 = lobby_world_block->word_0x51A2;
    setup.level_0x8 = lobby_world_block->ids_0x51A6[index];
    ai_npc_status_preview((struct AINPCSetup*)&setup, (struct AINPCStatus*)work->status_0x09C);
}

/* Greys out the list row of the entry the block has selected and sets the list's row count. */
void lb_page_bits_set(LbScreen13Work* work) {
    u8 i;
    u32 selected;

    selected = lobby_world_block->entry_0x3E03 & 0x7F;
    work->list_0x070.count_0x06 = work->count_0x006;
    for (i = 0; i < work->count_0x006; i++) {
        if (selected == work->ids_0x008[i]) {
            work->list_0x070.off_mask_0x02 = (u16)(1 << i);
        }
    }
}

/* Lists the entries whose unlock flag is set (`lb_entry_unlock_flags`) and sets the entry list up. */
void lb_entry_list_init(LbScreen13Work* work) {
    u16* flag;
    s32 i;

    work->sub_0x001 = 0;
    memset(work->ids_0x008, 0xFF, 7);
    work->count_0x006 = 0;
    for (i = 0, flag = lb_entry_unlock_flags; i < 7; flag++, i++) {
        if (*flag == 0xFFFF || userdata_progress_flag_ck(*flag) == 1) {
            work->ids_0x008[work->count_0x006] = i;
            work->count_0x006++;
        }
    }
    memset(&work->list_0x070, 0, sizeof(LbList));
    work->list_0x070.se_ok_0x14 = -1;
    work->list_0x070.flags_0x18 = 3;
    lb_page_bits_set(work);
}

/* One step of the entry list: picks an entry, confirms it and makes it the selected one; 2 on cancel. */
s32 lb_entry_list_step(LbScreen13Work* work) {
    s32 result = 0;
    u8 entry;

    switch (work->sub_0x001) {
    case 0:
        switch (lb_choice_step(&work->list_0x070)) {
        case 1:
            work->sub_0x001 = 1;
            work->confirm_0x044 = 0;
            sysSE_req(0);
            break;
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        switch (toggle_word_step(&work->confirm_0x044, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->sub_0x001 = 0;
            entry = (lobby_world_block->entry_0x3E03 & 0x80) | work->ids_0x008[work->list_0x070.cursor_0x00];
            lb_param_w.entry_0x08 = entry;
            lobby_world_block->entry_0x3E03 = entry;
            sysSE_stop(0x16);
            lb_page_bits_set(work);
            break;
        case 2:
            work->sub_0x001 = 0;
            break;
        }
        break;
    }
    return result;
}

/* Lists the dances (the two base ones and those the progress flags 0x4B/0x4C open) with the picked pair marked. */
void lb_dance_list_init(LbScreen13Work* work) {
    work->sub_0x001 = 0;
    work->confirm_0x044 = 0;
    memset(work->dances_0x012, 0xFF, 4);
    memset(&work->list_0x070, 0, sizeof(LbList));
    work->dances_0x012[0] = 0;
    work->dances_0x012[1] = 1;
    work->list_0x070.count_0x06 = 2;
    if (userdata_progress_flag_ck(0x4B) != 0) {
        work->dances_0x012[work->list_0x070.count_0x06] = 2;
        work->list_0x070.count_0x06++;
    }
    if (userdata_progress_flag_ck(0x4C) != 0) {
        work->dances_0x012[work->list_0x070.count_0x06] = 3;
        work->list_0x070.count_0x06++;
    }
    work->dance_0x016[0] = lobby_world_block->byte_0x51A4;
    work->dance_0x016[1] = lobby_world_block->byte_0x51A5;
    work->list_0x070.off_mask_0x02 = (1 << work->dance_0x016[0]) | (1 << work->dance_0x016[1]);
}

/* One step of the dance page: with more than two dances, picks two in turn and confirms them; 1 once closed, 2 on
 * cancel. */
s32 lb_dance_list_step(LbScreen13Work* work) {
    s32 result = 0;
    s16 cursor;

    switch (work->sub_0x001) {
    case 0:
        if (work->list_0x070.count_0x06 <= 2) {
            if (lb_cmd_pressed_ck(0x20) != 0) {
                result = 2;
                sysSE_req(1);
            }
        } else {
            switch (toggle_word_step(&work->confirm_0x044, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
            case 1:
                work->sub_0x001 = 1;
                work->list_0x070.off_mask_0x02 = 0;
                work->dance_0x016[0] = 0xFF;
                work->dance_0x016[1] = 0xFF;
                sysSE_req(5);
                break;
            case 2:
                result = 2;
                break;
            }
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            cursor = work->list_0x070.cursor_0x00;
            if (work->dance_0x016[0] == work->dances_0x012[cursor] || work->dance_0x016[1] == work->dances_0x012[cursor]) {
                sysSE_req(2);
            } else if (work->dance_0x016[0] == 0xFF) {
                work->dance_0x016[0] = work->dances_0x012[cursor];
                work->list_0x070.off_mask_0x02 |= (u16)(1 << cursor);
                sysSE_req(0);
            } else if (work->dance_0x016[1] == 0xFF) {
                work->sub_0x001 = 2;
                work->confirm_0x044 = 0;
                work->dance_0x016[1] = work->dances_0x012[cursor];
                work->list_0x070.off_mask_0x02 |= (u16)(1 << cursor);
                sysSE_req(0);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (work->dance_0x016[0] == 0xFF) {
                result = 2;
                sysSE_req(1);
            } else if (work->dance_0x016[1] == 0xFF) {
                work->list_0x070.off_mask_0x02 = 0;
                work->dance_0x016[0] = 0xFF;
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->list_0x070.cursor_0x00 = menu_cursor_step(work->list_0x070.cursor_0x00, work->list_0x070.count_0x06,
                                                            lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 2:
        switch (toggle_word_step(&work->confirm_0x044, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->sub_0x001 = 3;
            lobby_world_block->byte_0x51A4 = work->dance_0x016[0];
            lobby_world_block->byte_0x51A5 = work->dance_0x016[1];
            sysSE_stop(0x16);
            break;
        case 2:
            work->sub_0x001 = 1;
            work->dance_0x016[1] = 0xFF;
            work->list_0x070.off_mask_0x02 = 1 << work->dance_0x016[0];
            break;
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            result = 1;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* Lists the entries for the skill page and greys out those whose level opens no skill slot. */
void lb_skill_entry_list_init(LbScreen13Work* work) {
    s16 i;
    const u8* behaviour;
    u8* id;

    lb_entry_list_init(work);
    work->list_0x070.off_mask_0x02 = 0;
    work->list_0x070.se_ok_0x14 = 5;
    work->list_0x070.flags_0x18 = 3;
    for (i = 0; i < work->list_0x070.count_0x06; i++) {
        id = &work->ids_0x008[i];
        behaviour = ai_npc_behaviour_get(*id);
        if (behaviour[lb_entry_id_get(*id)->level_0x00 + 4] == 0) {
            work->list_0x070.off_mask_0x02 |= (u16)(1 << i);
        }
    }
}

/* One step of the skill page: picks an entry, a slot and an action (change or remove), runs the skill list and
 * confirms or drops the edit; 2 on cancel. */
s32 lb_skill_menu_step(LbScreen13Work* work) {
    s32 result = 0;
    s16 slots;
    u8* skill;

    switch (work->sub_0x001) {
    case 0:
        switch (lb_choice_step(&work->list_0x070)) {
        case 1:
            work->sub_0x001 = 1;
            work->behaviour_0x094 = ai_npc_behaviour_get(work->ids_0x008[work->list_0x070.cursor_0x00]);
            work->entry_0x090 = lb_entry_id_get(work->ids_0x008[work->list_0x070.cursor_0x00]);
            memset(&work->edit_0x098, 0, sizeof(LbEntryId));
            memcpy(&work->edit_0x098, work->entry_0x090, sizeof(LbEntryId));
            work->entry_0x090 = &work->edit_0x098;
            work->slot_0x034 = 0;
            work->changed_0x004 = 0;
            break;
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (work->slot_0x034 == (work->behaviour_0x094 + work->entry_0x090->level_0x00)[4]) {
                work->sub_0x001 = 4;
                work->confirm_0x044 = 0;
                sysSE_req(0);
            } else {
                work->sub_0x001 = 2;
                work->action_0x036 = 0;
                sysSE_req(5);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (work->changed_0x004 == 0) {
                work->sub_0x001 = 0;
            } else {
                work->sub_0x001 = 5;
                work->confirm_0x044 = 0;
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            slots = (work->behaviour_0x094 + work->entry_0x090->level_0x00)[4];
            if (work->changed_0x004 != 0) {
                slots++;
            }
            work->slot_0x034 = menu_cursor_step(work->slot_0x034, slots, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (work->action_0x036) {
            case 0:
                work->sub_0x001 = 3;
                lb_skill_pick_init(work);
                sysSE_req(5);
                break;
            case 1:
                skill = &work->entry_0x090->skills_0x01[work->slot_0x034];
                if (*skill != 0) {
                    work->sub_0x001 = 1;
                    *skill = 0;
                    work->changed_0x004 = 1;
                    lb_skill_clash_remove(work);
                    sysSE_stop(0x18);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            work->sub_0x001 = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->action_0x036 = menu_cursor_step(work->action_0x036, 2, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 3:
        switch (lb_skill_pick_step(work)) {
        case 1:
            work->sub_0x001 = 1;
            work->changed_0x004 = 1;
            break;
        case 2:
            work->sub_0x001 = 1;
            break;
        }
        break;
    case 4:
        switch (toggle_word_step(&work->confirm_0x044, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->sub_0x001 = 0;
            memcpy(lb_entry_id_get(work->ids_0x008[work->list_0x070.cursor_0x00]), &work->edit_0x098,
                   sizeof(LbEntryId));
            sysSE_stop(0x16);
            break;
        case 2:
            work->sub_0x001 = 1;
            break;
        }
        break;
    case 5:
        switch (lb_yes_no_step(&work->confirm_0x044)) {
        case 1:
            work->sub_0x001 = 0;
            break;
        case 2:
            work->sub_0x001 = 1;
            break;
        }
        break;
    }
    return result;
}

/* Collects the skills of the other slots and builds the first skill page. */
void lb_skill_pick_init(LbScreen13Work* work) {
    s16 i;
    s16 n;
    u8 skill;

    work->picking_0x002 = 0;
    work->cursor_0x038 = 0;
    memset(work->others_0x020, 0, 3);
    n = 0;
    for (i = 0; i < work->behaviour_0x094[work->entry_0x090->level_0x00 + 4]; i++) {
        if (i != work->slot_0x034) {
            skill = work->entry_0x090->skills_0x01[i];
            if (skill != 0) {
                work->others_0x020[n] = skill;
                n++;
            }
        }
    }
    lb_skill_page_build(work, 0);
}

/* Whether the NUL-terminated byte list contains `value`. */
u32 lb_byte_list_has(u8* list, u8 value) {
    while (*list != 0) {
        if (value == *list) {
            return 1;
        }
        list++;
    }
    return 0;
}

/* The clash flags of `skill` against the skills of the other slots: 2/4/8 when another skill of its exclusive group is
 * set, 0x10/0x20 when skill 7/11 lacks the skill it builds on. */
u16 lb_skill_clash_flags_get(LbScreen13Work* work, u8 skill) {
    u16 flags = 0;
    s16 i;
    s32 found;

    if (lb_byte_list_has(lb_skill_group_a, skill) == 1) {
        for (i = 0; i < 3; i++) {
            if (lb_byte_list_has(lb_skill_group_a, work->others_0x020[i]) == 1) {
                flags |= 2;
            }
        }
    }
    if (lb_byte_list_has(lb_skill_group_b, skill) == 1) {
        for (i = 0; i < 3; i++) {
            if (lb_byte_list_has(lb_skill_group_b, work->others_0x020[i]) == 1) {
                flags |= 4;
            }
        }
    }
    if (lb_byte_list_has(lb_skill_group_c, skill) == 1) {
        for (i = 0; i < 3; i++) {
            if (lb_byte_list_has(lb_skill_group_c, work->others_0x020[i]) == 1) {
                flags |= 8;
            }
        }
    }
    switch (skill) {
    case 7:
        found = 0;
        for (i = 0; i < 3; i++) {
            if (lb_byte_list_has(lb_skill_needs_7, work->others_0x020[i]) == 1) {
                found = 1;
            }
        }
        if (found == 0) {
            flags |= 0x10;
        }
        break;
    case 11:
        found = 0;
        for (i = 0; i < 3; i++) {
            if (lb_byte_list_has(lb_skill_needs_11, work->others_0x020[i]) == 1) {
                found = 1;
            }
        }
        if (found == 0) {
            flags |= 0x20;
        }
        break;
    }
    return flags;
}

/* Clears every set skill that now clashes with the others, announcing each removal. */
void lb_skill_clash_remove(LbScreen13Work* work) {
    s8 text[0x80];
    s16 slot;
    s16 i;
    s16 n;
    u8 skill;

    for (slot = 0; slot < work->behaviour_0x094[work->entry_0x090->level_0x00 + 4]; slot++) {
        memset(work->others_0x020, 0, 3);
        n = 0;
        for (i = 0; i < work->behaviour_0x094[work->entry_0x090->level_0x00 + 4]; i++) {
            skill = work->entry_0x090->skills_0x01[i];
            if (skill != 0 && i != slot) {
                work->others_0x020[n] = skill;
                n++;
            }
        }
        if (work->entry_0x090->skills_0x01[slot] != 0
            && lb_skill_clash_flags_get(work, work->entry_0x090->skills_0x01[slot]) != 0) {
            sprintf((char*)text, (char*)LbStr(3, 0x1A9), get_item_name_str(work->entry_0x090->skills_0x01[slot]));
            hud_msg_push(0, (char*)text);
            work->entry_0x090->skills_0x01[slot] = 0;
        }
    }
}

/* Builds skill page `page` (eight rows of the unlocked skills) with each row's set/clash flags and the page count. */
void lb_skill_page_build(LbScreen13Work* work, s16 page) {
    s16 skill;
    s16 i;
    s16 first;
    s16 last;
    s16 n;

    memset(work->skill_rows_0x018, 0, 8);
    first = page * 8;
    last = first + 7;
    n = 0;
    work->skill_count_0x03E = 0;
    for (skill = 1; skill < 0x1C; skill++) {
        if ((lobby_world_block->flags_0x519C & (1 << skill)) != 0) {
            if (first <= n && n <= last) {
                work->skill_rows_0x018[work->skill_count_0x03E] = skill;
                work->skill_flags_0x024[work->skill_count_0x03E] = 0;
                for (i = 0; i < work->behaviour_0x094[work->entry_0x090->level_0x00 + 4]; i++) {
                    if (work->skill_rows_0x018[work->skill_count_0x03E] == work->entry_0x090->skills_0x01[i]
                        && i != work->slot_0x034) {
                        work->skill_flags_0x024[work->skill_count_0x03E] |= 1;
                    }
                    work->skill_flags_0x024[work->skill_count_0x03E] |=
                        lb_skill_clash_flags_get(work, work->skill_rows_0x018[work->skill_count_0x03E]);
                }
                work->skill_count_0x03E++;
            }
            n++;
        }
    }
    if (work->cursor_0x038 >= work->skill_count_0x03E) {
        work->cursor_0x038 = work->skill_count_0x03E - 1;
    }
    work->page_0x03A = page;
    work->pages_0x03C = menu_page_count(n, 8);
}

/* One step of the skill list: sets the picked skill into the slot (refused when it is set or clashes), pages and moves
 * the cursor; 1 once set, 2 on cancel. */
s32 lb_skill_pick_step(LbScreen13Work* work) {
    s32 result = 0;
    s16 page;
    u8 skill;

    work->moved_0x040 = 0;
    if (work->picking_0x002 == 0) {
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (work->skill_flags_0x024[work->cursor_0x038] == 0) {
                result = 1;
                skill = work->skill_rows_0x018[work->cursor_0x038];
                work->entry_0x090->skills_0x01[work->slot_0x034] = skill;
                lb_skill_clash_remove(work);
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            page = menu_cursor_step_fixed_tail(work->page_0x03A, work->pages_0x03C, lb_cmd_repeat_get(), 4, 8,
                                               &work->moved_0x040);
            work->page_0x03A = page;
            lb_skill_page_build(work, page);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cursor_0x038 = menu_cursor_step(work->cursor_0x038, work->skill_count_0x03E, lb_cmd_repeat_get(), 1, 2);
        }
    }
    return result;
}

/* Draws the sub-page's arrow sprites at the layout position (`get_lsp_data(0x1EB6)`, `lb_subpage_arrow_sprite_ids`). */
void lb_subpage_arrow_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x1EB6, &pos);
    draw_sprite_ary(lb_subpage_arrow_sprite_ids, &pos);
}

/* Draws screen 13's main choice: the frame and its rows (the first row's text follows the entry's 0x80 bit). */
void lb_s13_menu_draw(LbScreen13Work* work) {
    _mh_ivec2_ pos;
    u16* layout;
    bool sel;
    u32 color;
    s16 i;
    u16 str;

    get_lsp_data(0x1EEC, &pos);
    draw_sprite_anim_ary(lb_s13_frame_sprite_ids, work->menu_0x050.count_0x06, &pos);
    draw_sprite_idx(0x1EEE, &pos);
    for (i = 0, layout = lb_s13_row_layout_ids; i < work->menu_0x050.count_0x06; layout++, i++) {
        sel = i == work->menu_0x050.cursor_0x00;
        color = GetMenuFontColor(true, sel, true, false);
        if (i == 0) {
            if ((lobby_world_block->entry_0x3E03 & 0x80) != 0) {
                str = 0x1AF;
            } else {
                str = 0x1B0;
            }
        } else {
            str = i + 0x1B0;
        }
        lb_frame_draw_at(0x1EEC, *layout, (s8*)LbStr(0, str), sel, color, 0);
    }
}

/* Draws the help window of the main choice and its on/off mark for the entry's 0x80 bit. */
void lb_s13_help_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x1F27, &pos);
    draw_sprite_ary(lb_s13_help_sprite_ids, &pos);
    draw_font_idx(0x1F28, (s8*)LbStr(0, 0x1B7), 1, &pos);
    if ((lobby_world_block->entry_0x3E03 & 0x80) != 0) {
        draw_sprite_idx(0x1F29, &pos);
        return;
    }
    draw_sprite_idx(0x1F2A, &pos);
}

/* Draws the entry list: one name row per listed entry, greyed when its list bit is set; `active` lights the cursor. */
void lb_s13_entry_list_draw(LbScreen13Work* work, bool active) {
    _mh_ivec2_ pos;
    u16* layout;
    bool sel;
    s16 i;
    u32 color;
    u8 entry;

    get_lsp_data(0x1EEC, &pos);
    draw_sprite_anim_ary(lb_s13_frame_sprite_ids, work->list_0x070.count_0x06, &pos);
    draw_sprite_idx(0x1EEF, &pos);
    for (i = 0, layout = lb_s13_row_layout_ids; i < work->list_0x070.count_0x06; layout++, i++) {
        sel = i == work->list_0x070.cursor_0x00;
        entry = work->ids_0x008[i];
        color = GetMenuFontColor((work->list_0x070.off_mask_0x02 & (1 << i)) == 0, sel, active, false);
        if (!active) {
            sel = false;
        }
        lb_frame_draw_at(0x1EEC, *layout, get_player_name_str(entry), sel, color, 7);
    }
}

/* Draws the status window of entry `entry`: its name, mask, level and the skills of its slots. */
void lb_s13_entry_status_draw(u8 entry) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    const u8* behaviour;
    LbEntryId* setup;
    u16* layout;
    s8 i;

    get_lsp_data(0x1F51, &base);
    draw_sprite_ary(lb_s13_status_sprite_ids, &base);
    draw_font_idx(0x1F52, get_player_name_str(entry), 1, &base);
    draw_font_idx(0x1F53, (s8*)lb_chacha_mask_name_get(entry), 0, &base);
    behaviour = ai_npc_behaviour_get(entry);
    setup = lb_entry_id_get(entry);
    draw_number_rec_idx(0x1F56, setup->level_0x00, 0, &base);
    for (i = 0, layout = lb_s13_status_slot_layout_ids; i < behaviour[setup->level_0x00 + 4]; layout++, i++) {
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_s13_skill_row_draw(setup->skills_0x01[i], true, false, true, false, &pos);
    }
}

/* Draws one skill row: its group badge (blinking when selected under the pointer), its frame (greyed when empty) and
 * its name in the menu colour of the four flags. */
void lb_s13_skill_row_draw(u8 skill, bool enabled, bool sel, bool active, bool marked, _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    s32 group;

    group = 0;
    if (skill != 0) {
        group = (u8)ai_npc_skill_group_get(skill) + 1;
    }
    draw_sprite_anim_ary(lb_s13_skill_badge_sprite_ids, group, pos);
    if (sel && active && chk_pointer() == 0) {
        spr_data_copy(&spr, get_lsp_data(0x1EDF, NULL));
        draw_sprite_blink(&spr, pos);
    }
    sprite_frame_apply(&spr, 0x1ED5, group, NULL);
    if (skill == 0) {
        spr.color = 0x464646FF;
    }
    draw_sprite(spr, pos);
    draw_sprite_anim_idx(0x1ED9, group, pos);
    if (sel && active && chk_pointer() == 0) {
        draw_sprite_ary(lb_s13_skill_cursor_sprite_ids, pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x1ED6, NULL));
    spr.color = GetMenuFontColor(enabled, sel, active, marked);
    draw_font(spr, get_item_name_str(skill), 4, pos);
}

/* Draws the dance list: one group row per listed dance, the picked ones marked; `active` lights the cursor. */
void lb_s13_dance_list_draw(LbScreen13Work* work, bool active) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16* layout;
    bool sel;
    s16 i;
    bool marked;

    get_lsp_data(0x1EEC, &base);
    draw_sprite_anim_ary(lb_s13_frame_sprite_ids, work->list_0x070.count_0x06, &base);
    draw_sprite_idx(0x1EF1, &base);
    for (i = 0, layout = lb_s13_row_layout_ids; i < work->list_0x070.count_0x06; layout++, i++) {
        if (i == work->list_0x070.cursor_0x00 && active) {
            sel = true;
        } else {
            sel = false;
        }
        marked = (work->list_0x070.off_mask_0x02 & (1 << i)) != 0;
        if (sel) {
            chk_pointer();
        }
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_group_row_draw(work->dances_0x012[i], true, sel, active, marked, 0, &pos);
    }
}

/* Draws the two picked dances, the one being picked under the cursor. */
void lb_s13_dance_pick_draw(LbScreen13Work* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u8 i;
    bool sel;

    get_lsp_data(0x1F88, &base);
    draw_sprite_idx(0x1FA3, &base);
    draw_sprite_ary(lb_s13_pick_sprite_ids, &base);
    for (i = 0; i < 2; i++) {
        get_lsp_data(lb_s13_pick_layout_ids[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        sel = false;
        if (work->sub_0x001 == 1) {
            if (work->dance_0x016[0] == 0xFF) {
                if (i == 0) {
                    sel = true;
                }
            } else if (work->dance_0x016[1] != 0 && i == 1) {
                sel = true;
            }
        }
        lb_group_row_draw(work->dance_0x016[i], true, sel, true, false, 1, &pos);
    }
}

/* Draws the dance pair's four values with their ranks and the two group colours of each. */
void lb_s13_dance_values_draw(LbScreen13Work* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 values[4];
    s32 i;
    s32 j;
    u16* layout;
    u16* value;
    u8 first;
    u8 second;

    get_lsp_data(0x1FBA, &base);
    draw_sprite_ary(lb_s13_value_sprite_ids, &base);
    ai_npc_pair_values_get(work->dance_0x016[0], work->dance_0x016[1], values);
    for (i = 0, j = 0, layout = lb_s13_value_layout_ids, value = values; i < 4; j += 2, layout++, value++, i++) {
        first = work->dance_0x016[lb_s13_value_pairs[j]];
        second = work->dance_0x016[lb_s13_value_pairs[j + 1]];
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        draw_sprite_ary(lb_s13_value_row_sprite_ids, &pos);
        draw_font_idx(0x1FAC, get_digit_str(*value), 4, &pos);
        draw_sprite_idx(lb_s13_rank_sprite_ids[(u8)ai_npc_level_rank_get(*value)], &pos);
        draw_sprite_idx_color(0x1FB0, lb_group_color_get(first), &pos);
        draw_sprite_idx_color(0x1FB1, lb_group_color_get(second), &pos);
    }
}

/* Puts screen 13's help line for its state on panel 0x1877 (and the warning line, or the yes/no choice, when one
 * applies). */
void lb_s13_help_line_draw(LbScreen13Work* work) {
    u16 str;
    u16 warn;
    u16 flags;

    str = 0xFFFF;
    warn = 0xFFFF;
    switch (work->state_0x000) {
    case 1:
        str = work->menu_0x050.cursor_0x00 + 0x189;
        if (work->menu_0x050.cursor_0x00 == 0 && (lb_param_w.entry_0x08 & 0x80) != 0) {
            str = 0x188;
        }
        break;
    case 3:
        switch (work->sub_0x001) {
        case 0:
            if (work->count_0x006 > 1) {
                str = 0x18F;
                if ((work->list_0x070.off_mask_0x02 & (1 << work->list_0x070.cursor_0x00)) != 0) {
                    warn = 0x192;
                }
            } else {
                str = 0x190;
                warn = 0x191;
            }
            break;
        case 1:
            str = 0x193;
            lb_panel_yes_no_draw(0x1879, work->confirm_0x044);
            break;
        }
        break;
    case 4:
        switch (work->sub_0x001) {
        case 0:
            if (work->list_0x070.count_0x06 <= 2) {
                str = 0x195;
                warn = 0x196;
            } else {
                str = 0x194;
                lb_panel_yes_no_draw(0x1879, work->confirm_0x044);
            }
            break;
        case 1:
            if (work->dance_0x016[0] == 0xFF) {
                str = 0x197;
            } else {
                str = 0x198;
                if (work->dance_0x016[0] == work->dances_0x012[work->list_0x070.cursor_0x00]) {
                    warn = 0x199;
                }
            }
            break;
        case 2:
            str = 0x19A;
            lb_panel_yes_no_draw(0x1879, work->confirm_0x044);
            break;
        case 3:
            str = 0x19B;
            break;
        }
        break;
    case 5:
        switch (work->sub_0x001) {
        case 0:
            str = 0x19C;
            if ((work->list_0x070.off_mask_0x02 & (1 << work->list_0x070.cursor_0x00)) != 0) {
                warn = 0x19D;
            }
            break;
        case 1:
            str = 0x19E;
            break;
        case 2:
            str = work->action_0x036 + 0x19F;
            if (work->action_0x036 == 1 && work->entry_0x090->skills_0x01[work->slot_0x034] == 0) {
                warn = 0x1A1;
            }
            break;
        case 4:
            str = 0x1A2;
            lb_panel_yes_no_draw(0x1879, work->confirm_0x044);
            break;
        case 3:
            if (work->picking_0x002 == 0) {
                str = 0x1A3;
                flags = work->skill_flags_0x024[work->cursor_0x038];
                if ((flags & 1) != 0) {
                    warn = 0x1A4;
                } else if ((flags & 2) != 0) {
                    warn = 0x1A5;
                } else if ((flags & 0x30) != 0) {
                    warn = 0x1A6;
                } else if ((flags & 4) != 0) {
                    warn = 0x1A7;
                } else if ((flags & 8) != 0) {
                    warn = 0x1A8;
                }
            }
            break;
        case 5:
            str = 0x1AA;
            lb_panel_yes_no_draw(0x1879, work->confirm_0x044);
            break;
        }
        break;
    case 6:
        str = 0x1AB;
        break;
    }
    if (str != 0xFFFF) {
        lb_panel_msg_draw(0x1877, str);
        if (warn != 0xFFFF) {
            lb_panel_str_print(0x1877, warn, 2);
        }
    }
}

/* Draws one slot action option `text` (a dash when NULL), blinking under the pointer when selected. */
void lb_s13_option_draw(s8* text, bool sel, bool enabled, _mh_ivec2_* pos) {
    _SPR_DATA_ spr;

    draw_sprite_ary(lb_s13_option_sprite_ids, pos);
    if (sel && chk_pointer() == 0) {
        spr_data_copy(&spr, get_lsp_data(0x1EEB, NULL));
        draw_sprite_blink(&spr, pos);
    }
    draw_sprite_ary(lb_s13_option_frame_sprite_ids, pos);
    if (sel && chk_pointer() == 0) {
        draw_sprite_ary(lb_s13_option_cursor_sprite_ids, pos);
    }
    if (text == NULL) {
        draw_sprite_idx(0x1F08, pos);
        return;
    }
    spr_data_copy(&spr, get_lsp_data(0x1EE1, NULL));
    spr.color = GetMenuFontColor(enabled, sel, true, false);
    draw_font(spr, text, 5, pos);
}

/* Draws the skill slots of the setup being edited and the confirm mark (blinking while the cursor is on it). */
void lb_s13_slot_list_draw(LbScreen13Work* work, u32 active) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s16 i;
    u16* layout;
    u8 slots;
    bool sel;
    s32 on_confirm;
    u16 mark;
    u16 anim;

    get_lsp_data(0x1EEC, &base);
    draw_sprite_anim_ary(lb_s13_frame_sprite_ids, 7, &base);
    draw_sprite_idx(0x1EF0, &base);
    draw_sprite_idx(0x1EF3, &base);
    draw_sprite_anim_ary(lb_s13_slot_frame_sprite_ids, 7, &base);
    draw_font_idx(0x1EED, get_player_name_str(work->ids_0x008[work->list_0x070.cursor_0x00]), 1, &base);
    for (i = 0, layout = lb_s13_slot_layout_ids;
         i < (slots = work->behaviour_0x094[work->entry_0x090->level_0x00 + 4]); layout++, i++) {
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        if (active == 1 && i == work->slot_0x034) {
            sel = true;
        } else {
            sel = false;
        }
        lb_s13_skill_row_draw(work->entry_0x090->skills_0x01[i], true, sel, true, false, &pos);
    }
    if (work->slot_0x034 == slots && work->sub_0x001 == 1) {
        on_confirm = 1;
    } else {
        on_confirm = 0;
    }
    mark = work->changed_0x004 != 0 ? 2 : 0;
    if (on_confirm != 0) {
        mark = 1;
        anim = (u32)lobby_w.counter_0x030 % 40 + 10;
    } else {
        anim = mark;
    }
    draw_sprite_anim_ary(lb_s13_confirm_sprite_ids, mark, &base);
    draw_sprite_anim_idx(0x1F08, anim, &base);
}

/* Draws the slot action options (change, remove), remove greyed for an empty slot. */
void lb_s13_slot_action_draw(LbScreen13Work* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16* layout;
    s16 i;
    bool sel;
    bool enabled;

    get_lsp_data(0x1F44, &base);
    draw_sprite_ary(lb_s13_action_sprite_ids, &base);
    for (i = 0, layout = lb_s13_action_layout_ids; i < 2; layout++, i++) {
        sel = work->action_0x036 == i;
        if (work->entry_0x090->skills_0x01[work->slot_0x034] == 0 && i == 1) {
            enabled = false;
        } else {
            enabled = true;
        }
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_s13_option_draw((s8*)LbStr(0, i + 0x1B5), sel, enabled, &pos);
    }
}

/* Draws the description window of skill `skill`: its row and its text. */
void lb_s13_skill_info_draw(u8 skill) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;

    get_lsp_data(0x1FCB, &base);
    draw_sprite_ary(lb_s13_info_sprite_ids, &base);
    get_lsp_data(0x1FDF, &pos);
    pos.x += base.x;
    pos.y += base.y;
    lb_s13_skill_row_draw(skill, true, false, true, false, &pos);
    draw_font_idx(0x1FCC, (s8*)lb_chacha_skill_name_get(skill), 0, &base);
}

/* Draws the skill list page: its rows with their set/clash flags, the page arrows and the cursor row's description. */
void lb_s13_skill_list_draw(LbScreen13Work* work, bool active) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16* layout;
    bool sel;
    s16 i;

    get_lsp_data(0x1EEC, &base);
    draw_sprite_anim_ary(lb_s13_frame_sprite_ids, 8, &base);
    draw_sprite_idx(0x1EF2, &base);
    lb_page_arrow_draw(0x1EBC, work->page_0x03A, work->pages_0x03C, work->moved_0x040, active);
    for (i = 0, layout = lb_s13_skill_layout_ids; i < work->skill_count_0x03E; layout++, i++) {
        if (i == work->cursor_0x038) {
            sel = true;
            lb_s13_skill_info_draw(work->skill_rows_0x018[i]);
        } else {
            sel = false;
        }
        get_lsp_data(*layout, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_s13_skill_row_draw(work->skill_rows_0x018[i], work->skill_flags_0x024[i] == 0, sel, active,
                              (work->skill_flags_0x024[i] & 1) != 0, &pos);
    }
}
}

