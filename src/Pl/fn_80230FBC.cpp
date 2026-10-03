/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80230FBC` -> `zz_0230fbc_`, no runtime name, and
 * `grep -n "^fn_8023" config/RMHE08/symbols.txt` - the four entries are `fn_80230FBC`,
 * `fn_802310D4`, `fn_80233448`, `fn_80234E9C`; every non-mangled callee is a bare `fn_` placeholder)
 *
 * Pl/fn_80230FBC.cpp - the player work's motion-kind dispatch and four of its nine kind banks.
 *
 * `.text` 0x80230FBC-0x802373AC (25584 B): `fn_80230FBC`, the 9-way dispatcher on the kind byte
 * `_PLW`+0x002, then the kind banks for kinds 0-3, each a switch on `Get_motion_no(_PLW*)` over the
 * motion numbers 1002-1271 (270 values).  Registered sections: extab 0x80011C84-0x80011CA4,
 * extabindex 0x8002E9B0-0x8002E9E0, and the compiler-emitted jump tables in
 * `.data` 0x805C1F94-0x805C2C60 (9 + 270 + 270 + 270 entries).
 *
 * Module (evidence order): 1. no `__FILE__` string - the unit's own `.data` pool is nothing but the
 * four jump tables, so the string the discovery pass offered, `enemy_control.cpp` at 0x805A1BB8,
 * cannot be this unit's: the pool order follows the link order, and this unit sits (by its `.data`,
 * 0x805C1F94, between `Pl/fn_80229ECC.cpp`'s 0x805C1F94 end and `Pl/fn_80241558.cpp`'s 0x805C3D20
 * start) after the enemy bands.  2. no runtime-dump name (`zz_0230fbc_`).  3. class 3: the first
 * argument is passed straight to `Get_motion_no` (`Get_motion_no__FP4_PLW`), `_PLW`+0xAF4/+0xAF8/
 * +0xAFC are the player's three `_se_w` works, and the banks call the `Pl` SE/effect helpers, so the
 * module is `Pl`; the file keeps the map's stem (class 4).
 *
 * Language: C++ - the callees are manglings (`Get_motion_no__FP4_PLW`,
 * `se_req_frame_set__FP5_se_wllll`, `SE_Code_Make__Flsls`, `PlayMode_ck__Fv`) and every object in
 * this range carries an extab/extabindex pair, i.e. the Pl lib's `-Cpp_exceptions on`.
 *
 * Flags: none beyond the lib's `cflags_pl` (`Wii/1.0`, `-O3 -inline noauto -opt nopeephole
 * -Cpp_exceptions on`), the same set the sibling Pl switch units measure.
 *
 * Residual: **none**.  Every section of the reconstructed object is byte-identical to the target
 * (`cmp` per section: `.text` 25584 B, `.data` 3276 B, extab 32 B, extabindex 48 B), and objdiff
 * reports 100.00000 for each of the four functions.  The only difference left is a *symbol name*:
 * MWCC emits the four jump tables as anonymous locals (`@211`/`@529`/`@859`/`@1173`) where dtk's
 * analyzer named them `jumptable_805C1F94`/`_805C1FB8`/`_805C23F0`/`_805C2828` in the target; the
 * offsets and every loader reloc against them are the same.
 *
 * Three source shapes are load-bearing, none of them obvious from the code:
 *   1. the three `_se_w` work pointers are *declared* `Part, Main, Frame` but *assigned*
 *      `Part, Frame, Main` - that is the only shape that colours them r31/r29/r30 as retail does;
 *   2. a case body whose target code runs on into the next body is a deliberate source fallthrough
 *      (no `break`): case 1005's body (`0x1a0`) falls into case 1007's (`0x1cc`);
 *   3. the four bodies that arm both `partHi` and a `partHi | 2` value give the `| 2` value its own
 *      variable (`partHi2`) - with the expression written twice MWCC colours the pair the other way
 *      round (r28/r27 against retail's r27/r28, 99.94491 on the function).
 *
 * The case bodies are written in retail's *emission* order, which is not numeric order: the run goes
 * 1002..1111, then 1128, 1131..1135, then 1116..1127 (the table points 1116-1127 past 1135), then
 * 1136 onwards.  The empty case values are one label group, which is how those table entries land on
 * the shared epilogue.
 *
 * `tools/units/flipcheck.py Pl/fn_80230FBC` says READY (4 sections match the claim); the unit is still
 * registered `NonMatching`, and `ninja build/RMHE08/ok` was green with it in that state.  The flip is
 * red for the link's `.data` padding, not the object: MWCC gives this file's `.data` section header an
 * `sh_addralign` of 8 while the claim starts at the 4-mod-8 0x805C1F94, so mwld pads 4 bytes, the four
 * tables land at 0x805C1F98/…+8 and `main.dol` hashes 8D7E9DFC...; setting that *one* field to 4 in a
 * scratch copy of the object (the `.comment` entry left at 8 - the linker does not use it) links to
 * sha1 BF485073... with 0 differing bytes.  Details in `src/Pl/fn_8023C2D0.cpp`'s header.
 * Resolved the same way - `tools/elf/objalign.py` (e242dfecf) plus the flip (6d0cf5705), green; see
 * that header and docs/matching.md section 55.
 */

#include "types.h"
#include "ef/fn_800CDB2C.h"
#include "Pl/fn_80241558.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "sound/fn_800D7F54.h"
#include "sound/se.h"
#include "unsplit/Pl.h"

/* The four kind banks of this unit, defined below in `.text` order. */
extern "C" void fn_802310D4(_PLW* work, u8 part);
extern "C" void fn_80233448(_PLW* work, u8 part);
extern "C" void fn_80234E9C(_PLW* work, u8 part);

/* Dispatches on the player's motion-kind byte to the nine per-kind motion banks.  Returns without
 * doing anything during an event demo, outside play mode 3 (unless the master action is running) or
 * for a kind past the last bank. */
extern "C" void fn_80230FBC(_PLW* self)
{
    u8 part;

    if (event_demo_ck() == 1) {
        return;
    }
    if ((u8)PlayMode_ck() == 3) {
        if (fn_8026FD94(self) == 0) {
            return;
        }
    }
    part = self->field_0x5A7;
    Get_motion_no(self);
    switch (self->field_0x002) {
    case 0:
        fn_802310D4(self, part);
        break;
    case 1:
        fn_80233448(self, part);
        break;
    case 2:
        fn_80234E9C(self, part);
        break;
    case 3:
        fn_802373AC(self, part);
        break;
    case 7:
        fn_802399C8(self, part);
        break;
    case 4:
        fn_8023FC20(self, part);
        break;
    case 5:
        fn_80241558(self, part);
        break;
    case 6:
        fn_802430E8(self, part);
        break;
    case 8:
        fn_8023C2D0(self, part);
        break;
    }
}
extern "C" void fn_802310D4(_PLW* work, u8 part)
{
    u32 partHi;
    u32 partHi2;
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 1006: case 1008: case 1010: case 1011: case 1014: case 1015: case 1016: case 1017: case 1018: case 1019: case 1020: case 1021: case 1022: case 1023: case 1024: case 1025: case 1026: case 1027: case 1028: case 1029: case 1030: case 1031: case 1032: case 1033: case 1034: case 1035: case 1036: case 1037: case 1038: case 1039: case 1040: case 1041: case 1042: case 1043: case 1044: case 1045: case 1046: case 1047: case 1048: case 1049: case 1050: case 1055: case 1058: case 1059: case 1063: case 1070: case 1071: case 1072: case 1073: case 1074: case 1075: case 1076: case 1077: case 1078: case 1079: case 1080: case 1081: case 1082: case 1083: case 1084: case 1085: case 1086: case 1087: case 1088: case 1089: case 1090: case 1091: case 1092: case 1093: case 1094: case 1095: case 1096: case 1097: case 1098: case 1099: case 1100: case 1102: case 1103: case 1105: case 1108: case 1109: case 1110: case 1112: case 1113: case 1114: case 1115: case 1129: case 1130: case 1140: case 1141: case 1142: case 1143: case 1144: case 1145: case 1146: case 1147: case 1148: case 1149: case 1155: case 1156: case 1157: case 1158: case 1159: case 1175: case 1176: case 1177: case 1178: case 1179: case 1180: case 1181: case 1182: case 1183: case 1184: case 1185: case 1186: case 1187: case 1188: case 1189: case 1190: case 1191: case 1192: case 1193: case 1194: case 1195: case 1196: case 1197: case 1198: case 1199: case 1200: case 1202: case 1203: case 1204: case 1205: case 1206: case 1207: case 1208: case 1209: case 1210: case 1211: case 1214: case 1215: case 1216: case 1217: case 1218: case 1219: case 1220: case 1222: case 1223: case 1224: case 1225: case 1226: case 1227: case 1228: case 1229: case 1230: case 1231: case 1232: case 1233: case 1234: case 1235: case 1236: case 1237: case 1238: case 1239: case 1240: case 1241: case 1242: case 1243: case 1244: case 1245: case 1246: case 1247: case 1248: case 1249: case 1250: case 1252: case 1253: case 1254: case 1255: case 1256: case 1257: case 1258: case 1259: case 1260: case 1261: case 1264: case 1265: case 1266: case 1267: case 1268: case 1269: case 1270:
        break;
    case 1002:
        se_req_frame_set(seWorkMain, 0x24, 5, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 6, 6, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x12, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0x24, 3);
        break;
    case 1003:
        se_req_frame_set(seWorkMain, 0xA, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x30, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x32, 3);
        break;
    case 1004:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3E, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        fn_80229E10(seWorkPart, 0x40, 3);
        break;
    case 1005:
        se_req_frame_set(seWorkMain, 0x12, 0xB, 0xB, 3);
        fn_80229EA8(seWorkPart, 0x16, 7, 3);
        /* falls through */
    case 1007:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1009:
        se_req_frame_set(seWorkMain, 8, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x26, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3C, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        fn_80229E10(seWorkPart, 0x3E, 3);
        break;
    case 1012:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(1, 2, 0, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 6, 0x28, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x14, 0xB, 0x11, 0x3000003);
        se_req_frame_set(seWorkMain, 0x3A, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x28, 3);
        fn_80229E10(seWorkPart, 0x44, 3);
        break;
    case 1013:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(1, 2, 0, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 6, 0x28, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x14, 0xB, 0x11, 0x3000003);
        se_req_frame_set(seWorkMain, 0x3A, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x28, 3);
        fn_80229E10(seWorkPart, 0x44, 3);
        break;
    case 1051:
        se_req_frame_set(seWorkMain, 0x10, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x1A, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1A, 3, 3, 4);
        break;
    case 1052:
        se_req_frame_set(seWorkMain, 0x14, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x16, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x30, 0xEE, 0x14, 0x3000003);
        break;
    case 1053:
        se_req_frame_set(seWorkPart, 8, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x2A, 0xEE, 0x11, 0x3000003);
        break;
    case 1054:
        se_req_frame_set(seWorkMain, 4, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 8, 0xC, 0xB, 3);
        se_req_frame_set(seWorkMain, 6, 6, 0xB, 4);
        break;
    case 1056:
        se_req_frame_set(seWorkMain, 4, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 8, 0xEE, 0x14, 0x3000003);
        break;
    case 1057:
        se_req_frame_set(seWorkMain, 4, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x18, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x2C, 0xEE, 0x11, 0x3000003);
        break;
    case 1060:
        se_req_frame_set(seWorkPart, 8, 0xF5, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x36, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xEE, 0x11, 0x3000003);
        break;
    case 1061:
        se_req_frame_set(seWorkPart, 8, 0xF5, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x36, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xEE, 0x14, 0x3000003);
        break;
    case 1062:
        se_req_frame_set(seWorkPart, 8, 0xF5, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x54, 0xEE, 0x11, 0x3000003);
        break;
    case 1064:
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x3A, 0xED, 0x14, 0x3000003);
        break;
    case 1065:
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x3000003);
        break;
    case 1066:
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x14, 0x3000003);
        break;
    case 1067:
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x3000003);
        break;
    case 1068:
        se_req_frame_set(seWorkPart, 6, 0xF5, 3, 0x3000003);
        break;
    case 1069:
        se_req_frame_set(seWorkPart, 6, 0xF5, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x3C, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x50, 0xEE, 0x11, 0x3000003);
        break;
    case 1101:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x14, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x10, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x12, 3);
        fn_80229EA8(seWorkPart, 0x22, 7, 3);
        break;
    case 1104:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 5, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x10, 6, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        break;
    case 1106:
        se_req_frame_set(seWorkPart, 0x26, SE_Code_Make(2, 1, 1, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x12, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x82, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        partHi2 = partHi | 2;
        fn_800DA428(seWorkFrame, 0x10, partHi2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x28, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x4C, partHi2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x9E, partHi | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0xB8, partHi | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        fn_80229E10(seWorkPart, 0x4C, 3);
        fn_80229EA8(seWorkPart, 0x48, 7, 3);
        fn_80229EA8(seWorkPart, 0x80, 7, 3);
        break;
    case 1107:
        se_req_frame_set(seWorkPart, 0x4C, SE_Code_Make(0, 3, 0, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xE4, 3, 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x32, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x72, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0xDC, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE0, 0, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x106, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        partHi2 = partHi | 2;
        fn_800DA428(seWorkFrame, 0x1A, partHi2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x6E, partHi2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x108, partHi | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x118, partHi | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x1C, 3);
        fn_80229E10(seWorkPart, 0x70, 3);
        fn_80229EA8(seWorkPart, 0xAA, 7, 3);
        fn_80229EA8(seWorkPart, 0x112, 7, 3);
        break;
    case 1111:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(0, 3, 1, 1), 0xC, 3);
        se_req_frame_set(seWorkPart, 8, 0x11, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0xA, 0x27, 0x11, 0x3000003);
        se_req_frame_set(seWorkMain, 0x30, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x30, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        break;
    case 1128:
        se_req_frame_set(seWorkPart, 0x1C, SE_Code_Make(2, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x10, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x2A, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0x88, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x28, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x76, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x94, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x2A, 3);
        fn_80229EA8(seWorkPart, 0x9C, 7, 3);
        break;
    case 1131:
        se_req_frame_set(seWorkMain, 4, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xE, (part << 0x18) | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        break;
    case 1132:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(3, 3, 3, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x10, 5, 3, 4);
        se_req_frame_set(seWorkMain, 0x2A, 4, 3, 4);
        se_req_frame_set(seWorkMain, 0x60, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x6E, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x70, 3);
        fn_80229EA8(seWorkPart, 0x36, 7, 3);
        break;
    case 1133:
        se_req_frame_set(seWorkMain, 4, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x14, 0, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xA0, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        fn_80229EA8(seWorkPart, 0x3E, 7, 3);
        break;
    case 1134:
        se_req_frame_set(seWorkMain, 4, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 5, 3, 4);
        fn_800DA428(seWorkFrame, 8, (part << 0x18), 0x14, 2);
        break;
    case 1135:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(2, 3, 2, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x16, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x30, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0xA0, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xB0, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x52, 3);
        fn_80229EA8(seWorkPart, 0xBC, 7, 3);
        break;
    case 1116:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x30, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x2C, (part << 0x18) | 2, 0x11, 2);
        fn_80229E10(seWorkPart, 0x2E, 3);
        break;
    case 1117:
        se_req_frame_set(seWorkMain, 0x1E, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1A, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x1C, 3);
        break;
    case 1118:
        se_req_frame_set(seWorkMain, 4, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x10, (part << 0x18), 0x11, 2);
        break;
    case 1119:
        se_req_frame_set(seWorkMain, 0x14, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x1C, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x10, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18), 0x11, 2);
        break;
    case 1120:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 8, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x28, partHi | 2, 0x11, 2);
        fn_80229E10(seWorkPart, 0x2A, 3);
        break;
    case 1121:
        se_req_frame_set(seWorkMain, 0x10, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1A, (part << 0x18), 0x11, 2);
        break;
    case 1122:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x30, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x2C, (part << 0x18) | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 0x2E, 3);
        break;
    case 1123:
        se_req_frame_set(seWorkMain, 0x1E, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x16, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        break;
    case 1124:
        se_req_frame_set(seWorkMain, 4, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 6, (part << 0x18) | 1, 0x11, 2);
        break;
    case 1125:
        se_req_frame_set(seWorkMain, 0x1C, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18), 0x11, 2);
        break;
    case 1126:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 8, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x28, partHi | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 0xA, 3);
        break;
    case 1127:
        se_req_frame_set(seWorkMain, 0x10, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18), 0x11, 2);
        break;
    case 1136:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x30, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x2C, (part << 0x18) | 2, 0x11, 2);
        fn_80229E10(seWorkPart, 0x2E, 3);
        break;
    case 1137:
        se_req_frame_set(seWorkMain, 0x1E, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1A, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x11A, 3);
        break;
    case 1138:
        se_req_frame_set(seWorkMain, 8, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x30, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x2C, (part << 0x18) | 2, 0x11, 2);
        fn_80229E10(seWorkPart, 0x2E, 3);
        break;
    case 1139:
        se_req_frame_set(seWorkMain, 0x1E, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x18, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x1A, 3);
        break;
    case 1150:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1151:
        se_req_frame_set(seWorkPart, 0x32, SE_Code_Make(0x14, 1, 0x15, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x2A, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x2A, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0xA4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x94, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0xB4, 0xEE, 0x14, 0x3000003);
        break;
    case 1152:
        se_req_frame_set(seWorkPart, 0x48, SE_Code_Make(0x14, 3, 0x14, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0xD2, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0xBC, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0xB4, 0xEE, 0x11, 0x3000003);
        break;
    case 1153:
        se_req_frame_set(seWorkMain, 0x1E, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1E, 3, 3, 4);
        se_req_frame_set(seWorkPart, 8, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x16, 0xEE, 0x11, 0x3000003);
        break;
    case 1154:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x15, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x14, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x14, 3, 3, 4);
        break;
    case 1160:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1161:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1162:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1163:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1164:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x16, 0xEE, 0x11, 0x3000003);
        break;
    case 1165:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x16, 0xEE, 0x11, 0x3000003);
        break;
    case 1166:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(0x16, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 4, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0x5A, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1167:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1168:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(0x17, 3, 0x17, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 0x10, 0xF5, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x30, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x4A, 0xEE, 0x11, 0x3000003);
        break;
    case 1169:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x17, 3, 0x17, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 0xF, 3, 4);
        se_req_frame_set(seWorkMain, 0x22, 0xE, 3, 4);
        se_req_frame_set(seWorkMain, 0xE, 3, 0xB, 4);
        se_req_frame_set(seWorkMain, 0x60, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x66, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x82, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkMain, 0x26, 3, 0xB, 4);
        break;
    case 1170:
        se_req_frame_set(seWorkMain, 4, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x14, 0xA, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x16, 6, 0xB, 4);
        break;
    case 1171:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 3, 0xB, 4);
        se_req_frame_set(seWorkMain, 6, 0xF, 3, 4);
        break;
    case 1172:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(0x16, 3, 0x16, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x12, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x12, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0xA0, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1173:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x1A, 6, 0xB, 4);
        break;
    case 1174:
        se_req_frame_set(seWorkMain, 4, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1201:
        se_req_frame_set(seWorkPart, 8, SE_Code_Make(4, 1, 5, 1), 0xC, 3);
        break;
    case 1212:
        partHi = part << 0x18;
        partHi2 = partHi | 2;
        fn_800DA428(seWorkFrame, 0x1C, partHi2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x22, partHi2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x34, partHi, 0x14, 2);
        se_req_frame_set(seWorkMain, 0x32, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x34, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x4A, 8, 0xE, 0x3000003);
        fn_80229E10(seWorkPart, 0x24, 3);
        fn_80229EA8(seWorkPart, 0x54, 7, 3);
        break;
    case 1213:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x14, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x1C, 8, 0x11, 0x3000003);
        fn_80229E10(seWorkPart, 0x20, 3);
        break;
    case 1221:
        se_req_frame_set(seWorkMain, 0x38, 0xC, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        partHi2 = partHi | 2;
        fn_800DA428(seWorkFrame, 0xC, partHi2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1E, partHi2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x62, partHi | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x20, 3);
        break;
    case 1251:
        se_req_frame_set(seWorkPart, 8, SE_Code_Make(0x18, 1, 0x19, 1), 0xC, 3);
        break;
    case 1262:
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x32, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x5C, 0xEE, 0x11, 0x3000003);
        break;
    case 1263:
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x24, 0xEE, 0x14, 0x3000003);
        break;
    case 1271:
        se_req_frame_set(seWorkPart, 8, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x2E, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x44, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x5E, 0xED, 0x11, 0x3000003);
        break;
    }
}

extern "C" void fn_80233448(_PLW* work, u8 part)
{
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 1006: case 1008: case 1010: case 1011: case 1014: case 1015: case 1016: case 1017: case 1019: case 1020: case 1021: case 1022: case 1023: case 1024: case 1025: case 1026: case 1027: case 1028: case 1029: case 1030: case 1031: case 1032: case 1033: case 1034: case 1035: case 1036: case 1037: case 1038: case 1039: case 1040: case 1041: case 1042: case 1043: case 1044: case 1045: case 1046: case 1047: case 1048: case 1049: case 1050: case 1055: case 1058: case 1059: case 1063: case 1071: case 1072: case 1073: case 1074: case 1075: case 1076: case 1077: case 1078: case 1079: case 1080: case 1081: case 1082: case 1083: case 1084: case 1085: case 1086: case 1087: case 1088: case 1089: case 1090: case 1091: case 1092: case 1093: case 1094: case 1095: case 1096: case 1097: case 1098: case 1099: case 1100: case 1107: case 1108: case 1109: case 1110: case 1111: case 1112: case 1113: case 1114: case 1115: case 1122: case 1123: case 1125: case 1126: case 1127: case 1128: case 1129: case 1131: case 1132: case 1133: case 1134: case 1135: case 1136: case 1137: case 1138: case 1139: case 1140: case 1141: case 1142: case 1143: case 1144: case 1145: case 1146: case 1147: case 1148: case 1149: case 1158: case 1159: case 1162: case 1163: case 1164: case 1165: case 1166: case 1167: case 1168: case 1169: case 1170: case 1171: case 1172: case 1173: case 1174: case 1175: case 1176: case 1177: case 1178: case 1179: case 1180: case 1181: case 1182: case 1183: case 1184: case 1185: case 1186: case 1187: case 1188: case 1189: case 1190: case 1191: case 1192: case 1193: case 1194: case 1195: case 1196: case 1197: case 1198: case 1199: case 1200: case 1202: case 1203: case 1204: case 1205: case 1206: case 1207: case 1208: case 1209: case 1210: case 1214: case 1215: case 1216: case 1217: case 1218: case 1219: case 1220: case 1222: case 1223: case 1224: case 1225: case 1226: case 1227: case 1228: case 1229: case 1230: case 1231: case 1232: case 1233: case 1234: case 1235: case 1236: case 1237: case 1238: case 1239: case 1240: case 1241: case 1242: case 1243: case 1244: case 1245: case 1246: case 1247: case 1248: case 1249: case 1250: case 1252: case 1253: case 1254: case 1255: case 1256: case 1257: case 1258: case 1259: case 1260: case 1264: case 1265: case 1266: case 1267: case 1268: case 1269: case 1270:
        break;
    case 1002:
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18), 0x14, 2);
        fn_80229EA8(seWorkPart, 0x10, 7, 3);
        break;
    case 1003:
        se_req_frame_set(seWorkMain, 0x12, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x1E, (part << 0x18), 0x11, 2);
        fn_800DA428(seWorkFrame, 0x28, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        break;
    case 1004:
        se_req_frame_set(seWorkPart, 6, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        fn_800DA428(seWorkFrame, 0x12, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x28, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x2A, 3);
        break;
    case 1005: case 1007:
        se_req_frame_set(seWorkPart, 4, 0x1C, 0xE, 0x3000003);
        fn_800DA428(seWorkFrame, 0xA, (part << 0x18) | 1, 0x11, 2);
        break;
    case 1009:
        se_req_frame_set(seWorkMain, 4, 0, 3, 4);
        se_req_frame_set(seWorkMain, 0x12, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x14, (part << 0x18) | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2C, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        fn_80229E10(seWorkPart, 0x30, 3);
        break;
    case 1012:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 1, 1, 1), 0xC, 3);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 3, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18), 0x11, 2);
        fn_80229E10(seWorkPart, 0x1E, 3);
        fn_80229E10(seWorkPart, 0x2E, 3);
        se_req_frame_set(seWorkPart, 6, 0x28, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x14, 0xB, 0xE, 0x3000003);
        break;
    case 1013:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 1, 1, 1), 0xC, 3);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x1E, 3);
        fn_80229E10(seWorkPart, 0x2E, 3);
        se_req_frame_set(seWorkPart, 6, 0x28, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x14, 0xB, 0xE, 0x3000003);
        break;
    case 1018:
        se_req_frame_set(seWorkMain, 4, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0xE, 3);
        break;
    case 1101:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(0, 2, 0, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x38, (part << 0x18), 0x11, 2);
        fn_80229EA8(seWorkPart, 0xC, 7, 3);
        break;
    case 1102:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(1, 1, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0, 3, 4);
        se_req_frame_set(seWorkMain, 6, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, (part << 0x18) | 1, 0x11, 2);
        fn_80229E10(seWorkPart, 0x36, 3);
        break;
    case 1103:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(2, 4, 2, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0, 0xB, 4);
        se_req_frame_set(seWorkMain, 0xE, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x28, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 8, (part << 0x18), 0x11, 2);
        fn_800DA428(seWorkFrame, 0x14, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x34, (part << 0x18) | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x6C, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x36, 3);
        fn_80229EA8(seWorkPart, 0x3C, 7, 3);
        break;
    case 1104:
        se_req_frame_set(seWorkPart, 0x1A, SE_Code_Make(0, 4, 0, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x18, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_80229E10(seWorkPart, 0x1C, 3);
        fn_800DA428(seWorkFrame, 6, (part << 0x18) | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0xA, (part << 0x18) | 3, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18), 0x11, 2);
        fn_800DA428(seWorkFrame, 0x22, (part << 0x18) | 3, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x54, (part << 0x18) | 1, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x24, 0xD, 7, 0x3000002);
        fn_80229EA8(seWorkPart, 0x3C, 7, 3);
        break;
    case 1105:
        se_req_frame_set(seWorkPart, 0x1C, SE_Code_Make(2, 4, 2, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xE, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 8, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1A, (part << 0x18) | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x52, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x1C, 3);
        fn_80229EA8(seWorkPart, 0x34, 7, 3);
        se_req_frame_set(seWorkPart, 0x1C, 8, 0x11, 0x3000002);
        break;
    case 1106:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(1, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xA, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x12, (part << 0x18) | 3, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x42, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229EA8(seWorkPart, 0x18, 7, 3);
        break;
    case 1116:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(0, 2, 0, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xE, 0, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0xA, (part << 0x18) | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0xA, 3);
        break;
    case 1117:
        se_req_frame_set(seWorkMain, 0xE, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        break;
    case 1118:
        se_req_frame_set(seWorkMain, 0xA, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x40, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0xE, 3);
        fn_80229EA8(seWorkPart, 0x1E, 0xB, 3);
        break;
    case 1119:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(3, 4, 3, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xA, 1, 0xB, 3);
        se_req_frame_set(seWorkMain, 0xA, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x42, (part << 0x18), 0x14, 2);
        fn_80229EA8(seWorkPart, 0x22, 0xB, 3);
        se_req_frame_set(seWorkPart, 8, 8, 0x11, 0x3000002);
        break;
    case 1120:
        se_req_frame_set(seWorkPart, 0x12, SE_Code_Make(0, 2, 0, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0, 7, 3);
        se_req_frame_set(seWorkMain, 0xC, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 8, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3A, (part << 0x18), 0x14, 2);
        fn_80229EA8(seWorkPart, 0x22, 7, 3);
        break;
    case 1121:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(2, 4, 2, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 6, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x36, (part << 0x18), 0x14, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        break;
    case 1124:
        se_req_frame_set(seWorkPart, 0xC, SE_Code_Make(1, 2, 1, 2), 0xC, 3);
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 3, 0x14, 2);
        se_req_frame_set(seWorkMain, 0xA, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_80229E10(seWorkPart, 6, 3);
        break;
    case 1130:
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 6, (part << 0x18) | 3, 0x14, 2);
        break;
    case 1051:
        se_req_frame_set(seWorkMain, 4, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 6, 0xED, 0x11, 0x3000003);
        break;
    case 1052:
        se_req_frame_set(seWorkMain, 0xC, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x14, 0xED, 0x11, 0x3000003);
        break;
    case 1053:
        se_req_frame_set(seWorkPart, 0xC, 0xEE, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x28, 0xEE, 0x11, 0x3000003);
        break;
    case 1054: case 1056:
        se_req_frame_set(seWorkPart, 4, 0xEE, 7, 0x3000003);
        se_req_frame_set(seWorkPart, 0xA, 0xEE, 0x11, 0x3000003);
        break;
    case 1057:
        se_req_frame_set(seWorkMain, 0x16, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x12, 0xEE, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x28, 0xED, 0x14, 0x3000003);
        break;
    case 1060:
        se_req_frame_set(seWorkPart, 6, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x11, 0x3000003);
        break;
    case 1061:
        se_req_frame_set(seWorkPart, 6, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x11, 0x3000003);
        break;
    case 1062:
        se_req_frame_set(seWorkPart, 6, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x11, 0x3000003);
        break;
    case 1064:
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x36, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x4E, 0xED, 0x14, 0x3000003);
        break;
    case 1065:
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x14, 0x3000003);
        break;
    case 1066:
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x36, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x4E, 0xED, 0x14, 0x3000003);
        break;
    case 1067:
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x14, 0x3000003);
        break;
    case 1068:
        se_req_frame_set(seWorkPart, 4, 0xED, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x10, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        break;
    case 1069:
        se_req_frame_set(seWorkPart, 4, 0xED, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x10, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 6, 0xEA, 0xB, 0x3000003);
        break;
    case 1070:
        se_req_frame_set(seWorkMain, 4, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEE, 0x11, 0x3000003);
        break;
    case 1150:
        se_req_frame_set(seWorkPart, 0xA, SE_Code_Make(0x14, 3, 0x15, 1), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xE, 2, 0xB, 4);
        break;
    case 1151:
        se_req_frame_set(seWorkPart, 0xA, SE_Code_Make(0x15, 3, 0x16, 1), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xF, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 8, 2, 0xB, 4);
        break;
    case 1152:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x16, 2, 0x16, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkMain, 0x14, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x16, 3, 0xB, 4);
        break;
    case 1153:
        se_req_frame_set(seWorkPart, 0x20, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0x10, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkMain, 0x20, 0xA, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 2, 0xB, 4);
        se_req_frame_set(seWorkPart, 0x52, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x66, 0xED, 0x14, 0x3000003);
        break;
    case 1154:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0x16, 3, 0x16, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkMain, 0x16, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1C, 2, 0xB, 4);
        se_req_frame_set(seWorkPart, 0x58, 0xED, 0x11, 0x3000003);
        break;
    case 1155:
        se_req_frame_set(seWorkPart, 0xA, SE_Code_Make(0x17, 4, 0x17, 4), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkMain, 0xC, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 2, 0xB, 3);
        break;
    case 1156: case 1157:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkMain, 0x16, 0xA, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1A, 2, 0xB, 4);
        break;
    case 1160:
        se_req_frame_set(seWorkMain, 4, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 6, 2, 0xB, 4);
        break;
    case 1161:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(0x16, 2, 0x16, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkMain, 0x16, 0xB, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1C, 2, 0xB, 4);
        se_req_frame_set(seWorkPart, 0x52, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x66, 0xED, 0x14, 0x3000003);
        break;
    case 1201:
        se_req_frame_set(seWorkPart, 8, SE_Code_Make(4, 1, 5, 1), 0xC, 3);
        break;
    case 1211:
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1E, (part << 0x18) | 4, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        break;
    case 1212:
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18) | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x28, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x7A, (part << 0x18), 0x11, 2);
        se_req_frame_set(seWorkPart, 0x2A, 8, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x30, 0xA, 0xB, 0x3000003);
        fn_80229E10(seWorkPart, 0x28, 3);
        break;
    case 1213:
        fn_800DA428(seWorkFrame, 0x18, (part << 0x18) | 3, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x58, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x2A, 3);
        se_req_frame_set(seWorkPart, 0x26, 8, 0x11, 0x3000003);
        break;
    case 1221:
        fn_800DA428(seWorkFrame, 0x16, (part << 0x18), 0x14, 2);
        fn_800DA428(seWorkFrame, 0x24, (part << 0x18) | 1, 0x11, 2);
        fn_80229E10(seWorkPart, 0x24, 3);
        break;
    case 1251:
        se_req_frame_set(seWorkPart, 8, SE_Code_Make(0x18, 1, 0x19, 1), 0xC, 3);
        break;
    case 1261:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0xC, 0xED, 0x14, 0x3000003);
        break;
    case 1262:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x34, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x11, 0x3000003);
        break;
    case 1263:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x18, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x3000003);
        break;
    case 1271:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x3000003);
        se_req_frame_set(seWorkPart, 0x1A, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x28, 0xED, 0x11, 0x3000003);
        break;
    }
}

extern "C" void fn_80234E9C(_PLW* work, u8 part)
{
    u32 partHi;
    u32 partHi2;
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 1005: case 1006: case 1007: case 1008: case 1010: case 1014: case 1015: case 1016: case 1017: case 1018: case 1019: case 1020: case 1021: case 1022: case 1023: case 1024: case 1025: case 1026: case 1027: case 1028: case 1029: case 1030: case 1031: case 1032: case 1033: case 1034: case 1035: case 1036: case 1037: case 1038: case 1039: case 1040: case 1041: case 1042: case 1043: case 1044: case 1045: case 1046: case 1047: case 1048: case 1049: case 1050: case 1054: case 1055: case 1056: case 1058: case 1059: case 1063: case 1070: case 1071: case 1072: case 1073: case 1074: case 1075: case 1076: case 1077: case 1078: case 1079: case 1080: case 1081: case 1082: case 1083: case 1084: case 1085: case 1086: case 1087: case 1088: case 1089: case 1090: case 1091: case 1092: case 1093: case 1094: case 1095: case 1096: case 1097: case 1098: case 1099: case 1100: case 1107: case 1108: case 1114: case 1115: case 1116: case 1117: case 1118: case 1119: case 1120: case 1121: case 1122: case 1123: case 1124: case 1125: case 1126: case 1127: case 1128: case 1129: case 1133: case 1137: case 1138: case 1139: case 1145: case 1146: case 1147: case 1148: case 1149: case 1150: case 1155: case 1165: case 1166: case 1167: case 1168: case 1169: case 1180: case 1181: case 1182: case 1183: case 1184: case 1185: case 1186: case 1187: case 1188: case 1189: case 1190: case 1191: case 1192: case 1193: case 1194: case 1195: case 1196: case 1197: case 1198: case 1199: case 1200: case 1202: case 1203: case 1204: case 1205: case 1206: case 1207: case 1208: case 1209: case 1210: case 1211: case 1212: case 1213: case 1214: case 1215: case 1216: case 1217: case 1218: case 1219: case 1220: case 1222: case 1223: case 1224: case 1225: case 1226: case 1227: case 1228: case 1229: case 1230: case 1231: case 1232: case 1233: case 1234: case 1235: case 1236: case 1237: case 1238: case 1239: case 1240: case 1241: case 1242: case 1243: case 1244: case 1245: case 1246: case 1247: case 1248: case 1249: case 1250: case 1252: case 1253: case 1254: case 1255: case 1256: case 1257: case 1258: case 1259: case 1260: case 1261: case 1262: case 1263: case 1264: case 1265: case 1266: case 1267: case 1268: case 1269: case 1270:
        break;
    case 1002:
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3A, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x3C, (part << 0x18) | 1, 0x14, 2);
        fn_80229EA8(seWorkPart, 0x22, 7, 3);
        fn_80229E10(seWorkPart, 0x3E, 3);
        break;
    case 1003:
        se_req_frame_set(seWorkMain, 2, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x44, (part << 0x18), 0x11, 2);
        fn_80229EA8(seWorkPart, 0x2E, 7, 3);
        fn_80229E10(seWorkPart, 0x46, 3);
        break;
    case 1004:
        se_req_frame_set(seWorkPart, 6, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x12, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        break;
    case 1009:
        se_req_frame_set(seWorkMain, 4, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x1E, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3C, (part << 0x18) | 1, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x4C, (part << 0x18) | 1, 0x11, 2);
        fn_80229EA8(seWorkPart, 0x18, 7, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        fn_80229E10(seWorkPart, 0x42, 3);
        break;
    case 1011:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 8, 0x29, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x16, 0xB, 3, 0x3000003);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x20, partHi | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x36, partHi | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0x2E, 3);
        break;
    case 1012:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 8, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 6, 0x29, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x16, 0xB, 3, 0x3000003);
        fn_800DA428(seWorkFrame, 0x36, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0xA, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        break;
    case 1013:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 8, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 6, 0x29, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x16, 0xB, 3, 0x3000003);
        fn_800DA428(seWorkFrame, 0x36, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0xA, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        break;
    case 1051:
        se_req_frame_set(seWorkMain, 4, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xA, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0x38, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1052:
        se_req_frame_set(seWorkMain, 2, 0xF, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 6, 0xA, 3, 4);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x3000003);
        break;
    case 1053:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x3000003);
        break;
    case 1057:
        se_req_frame_set(seWorkMain, 4, 0xF, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 8, 0xA, 3, 4);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x14, 0x3000003);
        break;
    case 1060:
        se_req_frame_set(seWorkPart, 2, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x28, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x3000003);
        break;
    case 1061:
        se_req_frame_set(seWorkPart, 2, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x32, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x3A, 0xED, 0x11, 0x3000003);
        break;
    case 1062:
        se_req_frame_set(seWorkPart, 2, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x48, 0xED, 0x14, 0x3000003);
        break;
    case 1064:
        se_req_frame_set(seWorkPart, 6, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x34, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x50, 0xED, 0x11, 0x3000003);
        break;
    case 1065:
        se_req_frame_set(seWorkPart, 6, 0xEA, 3, 0x3000003);
        break;
    case 1066:
        se_req_frame_set(seWorkPart, 6, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x34, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x50, 0xED, 0x11, 0x3000003);
        break;
    case 1067:
        se_req_frame_set(seWorkPart, 6, 0xEA, 3, 0x3000003);
        break;
    case 1068:
        se_req_frame_set(seWorkMain, 4, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 2, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x36, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x3000003);
        break;
    case 1069:
        se_req_frame_set(seWorkMain, 4, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 2, 0xF5, 0xD, 0x3000003);
        se_req_frame_set(seWorkPart, 0x2A, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x3000003);
        break;
    case 1101:
        se_req_frame_set(seWorkMain, 4, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_80229EA8(seWorkPart, 4, 7, 3);
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x14, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x36, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x14, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x4A, partHi | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x4C, 3);
        break;
    case 1102:
        se_req_frame_set(seWorkPart, 0x30, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x28, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x34, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0x66, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x30, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x9A, partHi | 1, 0x14, 2);
        fn_80229EA8(seWorkPart, 4, 7, 3);
        fn_80229E10(seWorkPart, 0x52, 3);
        break;
    case 1103:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0x10, 8, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x2A, 8, 0x14, 0x3000003);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 4, 0x11, 2);
        fn_80229EA8(seWorkPart, 0x2E, 7, 3);
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x26, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x40, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        break;
    case 1104:
        se_req_frame_set(seWorkPart, 0x7C, SE_Code_Make(3, 2, 3, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xE, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x26, 3, 3, 4);
        se_req_frame_set(seWorkMain, 0x68, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x7C, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x22, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0xA4, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x34, 3);
        fn_80229E10(seWorkPart, 0xA2, 3);
        break;
    case 1105:
        se_req_frame_set(seWorkPart, 0xE, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xE, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x32, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_80229E10(seWorkPart, 0x34, 3);
        fn_800DA428(seWorkFrame, 0x2A, (part << 0x18) | 4, 0x14, 2);
        fn_80229EA8(seWorkPart, 0x58, 7, 3);
        break;
    case 1106:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(3, 3, 3, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 0x42, SE_Code_Make(2, 4, 2, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 6, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x42, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4E, 4, 3, 4);
        se_req_frame_set(seWorkMain, 0x52, 5, 3, 4);
        se_req_frame_set(seWorkMain, 0xB2, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xEA, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x44, (part << 0x18), 0x14, 2);
        fn_80229EA8(seWorkPart, 0x22, 7, 3);
        fn_80229E10(seWorkPart, 0xAA, 3);
        fn_80229EA8(seWorkPart, 0x76, 7, 3);
        break;
    case 1109:
        fn_800DA428(seWorkFrame, 0xA, (part << 0x18), 0x14, 2);
        break;
    case 1110:
        se_req_frame_set(seWorkMain, 0xC, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x10, (part << 0x18) | 4, 0x14, 2);
        break;
    case 1111:
        se_req_frame_set(seWorkPart, 0x22, SE_Code_Make(2, 4, 2, 4), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 8, 0x14, 0x3000003);
        se_req_frame_set(seWorkMain, 0x22, 6, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 5, 3, 4);
        se_req_frame_set(seWorkMain, 0x58, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x8A, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x82, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x8E, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229E10(seWorkPart, 0x54, 3);
        break;
    case 1112:
        se_req_frame_set(seWorkPart, 0x2E, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 6, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x2A, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x36, 3, 3, 4);
        break;
    case 1113:
        se_req_frame_set(seWorkMain, 0xC, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1130:
        fn_800DA428(seWorkFrame, 0x10, (part << 0x18) | 4, 0x14, 2);
        se_req_frame_set(seWorkMain, 0xA, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_80229E10(seWorkPart, 0x12, 3);
        break;
    case 1131:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x12, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        break;
    case 1132:
        fn_800DA428(seWorkFrame, 0xC, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        se_req_frame_set(seWorkMain, 8, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1134:
        se_req_frame_set(seWorkPart, 0xA, SE_Code_Make(0, 2, 1, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xC, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_800DA428(seWorkFrame, 0x22, (part << 0x18) | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3A, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x2C, 3);
        break;
    case 1135:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 6, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xE, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x76, partHi | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        fn_80229E10(seWorkPart, 0x60, 3);
        fn_80229EA8(seWorkPart, 0x22, 7, 3);
        break;
    case 1136:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 2, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1C, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x56, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 2, 8, 0x14, 0x3000003);
        fn_800DA428(seWorkFrame, 0x54, (part << 0x18) | 1, 0x14, 2);
        fn_80229E10(seWorkPart, 4, 3);
        fn_80229E10(seWorkPart, 0x56, 3);
        break;
    case 1140:
        se_req_frame_set(seWorkMain, 0xA, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x20, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x20, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        break;
    case 1141:
        fn_800DA428(seWorkFrame, 6, (part << 0x18) | 1, 0x11, 2);
        se_req_frame_set(seWorkMain, 6, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1142:
        se_req_frame_set(seWorkMain, 4, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1143:
        se_req_frame_set(seWorkMain, 0xC, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1144:
        se_req_frame_set(seWorkMain, 4, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1E, 3, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x20, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x22, 3);
        break;
    case 1151:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x16, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 4, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x14, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x14, 0x3000003);
        break;
    case 1152:
        se_req_frame_set(seWorkPart, 0x30, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x2C, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 4, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x2A, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x6A, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x70, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x7C, 0xED, 0x11, 0x3000003);
        break;
    case 1153:
        se_req_frame_set(seWorkPart, 0x1E, SE_Code_Make(0x16, 4, 0x16, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x1A, 0xC, 3, 4);
        se_req_frame_set(seWorkMain, 0x1A, 0x10, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x58, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x6C, 0xED, 0x14, 0x3000003);
        break;
    case 1154:
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 8, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1156:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x3000003);
        break;
    case 1157:
        se_req_frame_set(seWorkPart, 0xE, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x12, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x14, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0x32, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x11, 0x3000003);
        break;
    case 1158:
        se_req_frame_set(seWorkPart, 0x10, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 6, 0x10, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xE, 0xC, 3, 4);
        se_req_frame_set(seWorkMain, 0x48, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x5E, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x80, 0xED, 0x11, 0x3000003);
        break;
    case 1159:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(0x17, 3, 0x17, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 0x42, SE_Code_Make(0x16, 4, 0x16, 4), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x4E, 0xB, 3, 4);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0x44, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 6, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x42, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0xAC, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0xDA, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1160:
        se_req_frame_set(seWorkPart, 8, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0x10, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 0xC, 0x10, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x36, 0xED, 0x14, 0x3000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x11, 0x3000003);
        break;
    case 1161:
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1162:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        se_req_frame_set(seWorkPart, 4, 0x8C, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x26, 0x8C, 0xB, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0x8C, 0xB, 0x3000003);
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x26, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x40, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        break;
    case 1163:
        se_req_frame_set(seWorkPart, 0x84, SE_Code_Make(0x17, 2, 0x17, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 0xE, 0x10, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x52, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x68, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x7C, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x64, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x92, 0xED, 0x14, 0x3000003);
        break;
    case 1164:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x16, 3, 0x16, 3), 0xC, 3);
        se_req_frame_set(seWorkMain, 8, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x1A, 0x10, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1A, 0xC, 3, 4);
        se_req_frame_set(seWorkMain, 0x42, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x54, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x34, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x14, 0x3000003);
        break;
    case 1170:
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 8, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        break;
    case 1171:
        se_req_frame_set(seWorkMain, 6, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1172:
        se_req_frame_set(seWorkMain, 6, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1173:
        se_req_frame_set(seWorkMain, 0xC, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1174:
        se_req_frame_set(seWorkMain, 0xC, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1175:
        se_req_frame_set(seWorkMain, 0xC, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1176:
        se_req_frame_set(seWorkMain, 6, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1177:
        se_req_frame_set(seWorkMain, 6, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 0x20, 0xD, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1178:
        se_req_frame_set(seWorkMain, 0xE, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1179:
        se_req_frame_set(seWorkMain, 0xC, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1201:
        fn_80229E10(seWorkPart, 2, 3);
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(4, 2, 4, 2), 0xC, 3);
        break;
    case 1221:
        se_req_frame_set(seWorkMain, 0x38, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        fn_80229E10(seWorkPart, 2, 3);
        fn_80229E10(seWorkPart, 0xA, 3);
        fn_80229E10(seWorkPart, 0x1E, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xC, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x20, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x5E, partHi | 1, 0x11, 2);
        break;
    case 1251:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0x18, 2, 0x18, 2), 0xC, 3);
        se_req_frame_set(seWorkMain, 4, 0xA, 3, 4);
        break;
    case 1271:
        se_req_frame_set(seWorkMain, 2, 0xA, 3, 4);
        se_req_frame_set(seWorkMain, 8, 0xE, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
        se_req_frame_set(seWorkPart, 0x2C, 0xED, 0x11, 0x3000003);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x14, 0x3000003);
        break;
    }
}

