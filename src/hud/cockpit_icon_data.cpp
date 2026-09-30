/*
 * hud/cockpit_icon_data.cpp - the quest cockpit's map icon tables, a data-only unit (GUESS: the file name and the one
 * owner for the run; precedent `Network/network_shared_data.cpp`).
 *
 * Claims `.data` 0x805E6A58..0x805E70F8 and `.sdata` 0x80792D80..0x80792F8C.  The `.sdata` run is the icon id lists of
 * 8 bytes or less (MWCC's small-data cut) and their pointer arrays; the `.data` run holds the longer lists, the per-map
 * pointer tables `hud/cockpit_quest.cpp` indexes by map number, and the flash/slot sprite id lists
 * `menu/fn_802E4978.cpp` reads.  Both runs are reached by pointer only (no code loads a list through `sda21`), and the
 * tables are read with `lis`/`addi`, i.e. through extern arrays of unknown size (`hud/cockpit_icon_data.h`).
 *
 * Why one unit: a TU's `.data` is contiguous and this run is interleaved with other readers' lists, so the evidence
 * disagrees with "cockpit_quest.cpp owns it" (its readers are `hud/cockpit_quest.cpp`, `hud/fn_802EBED8.cpp`,
 * `ef/eft035.cpp` and `menu/fn_802E4978.cpp`, whose `.sdata2`/`.sdata` pools are shared); the owner question for the four
 * units (one TU spanning them or not) is open and not decided here.
 *
 * Emitted: every list and table up to 0x805E6EF8 (by map, by quest id, and the pit map's lists; names are GUESSes from the
 * tables' indices: `<table>_area<n>` is entry n of that table), the two slot sprite id lists and the flash id list.
 * Not emitted (owned, other readers' data): 0x805E6EF8..0x805E70C8, the lists `hud/fn_802EBED8.cpp` (0x805E6EF8..0x805E6FEC),
 * `ef/eft035.cpp` (0x805E6FEC..0x805E707C) and `menu/fn_802E4978.cpp` (0x805E707C..0x805E70C8) read - so the two slot lists
 * and the flash id list sit 0x1F0 bytes earlier in our object, and the `.data` section is that much shorter than the claim.
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
