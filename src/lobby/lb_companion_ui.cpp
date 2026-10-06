/*
 * lobby/lb_companion_ui.cpp - the lobby companion/status UI band: the act dispatchers and their per-act handlers, the
 *   page/dialog chain and its drawing helpers, and the quest cockpit's map icon tables.
 * RANGE. .text 0x80338808-0x8033F270 (133 functions); .data 0x805E27D4-0x805E7410 (from `lb_act_dispatch`'s switch
 *   table `jumptable_805E27D4`; the icon tables 0x805E6A58-0x805E70F8), .bss 0x806BE340-0x806BF0A0, .sdata
 *   0x80792D38-0x8079308C (the icon id lists of 8 bytes or less and their pointer arrays, 0x80792D80-0x80792F8C), .sbss
 *   0x80794B90-0x80794BE0, .sdata2 0x8079B2B8-0x8079B2C0, extab, extabindex.  The seam is undecidable from the DOL
 *   (docs/lobby.md).  The icon tables are defined ahead of the code; the lists `hud/cockpit_quest.cpp` and
 *   `ef/eft035.cpp` read at 0x805E6EF8-0x805E70C8 are not emitted, so the slot and flash id lists sit 0x1F0 bytes
 *   early.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps `clrlwi` + `slwi` and
 *   `lobby_world_block + (i >> 3)` in a register; measured in docs/lobby.md).
 * NAMES. The 79 defined functions are named from their bodies.  GUESSes: the link-gated senders whose caller is
 *   unwritten, after the protocol sub-command they build (`lb_sub0a_send`, ...), and the icon tables (`<table>_area<n>`
 *   is entry n).  GUESS: `lobby_hunter_cards` (.bss 0x806BE340, ten 0x130-byte records keyed by a hunter id at +0x03
 *   with the hunter's 0x100-byte card at +0x24, which `updatePeerCardBlock` refreshes).  `hud_key_lookup` (0x8033A0BC)
 *   is the map's.  The dump's `homebutton::MotorCallback(OSAlarm...)` for
 *   0x8033C4D8/0x8033C570 is not adopted: both are 0x24-byte refresh wrappers without an argument.  The game-root
 *   dispatcher at 0x80432154 calls `lb_act_dispatch`/`lb_act_dispatch_ex` as `(u8 index, LbActReq* req)`.  Module
 *   `lobby`: `lobby_w`, `lb_param_w`, `lb_deli_data`, 11 `LbStr` calls.
 * RESIDUALS. 55 rows unwritten or scoring zero: 0x80339F10-0x8033A0BC (`lb_act_dispatch_ex`), 0x8033A160-0x8033A850,
 *   0x8033A868-0x8033A9F4, 0x8033AAFC-0x8033ADAC, 0x8033ADD0-0x8033AE58, 0x8033AED0-0x8033B298, 0x8033B2C8-0x8033B7D0
 *   (`fn_8033B67C` among the `sprintf` users that need the `.sdata` format operand), 0x8033B808-0x8033C36C,
 *   0x8033C4FC-0x8033C570, 0x8033C594-0x8033CEC0, 0x8033CF18-0x8033E050 (with the written `lb_byte_list_has`, which
 *   scores zero), 0x8033E088-0x8033F270.
 *  - `lb_page_entry_bit_set`: retail forms `lobby_world_block + (index >> 3)` in a register and keeps the bit-field
 *    offset as the load displacement (`add r5,r3,r0; lbz r4,14688(r5)`), ours folds it into the base
 *    (`addi r5,r3,14688`);
 *  - `lb_entry_flags_clear`: retail materialises both `.sbss` bases before the first store, ours folds the first into
 *    its `stb`'s sda21 operand (a `u8* seen = lbl_80794B90;` local does not change it);
 *  - `lb_act_limit_set`, `lb_companion_tick`, `lb_companion_mode_set`, `lb_entry_start_send`, `lb_act_entry_start`:
 *    retail addresses the companion work at +0x6A2A/+0x69A4/+0x6978/+0x6A40/+0x6A3C where ours uses
 *    +0xA2A/+0x9A4/+0x978/+0xA40/+0xA3C (an `LbCompanionWork` offset 0x6000 off); `lb_act_entry_start` also copies the
 *    request's +0x18..+0x1B bytes ours drops;
 *  - `lb_entry_id_get`, `lb_entry_publish`, `lb_page_flags_update`, `lb_entry_model_publish`: retail reads
 *    `lobby_world_block` +0x5180/+0x519C/+0x51A0/+0x51A4/+0x51A6 where ours is 0x80 higher (+0x5200 ...);
 *    `lb_entry_publish` reads a record's +0x1/+0x2/+0x3 where ours reads +0x0 three times, `lb_entry_model_publish`
 *    stores a byte where ours stores a word;
 *  - `lb_act_value_apply`, `lb_act_limit_set`, `lb_act_slot_write`: retail reads the request's +0x4 (+0x9) where ours
 *    reads +0x8 (+0x7);
 *  - `lb_act_row_apply`: retail loads +0x6 with `lhz`, ours `lbz` + `clrlwi` (a field width);
 *  - `lb_tbl3_get`: retail scales by 4 and loads a word, ours by 2 with `lhzx` (the table holds `u32`s);
 *  - `lb_entry_start_default`: retail zero-extends the byte, ours sign-extends it (the parameter is `u8`, not `s8`);
 *  - `lb_seen_bit_set`, `lb_act_award_handover`, `lb_companion_tick`, `lb_act_slot_write`: ours emits a `clrlwi` or
 *    `extsb` retail does not; `lb_quest_work_init`: retail narrows the kind before storing it;
 *  - `lb_page_id_find`: retail tests at the bottom of the loop with a 4-byte stride, ours at the top with 8;
 *  - `lb_page_row_ck`: ours passes two extra zero arguments; `lb_act_announce`: ours adds one `b`;
 *  - `lb_act_handover`: ours saves one register fewer and its branch targets shift;
 *  - `lb_name_tail_copy`: the word copies' load/store order;
 *  - `lb_seen_pad_ck`, `lb_act_row_update`, `lb_act_best_keep`, `lb_triplet_value2_get`, `lb_triplet_value1_get`,
 *    `lb_page_bits_set`: register colouring only.
 *   flipcheck: `.bss`/`.sbss`/`.sdata2` claimed, not emitted; `.data` 0x4F0 against 0x4C3C; `.sdata` 0x20C against
 *   0x354; `.text`/extab/extabindex short of the claim; the `.sdata`/`.sdata2` pool is shared with
 *   `hud/cockpit_quest.cpp` and `menu/menu_placeinfo.cpp` (a low-confidence fold candidate); the source still spells
 *   `my_player_no`, `enemy_data_find`, `eft_slot_persist_ck` and `eft_slot_spawn` by their old stems `fn_800CF384`,
 *   `fn_803438E4`, `fn_80343B44`, `fn_80343B74`.
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

extern "C" {

/* This unit's own forward declarations (the definitions follow in address order). */
void lb_seen_bit_set(u8 bit, u8 index);
void lb_handled_set(u8 unused, u8 index);
void lb_sub0d_send(u8 index, LbActReq* req, s8 flag);
void lb_sub0e_send(u8 index, u16 id, u8 flag, u16 value);
void lb_name_tail_copy(LbNameTail* dst, LbNameTail* src);
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd);
}

#pragma peephole off

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
    lbl_80794B90[(u8)index] = lbl_80794B90[(u8)index] | (u8)(1 << (u8)bit);
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
                    fn_802A0188();
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
                        fn_802E5D68(pair->id_0x00, (s8)req->sel_0x04.bytes_0x00.d_0x03);
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

/* Act 15: sends the 0x2C-byte sub-0x0F text packet built from the caller's block; with the link down, `fn_80142C58`. */
void lb_sub0f_send(u8 index, u8 value, void* text, u16 id, u8 flag, f32 scale) {
    LbCmdSub0F cmd;

    lb_sub0f_init(&cmd);
    if (isServerSelectState() == 0) {
        fn_80142C58(value, text, id, flag, scale);
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

/* Clears the sub-0x0F packet's text payload (`fn_80125F54`) and returns the packet. */
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd) {
    fn_80125F54(cmd->text_0x0C.bytes_0x00);
    return cmd;
}

/* The link-down half of act 15: hands the packet's own fields straight to the text writer `fn_80142C58`. */
void lb_text_apply(u8 unused, LbCmdSub0F* cmd) {
    void* text = cmd->text_0x0C.bytes_0x00;

    fn_80142C58(cmd->value_0x04, text, cmd->id_0x06, cmd->flag_0x05, cmd->scale_0x08);
}

/* Sends the sub-0x10 command with one signed byte, or hands it to `fn_80146C00` when the link is down. */
void lb_sub10_send(u8 index, s8 value) {
    LbCmdSub10 cmd;

    if (isServerSelectState() == 0) {
        fn_80146C00(value, index);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x10);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 16: hands the request's byte and the pad index to `fn_80146C00`. */
void lb_act_byte_apply(u8 unused, LbActReq* req) {
    fn_80146C00((s8)req->sel_0x04.bytes_0x00.a_0x00, req->index_0x01);
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
    fn_802B09B8(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
}

/* Sends the sub-0x12 command with one byte, or hands it to `fn_802B45F4` when the link is down. */
void lb_sub12_send(u8 value) {
    LbCmdSub12 cmd;

    if (isServerSelectState() == 0) {
        fn_802B45F4(value);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x12);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 18: hands the request's byte to `fn_802B45F4`. */
void lb_act_index_apply(u8 unused, LbActReq* req) {
    fn_802B45F4(req->sel_0x04.bytes_0x00.a_0x00);
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
void lb_companion_tick(void) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            companion->count_0x69A4 = companion->count_0x69A4 + 1;
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
void lb_sub16_send(s32 value, s8 flag) {
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
void lb_companion_mode_set(void) {
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
void lb_entry_start_send(LbCompanionWork* companion, s8 value, s32 arg, u8 flag) {
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
    fn_802BC000((u8)index, req->sel_0x04.bytes_0x00.a_0x00);
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
void lb_area_change_flag(void) {
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

/* Resets the tutorial/quest announcement block `lbl_806BEF20`'s four fields and stores the kind byte. */
void lb_quest_work_init(u8 kind) {
    lbl_806BEF20.active_0x00 = 0;
    lbl_806BEF20.flag_0x08 = 0;
    lbl_806BEF20.kind_0x06 = kind;
    lbl_806BEF20.ptr_0x0C = NULL;
}

/* Whether the announcement block is active (`lbl_806BEF20.active_0x00 != 0`). */
s32 lb_quest_work_active_ck(void) {
    return lbl_806BEF20.active_0x00 != 0;
}

/* The three-level table lookup `lbl_805E6A20[a][b][c]` the triplet getters share. */
s32 lb_tbl3_get(u8 a, u8 b, u8 c) {
    u32** level = (u32**)lbl_805E6A20[a];

    return (s32)level[b][c];
}

/* The third byte of the 12-byte `LbTriplet` row at `lbl_805E69E8[a][b]`. */
u8 lb_triplet_value2_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x02;
}

/* The first byte of that row. */
u8 lb_triplet_value0_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x00;
}

/* The second byte of that row. */
u8 lb_triplet_value1_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x01;
}

/* The tutorial quest's name pointer (`*tutorial_quest_name`). */
s32 lb_quest_name_get(void) {
    return *tutorial_quest_name;
}

/* The tutorial message pointer for one entry (`tutorial_quest_msg[index]`). */
s32 lb_quest_msg_get(u8 index) {
    return *(tutorial_quest_msg + index);
}

/* Scans the 4-word-stride `lbl_806042B8` id table for `id`; the 0xFFFF terminator answers NULL. */
u16* lb_page_id_find(u16 id) {
    u16* row;

    row = lbl_806042B8;
    do {
        if (*row == id) {
            return row;
        }
        row += 4;
    } while (*row != 0xFFFF);
    return NULL;
}

/* Forwards the two halfwords of the page table row `lbl_806043E8[index]` to the row writer `fn_8033AC78`. */
void lb_page_row_apply(u16 index, s32 value) {
    fn_8033AC78(lbl_806043E8[index].first_0x00, lbl_806043E8[index].second_0x02, value);
}

/* Act 26: whether the request's fourth byte has bit 0 (the "already handled" bit) clear. */
u32 lb_act_handled_bit_ck(u8 unused, LbActReq* req) {
    if (req != NULL && (req->sel_0x04.bytes_0x00.d_0x03 & 1) == 0) {
        return 1;
    }
    return 0;
}

/* `fn_8033AC78`'s row lookup: -1 when the row is not in the page table, else the row goes to `fn_8033AED0`. */
s8 lb_page_row_ck(u16 id, u16 value, s16* out) {
    if (fn_8033AC78(id, value, 0) == NULL) {
        return -1;
    }
    return fn_8033AED0(out, 0, 0);
}

/* Sets the bit for `index` in the page block's per-entry bit field `lobby_world_block->bits_0x3960`. */
void lb_page_entry_bit_set(u8 unused, LbActReq* req) {
    u8 index = req->sel_0x04.bytes_0x00.c_0x02;

    lobby_world_block->bits_0x3960[index >> 3] =
        lobby_world_block->bits_0x3960[index >> 3] | (u8)(1 << (index & 7));
}

/* Copies the 0xC-byte `LbSettings` record from one owner to another, field by field. */
void lb_settings_copy(LbSettings* dst, LbSettings* src) {
    dst->first_0x00 = src->first_0x00;
    dst->second_0x02 = src->second_0x02;
    dst->third_0x04 = src->third_0x04;
    dst->fourth_0x06 = src->fourth_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->value_0x0C = src->value_0x0C;
}

/* A tail call of `fn_80217934`, the area-name/quest path helper the tutorial block drives. */
void lb_area_name_apply(void) {
    fn_80217934();
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
    lb_param_w.sub_0x26 = entry->byte_0x01;
    lb_param_w.sub_0x27 = entry->byte_0x02;
    lb_param_w.sub_0x28 = entry->byte_0x03;
    lb_param_w.sub_0x29 = lobby_world_block->byte_0x51A4;
    lb_param_w.sub_0x2A = lobby_world_block->byte_0x51A5;
    lb_param_w.sub_0x2C = lobby_world_block->word_0x51A0;
    lb_param_w.sub_0x2E = lobby_world_block->word_0x51A2;
}

/* Recomputes the block's page flags: ORs `fn_802D8F84(fn_802D7B5C(word_0x51A0))` into `flags_0x519C`. */
void lb_page_flags_update(void) {
    lobby_world_block->flags_0x519C =
        lobby_world_block->flags_0x519C | fn_802D8F84(fn_802D7B5C(lobby_world_block->word_0x51A0));
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

/* Republishes the entry byte (`lb_entry_publish`) and then the page's name path (`fn_80217934`). */
void lb_entry_republish(void) {
    lb_entry_publish();
    fn_80217934();
}

/* Sets `page->bits_0x72` for the entry the block has selected and mirrors the count. */
void lb_page_bits_set(LbPageWork* page) {
    u8 i;
    u32 selected;

    selected = lobby_world_block->entry_0x3E03 & 0x7F;
    page->saved_0x76 = page->count_0x06;
    for (i = 0; i < page->count_0x06; i++) {
        if (selected == page->ids_0x08[i]) {
            page->bits_0x72 = (u16)(1 << i);
        }
    }
}

/* Whether the NUL-terminated byte list contains `value`. */
u32 lb_byte_list_has(u8* list, u8 value) {
    u8* p;

    for (p = list; ; p++) {
        if (*p == 0) {
            return 0;
        }
        if (value == *p) {
            return 1;
        }
    }
}

/* Draws the sub-page's arrow sprites at the layout position (`get_lsp_data(0x1EB6)`, `lbl_805E723C`). */
void lb_subpage_arrow_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x1EB6, &pos);
    draw_sprite_ary(lbl_805E723C, &pos);
}
}

