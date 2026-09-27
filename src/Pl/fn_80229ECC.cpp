/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py - `zz_0229ecc_` at 0x80229ECC - and config/RMHE08/symbols.txt: every callee
 * that is not a C++ mangling is a bare `fn_` placeholder with no signature)
 *
 * Pl/fn_80229ECC.cpp - the player actor's per-motion SE / effect request dispatch.
 *
 * `.text` 0x80229ECC-0x80230FBC (0x70F0 B, one function), extab 0x80011C7C-0x80011C84 (large frame,
 * saved r28-r31), extabindex 0x8002E9A4-0x8002E9B0, and the compiler-emitted 635-entry jump table
 * `.data` 0x805C15A8-0x805C1F94 (0x9EC B) - all four ranges are registered in splits.txt.
 *
 * The body gates on `event_demo_ck() == 1`, `self->field_0x001 == 0` and
 * `PlayMode_ck() == 3 && fn_8026FD94(self) == 0`, then takes the actor's two `_se_w` works
 * (`field_0xAF4` -> `se_work_a`, `field_0xAF8` -> `se_work_b`) and the carve value
 * `v = fn_8027D050(self)`, falling back to `self->field_0x5A7` when that is 0.  It switches on
 * `(u16)Get_motion_no(self)` (0..0x27A, 635 values): 250 distinct bodies issue
 * `se_req_frame_set` / `fn_800DA428` / `fn_80229E10` / `fn_80229EA8` requests and `SE_Code_Make`
 * lookups against those two works, and two motions call
 * `fn_80244E88(&self->field_0xAF4, (u8)v, motion, self->field_0x002)`.  The remaining 385 case values
 * are empty and fall straight to the shared epilogue; a case body that repeats an earlier one is a
 * **second label on the same body** (MWCC emits one copy per distinct body, and the target's jump
 * table points every such value at it), which is why this file writes those labels as one group.
 *
 * Module (docs/plan.md 6.5 naming, the brief's evidence order):
 *   1. no `__FILE__` string - every `lbl_` referenced by every function in the 0x801ECA00-0x8026BA1C
 *      gap was resolved and read out of the DOL, and none holds a `.c`/`.cpp` name;
 *   2. no runtime-dump name - `dumpmap.py lookup 0x80229ECC` answers `zz_0229ecc_`;
 *   3. the class-3 evidence is conclusive for the module: the parameter is `_PLW*`
 *      (`Get_motion_no__FP4_PLW`), and two of the unnamed callees sit *inside* registered Pl ranges
 *      (`fn_8026FD94` in `pl_master`'s, `fn_8027D050` in `pl_act`'s), so this is the `Pl` lib.  The
 *      bracket test is silent (lobby below, Pl above), and the orchestrator confirmed `Pl`.
 *   Class 4 for the file name: nothing names it, so it keeps the map's `fn_80229ECC` stem.
 *
 * Language: C++ (high).  `PlayMode_ck__Fv`, `event_demo_ck__Fv`, `Get_motion_no__FP4_PLW`,
 * `se_req_frame_set__FP5_se_wllll` and `SE_Code_Make__Flsls` are manglings; the function itself is
 * `extern "C"` because the map spells it `fn_80229ECC` (unmangled) - a C++ definition would mangle it
 * to `fn_80229ECC__FP4_PLW` and pair nothing (playbook 42/48).
 *
 * Flags: the unit's `cflags_pl` exactly (`Wii/1.0`, `-O3 -inline noauto -opt nopeephole
 * -Cpp_exceptions on`) - no per-unit deviation, no pragma.
 *
 * Residual: **none**.  `build/RMHE08/report.json` reports 100.000000 fuzzy, 28912/28912 `.text` bytes,
 * 2560/2560 data bytes (`.data` 2540 + extab 8 + extabindex 12; every section 100.0), 1/1 functions.
 * The object is byte-identical to the target, so the unit is a flip candidate.
 *
 * Declaration homes (rule 2): the four foreign symbols a registered unit owns live in that unit's
 * header (`Pl/pl_act.h`, `Pl/pl_master.h`, `sound/fn_800D7F54.h`, `ef/fn_800CDB2C.h`); the five with
 * no owner live in the unsplit band (`unsplit/Pl.h`, `unsplit/unknown.h` for `Get_motion_no`).  Two
 * `_PLW` fields are named here for the first time - `field_0xAF4` / `field_0xAF8` (the actor's two
 * `_se_w` works) - added to `include/pl.h`; `pad_0x001`, `unk2` and `unk5A7` there were renamed to
 * `field_0x001`, `field_0x002` and `field_0x5A7` because this unit reads all three (rule 5).
 */

#include "types.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/fn_802693C4.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"

extern "C" void fn_80229ECC(_PLW* self)
{
    _se_w* se_work_a;
    _se_w* se_work_b;
    u32 v;
    u16 motion;

    if (event_demo_ck() == 1) {
        return;
    }
    if (self->field_0x001 == 0) {
        return;
    }
    if ((u8)PlayMode_ck() == 3) {
        if (fn_8026FD94(self) == 0) {
            return;
        }
    }
    fn_80229CB4(self);
    se_work_a = self->field_0xAF4;
    se_work_b = self->field_0xAF8;
    v = fn_8027D050(self);
    if ((u8)v == 0) {
        v = self->field_0x5A7;
    }
    motion = (u16)Get_motion_no(self);
    switch (motion) {
case 0: case 1: case 4: case 8: case 14: case 20: case 23: case 24: case 25: case 27: case 29: case 31: case 32: case 33: case 40: case 43: case 44: case 45: case 47: case 51: case 61: case 65: case 67: case 68: case 69: case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97: case 98: case 99: case 111: case 114: case 116: case 125: case 127: case 128: case 129: case 130: case 131: case 132: case 136: case 143: case 144: case 145: case 146: case 147: case 148: case 149: case 150: case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159: case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167: case 168: case 169: case 170: case 171: case 172: case 173: case 174: case 175: case 176: case 177: case 178: case 179: case 180: case 181: case 182: case 183: case 184: case 185: case 186: case 187: case 188: case 189: case 190: case 191: case 192: case 193: case 194: case 195: case 196: case 197: case 198: case 199: case 200: case 203: case 205: case 208: case 213: case 217: case 219: case 233: case 234: case 235: case 236: case 237: case 238: case 239: case 247: case 249: case 254: case 272: case 274: case 275: case 276: case 277: case 278: case 279: case 280: case 281: case 282: case 283: case 284: case 285: case 286: case 287: case 288: case 289: case 290: case 291: case 292: case 293: case 294: case 295: case 296: case 297: case 298: case 299: case 300: case 304: case 327: case 330: case 337: case 339: case 366: case 367: case 368: case 369: case 370: case 371: case 372: case 373: case 374: case 375: case 376: case 377: case 378: case 379: case 380: case 381: case 382: case 383: case 384: case 385: case 386: case 387: case 388: case 389: case 390: case 391: case 392: case 393: case 394: case 395: case 396: case 397: case 398: case 399: case 400: case 405: case 410: case 415: case 416: case 417: case 418: case 419: case 424: case 425: case 426: case 427: case 428: case 429: case 430: case 431: case 432: case 433: case 434: case 435: case 436: case 437: case 438: case 439: case 440: case 441: case 442: case 443: case 444: case 445: case 446: case 447: case 448: case 449: case 450: case 451: case 452: case 453: case 454: case 455: case 456: case 457: case 458: case 459: case 460: case 461: case 462: case 463: case 464: case 465: case 466: case 467: case 468: case 469: case 470: case 471: case 472: case 473: case 474: case 475: case 476: case 477: case 478: case 479: case 480: case 481: case 482: case 483: case 484: case 485: case 486: case 487: case 488: case 489: case 490: case 491: case 492: case 493: case 494: case 495: case 496: case 497: case 498: case 499: case 519: case 528: case 535: case 536: case 537: case 538: case 539: case 540: case 541: case 542: case 543: case 544: case 545: case 546: case 547: case 548: case 549: case 550: case 551: case 552: case 553: case 554: case 555: case 556: case 557: case 558: case 559: case 560: case 561: case 562: case 563: case 564: case 565: case 566: case 567: case 568: case 569: case 570: case 571: case 572: case 573: case 574: case 575: case 576: case 577: case 578: case 579: case 580: case 581: case 582: case 583: case 584: case 585: case 586: case 587: case 588: case 589: case 590: case 591: case 592: case 593: case 594: case 595: case 596: case 597: case 598: case 599: case 601: case 602: case 605: case 606: case 607: case 608: case 609: case 610: case 611: case 612: case 613: case 616: case 618: case 622: case 624: case 625: case 626: case 629: case 630: case 631: case 632: case 633:
    break;
case 2:
    fn_800DA428(se_work_b, 20, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 56, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 90, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 22, 3);
    fn_80229E10(se_work_a, 58, 3);
    fn_80229E10(se_work_a, 92, 3);
    break;
case 3:
    se_req_frame_set(se_work_a, 56, SE_Code_Make(7, 1, 8, 1), 12, 3);
    fn_800DA428(se_work_b, 8, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 30, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 54, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 10, 3);
    fn_80229E10(se_work_a, 32, 3);
    fn_80229E10(se_work_a, 56, 3);
    break;
case 5:
    se_req_frame_set(se_work_a, 80, SE_Code_Make(9, 3, 9, 3), 12, 3);
    se_req_frame_set(se_work_a, 164, SE_Code_Make(9, 4, 9, 4), 12, 3);
    fn_800DA428(se_work_b, 6, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 30, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 68, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 40, 3);
    break;
case 6:
    fn_800DA428(se_work_b, 30, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 34, 3);
    break;
case 7: case 9:
    se_req_frame_set(se_work_a, 4, 28, 14, 0x3000003);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 10:
    se_req_frame_set(se_work_a, 38, 16, 16, 0x3000003);
    se_req_frame_set(se_work_a, 108, 16, 16, 0x3000003);
    fn_800DA428(se_work_b, 30, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 84, 7, 3);
    fn_80229EA8(se_work_a, 14, 7, 3);
    break;
case 11:
    fn_80229EA8(se_work_a, 4, 7, 3);
    break;
case 12:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(0, 3, 1, 2), 12, 3);
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 13:
    se_req_frame_set(se_work_a, 6, 13, 14, 0x3000002);
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 8, 3);
    break;
case 15:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_80229E10(se_work_a, 10, 3);
    break;
case 16:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 8, 3);
    break;
case 17:
    fn_80229EA8(se_work_a, 4, 7, 3);
    break;
case 18:
    se_req_frame_set(se_work_a, 60, SE_Code_Make(3, 2, 3, 2), 12, 3);
    se_req_frame_set(se_work_a, 94, 17, 16, 0x3000002);
    fn_800DA428(se_work_b, 92, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 112, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 90, 3);
    fn_80229EA8(se_work_a, 12, 11, 3);
    fn_80229EA8(se_work_a, 44, 7, 3);
    break;
case 19:
    se_req_frame_set(se_work_a, 4, 10, 14, 0x3000002);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 6, 3);
    fn_80229E10(se_work_a, 100, 3);
    fn_80229EA8(se_work_a, 74, 11, 3);
    break;
case 21:
    se_req_frame_set(se_work_a, 84, SE_Code_Make(9, 2, 9, 2), 12, 3);
    fn_800DA428(se_work_b, 34, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 74, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 40, 3);
    fn_80229E10(se_work_a, 78, 3);
    break;
case 22:
    fn_800DA428(se_work_b, 6, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_800DA428(se_work_b, 14, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_80229E10(se_work_a, 14, 3);
    break;
case 26:
    fn_800DA428(se_work_b, 10, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 52, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 78, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 102, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 166, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 174, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 200, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 212, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 82, 3);
    fn_80229E10(se_work_a, 144, 3);
    break;
case 28:
    se_req_frame_set(se_work_a, 6, SE_Code_Make(0, 3, 1, 1), 12, 3);
    se_req_frame_set(se_work_a, 4, 41, 12, 0x3000003);
    se_req_frame_set(se_work_a, 28, 11, 14, 0x3000002);
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_800DA428(se_work_b, 46, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 20, 3);
    fn_80229E10(se_work_a, 52, 3);
    break;
case 30:
    fn_800DA428(se_work_b, 6, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 16, 3);
    break;
case 34:
    fn_800DA428(se_work_b, 4, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 56, (((u8)(v)) << 24), 7, 2);
    fn_800DA428(se_work_b, 98, (((u8)(v)) << 24), 11, 2);
    fn_80229E10(se_work_a, 18, 3);
    fn_80229EA8(se_work_a, 56, 7, 3);
    fn_80229EA8(se_work_a, 104, 11, 3);
    break;
case 35:
    fn_800DA428(se_work_b, 38, (((u8)(v)) << 24), 7, 2);
    fn_800DA428(se_work_b, 82, (((u8)(v)) << 24), 11, 2);
    fn_80229E10(se_work_a, 46, 3);
    fn_80229E10(se_work_a, 84, 3);
    break;
case 36:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x4), 17, 2);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 37:
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 36, ((((u8)(v)) << 24) | 0x1), 17, 2);
    break;
case 38:
    se_req_frame_set(se_work_a, 16, SE_Code_Make(7, 3, 8, 2), 12, 3);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_80229E10(se_work_a, 16, 3);
    fn_80229E10(se_work_a, 32, 3);
    break;
case 39:
    se_req_frame_set(se_work_a, 42, 9, 12, 3);
    fn_800DA428(se_work_b, 18, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 48, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 72, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 24, 3);
    fn_80229E10(se_work_a, 74, 3);
    break;
case 41:
    fn_800DA428(se_work_b, 10, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229EA8(se_work_a, 4, 7, 3);
    break;
case 42:
    fn_800DA428(se_work_b, 20, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_80229E10(se_work_a, 24, 3);
    break;
case 46:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 48:
    se_req_frame_set(se_work_a, 4, 12, 14, 0x3000002);
    se_req_frame_set(se_work_a, 56, 46, 7, 0x3000002);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x3), 14, 2);
    fn_800DA428(se_work_b, 80, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 82, 3);
    fn_80229E10(se_work_a, 100, 3);
    se_req_frame_set(se_work_a, 8, 9, 14, 0x3000002);
    break;
case 49:
    fn_800DA428(se_work_b, 38, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x1), 17, 2);
    break;
case 50:
    se_req_frame_set(se_work_a, 20, SE_Code_Make(0, 2, 1, 2), 12, 3);
    se_req_frame_set(se_work_a, 18, 40, 20, 0x3000003);
    fn_800DA428(se_work_b, 20, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 32, 3);
    break;
case 52:
    fn_800DA428(se_work_b, 18, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 48, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 20, 3);
    fn_80229E10(se_work_a, 50, 3);
    break;
case 53:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(7, 2, 8, 2), 12, 3);
    fn_800DA428(se_work_b, 20, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 42, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 22, 3);
    fn_80229EA8(se_work_a, 40, 7, 3);
    break;
case 54:
    se_req_frame_set(se_work_a, 36, SE_Code_Make(3, 2, 3, 2), 12, 3);
    fn_800DA428(se_work_b, 32, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 76, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 162, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 192, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 34, 3);
    fn_80229E10(se_work_a, 194, 3);
    break;
case 55:
    fn_800DA428(se_work_b, 16, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 18, 3);
    fn_80229EA8(se_work_a, 34, 7, 3);
    break;
case 56:
    fn_800DA428(se_work_b, 2, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 57:
    se_req_frame_set(se_work_a, 4, 10, 20, 0x3000003);
    fn_800DA428(se_work_b, 88, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 90, 3);
    break;
case 58:
    fn_800DA428(se_work_b, 6, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_800DA428(se_work_b, 10, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 12, 3);
    break;
case 59:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(7, 2, 8, 2), 12, 3);
    fn_800DA428(se_work_b, 10, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 24, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 14, 3);
    break;
case 60:
    fn_80229EA8(se_work_a, 20, 11, 3);
    break;
case 62:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_800DA428(se_work_b, 10, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 14, 3);
    break;
case 63:
    fn_80229E10(se_work_a, 4, 3);
    break;
case 64:
    fn_800DA428(se_work_b, 26, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 60, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 28, 3);
    fn_80229E10(se_work_a, 62, 3);
    break;
case 66:
    fn_800DA428(se_work_b, 42, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 70, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 54, 3);
    break;
case 100:
    se_req_frame_set(se_work_a, 140, 234, 3, 0x3000003);
    se_req_frame_set(se_work_a, 200, 234, 3, 0x3000003);
    break;
case 101:
    se_req_frame_set(se_work_a, 4, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 28, 237, 17, 0x3000003);
    break;
case 102:
    se_req_frame_set(se_work_a, 8, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 28, 237, 17, 0x3000003);
    break;
case 103:
    se_req_frame_set(se_work_a, 6, 245, 13, 0x3000003);
    break;
case 104:
    se_req_frame_set(se_work_a, 6, 245, 13, 0x3000003);
    break;
case 105:
    se_req_frame_set(se_work_a, 6, 245, 13, 0x3000003);
    break;
case 106:
    se_req_frame_set(se_work_a, 24, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 36, 237, 17, 0x3000003);
    break;
case 107:
    fn_800DA428(se_work_b, 4, ((((u8)(v)) << 24) | 0x4), 20, 2);
    se_req_frame_set(se_work_a, 38, 246, 13, 0x3000003);
    se_req_frame_set(se_work_a, 42, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 44, 232, 13, 0x3000003);
    break;
case 108:
    se_req_frame_set(se_work_a, 30, 20, 13, 0x3000003);
    fn_800DA428(se_work_b, 24, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 44, (((u8)(v)) << 24), 20, 2);
    break;
case 109:
    se_req_frame_set(se_work_a, 8, 238, 6, 0x3000003);
    se_req_frame_set(se_work_a, 12, 238, 10, 0x3000003);
    se_req_frame_set(se_work_a, 24, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 84, 234, 13, 0x3000003);
    break;
case 110:
    se_req_frame_set(se_work_a, 8, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 26, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 46, 238, 20, 0x3000003);
    break;
case 112:
    se_req_frame_set(se_work_a, 8, 234, 13, 0x3000003);
    break;
case 113:
    se_req_frame_set(se_work_a, 8, 245, 3, 0x3000003);
    se_req_frame_set(se_work_a, 10, 232, 3, 0x3000003);
    se_req_frame_set(se_work_a, 20, 250, 3, 0x3000003);
    se_req_frame_set(se_work_a, 30, 234, 3, 0x3000003);
    se_req_frame_set(se_work_a, 96, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 100, 238, 7, 0x3000003);
    break;
case 115:
    se_req_frame_set(se_work_a, 4, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 10, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 74, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 80, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 36, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 102, 234, 13, 0x3000003);
    break;
case 117:
    se_req_frame_set(se_work_a, 8, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 26, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 46, 238, 20, 0x3000003);
    break;
case 118:
    se_req_frame_set(se_work_a, 16, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 56, 238, 17, 0x3000003);
    break;
case 119:
    se_req_frame_set(se_work_a, 22, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 10, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 14, 238, 17, 0x3000003);
    break;
case 120:
    se_req_frame_set(se_work_a, 4, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 34, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 96, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 116, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 168, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 204, 237, 17, 0x3000003);
    break;
case 121:
    se_req_frame_set(se_work_a, 44, SE_Code_Make(24, 2, 24, 2), 12, 3);
    se_req_frame_set(se_work_a, 40, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 34, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 154, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 170, 238, 17, 0x3000003);
    break;
case 122:
    se_req_frame_set(se_work_a, 8, 234, 3, 0x3000003);
    se_req_frame_set(se_work_a, 20, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 38, 237, 11, 0x3000003);
    break;
case 123:
    se_req_frame_set(se_work_a, 16, 245, 11, 0x3000003);
    break;
case 124:
    se_req_frame_set(se_work_a, 6, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 50, 237, 17, 0x3000003);
    break;
case 126:
    se_req_frame_set(se_work_a, 6, 238, 3, 0x3000003);
    break;
case 133:
    se_req_frame_set(se_work_a, 36, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 12, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 14, 238, 20, 0x3000003);
    break;
case 134:
    se_req_frame_set(se_work_a, 36, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 12, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 14, 238, 20, 0x3000003);
    break;
case 135:
    se_req_frame_set(se_work_a, 8, 237, 7, 0x3000003);
    break;
case 137:
    se_req_frame_set(se_work_a, 4, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 18, 238, 17, 0x3000003);
    break;
case 138:
    se_req_frame_set(se_work_a, 4, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 18, 238, 17, 0x3000003);
    break;
case 139:
    fn_800DA428(se_work_b, 52, (((u8)(v)) << 24), 7, 2);
    fn_800DA428(se_work_b, 100, (((u8)(v)) << 24), 11, 2);
    break;
case 140:
    fn_800DA428(se_work_b, 4, (((u8)(v)) << 24), 7, 2);
    fn_800DA428(se_work_b, 52, (((u8)(v)) << 24), 11, 2);
    break;
case 141:
    se_req_frame_set(se_work_a, 4, 238, 17, 0x3000003);
    break;
case 142:
    se_req_frame_set(se_work_a, 10, SE_Code_Make(20, 2, 21, 2), 12, 3);
    se_req_frame_set(se_work_a, 6, 245, 3, 0x3000003);
    break;
case 201:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 4, 2), 12, 3);
    fn_80229E10(se_work_a, 6, 3);
    break;
case 202:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 26, 10, 14, 0x3000003);
    fn_800DA428(se_work_b, 12, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 14, 3);
    break;
case 204:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 14, 10, 14, 0x3000003);
    fn_800DA428(se_work_b, 26, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 26, 3);
    break;
case 206: case 209:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(5, 2, 6, 6), 12, 3);
    se_req_frame_set(se_work_a, 4, 15, 14, 0x3000003);
    se_req_frame_set(se_work_a, 6, 12, 14, 0x3000003);
    se_req_frame_set(se_work_a, 32, 12, 14, 0x3000003);
    se_req_frame_set(se_work_a, 116, 13, 14, 0x3000003);
    fn_80229E10(se_work_a, 56, 3);
    fn_80229E10(se_work_a, 84, 3);
    fn_80229E10(se_work_a, 116, 3);
    fn_80229EA8(se_work_a, 118, 7, 3);
    break;
case 207:
    fn_800DA428(se_work_b, 30, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 74, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 10, 3);
    break;
case 210:
    se_req_frame_set(se_work_a, 4, 9, 12, 3);
    se_req_frame_set(se_work_a, 66, 13, 19, 0x3000003);
    se_req_frame_set(se_work_a, 146, 46, 10, 0x3000003);
    se_req_frame_set(se_work_a, 152, 15, 14, 0x3000003);
    fn_80229E10(se_work_a, 72, 3);
    fn_80229EA8(se_work_a, 146, 11, 3);
    break;
case 211:
    se_req_frame_set(se_work_a, 16, 28, 14, 0x3000003);
    se_req_frame_set(se_work_a, 18, 46, 12, 0x3000003);
    fn_800DA428(se_work_b, 12, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 30, 3);
    fn_80229EA8(se_work_a, 32, 11, 3);
    break;
case 212:
    fn_800DA428(se_work_b, 94, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 114, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 98, 3);
    fn_80229EA8(se_work_a, 32, 7, 3);
    break;
case 214:
    fn_80229E10(se_work_a, 16, 3);
    fn_80229E10(se_work_a, 66, 3);
    break;
case 215:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 15, 12, 0x3000003);
    se_req_frame_set(se_work_a, 6, 12, 14, 0x3000003);
    se_req_frame_set(se_work_a, 106, 10, 14, 0x3000003);
    fn_80229E10(se_work_a, 24, 3);
    fn_80229EA8(se_work_a, 114, 7, 3);
    break;
case 216:
    se_req_frame_set(se_work_a, 14, 17, 14, 0x3000003);
    fn_800DA428(se_work_b, 44, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 64, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_80229E10(se_work_a, 74, 3);
    fn_80229EA8(se_work_a, 4, 7, 3);
    break;
case 218:
    se_req_frame_set(se_work_a, 10, 28, 14, 0x3000003);
    fn_80229EA8(se_work_a, 4, 7, 3);
    break;
case 220:
    fn_80229E10(se_work_a, 18, 3);
    break;
case 221:
    fn_80229E10(se_work_a, 22, 3);
    fn_80229EA8(se_work_a, 56, 7, 3);
    break;
case 222:
    se_req_frame_set(se_work_a, 38, 12, 14, 0x3000003);
    fn_80229E10(se_work_a, 46, 3);
    break;
case 223:
    fn_80229E10(se_work_a, 6, 3);
    fn_80229EA8(se_work_a, 18, 7, 3);
    break;
case 224:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 4, 5, 4), 12, 3);
    se_req_frame_set(se_work_a, 64, 9, 12, 3);
    se_req_frame_set(se_work_a, 64, 14, 14, 0x3000003);
    fn_800DA428(se_work_b, 20, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 20, 3);
    break;
case 225:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 4, 5, 4), 12, 3);
    se_req_frame_set(se_work_a, 64, 9, 12, 3);
    se_req_frame_set(se_work_a, 118, 14, 19, 0x3000003);
    se_req_frame_set(se_work_a, 170, 11, 14, 0x3000003);
    fn_80229E10(se_work_a, 54, 3);
    fn_80229EA8(se_work_a, 50, 7, 3);
    break;
case 226:
    fn_800DA428(se_work_b, 54, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 72, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 74, 3);
    break;
case 227:
    se_req_frame_set(se_work_a, 22, SE_Code_Make(4, 3, 5, 3), 12, 3);
    se_req_frame_set(se_work_a, 22, 9, 17, 0x3000003);
    se_req_frame_set(se_work_a, 78, 11, 14, 0x3000003);
    se_req_frame_set(se_work_a, 128, 17, 14, 0x3000003);
    fn_800DA428(se_work_b, 52, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229E10(se_work_a, 58, 3);
    fn_80229E10(se_work_a, 150, 3);
    break;
case 228:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 8, 11, 14, 0x3000003);
    se_req_frame_set(se_work_a, 26, 9, 17, 0x3000003);
    fn_80229E10(se_work_a, 10, 3);
    break;
case 229:
    se_req_frame_set(se_work_a, 18, 46, 10, 0x3000003);
    fn_800DA428(se_work_b, 56, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 58, 3);
    fn_80229EA8(se_work_a, 22, 11, 3);
    break;
case 230:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 15, 12, 0x3000003);
    se_req_frame_set(se_work_a, 6, 12, 14, 0x3000003);
    se_req_frame_set(se_work_a, 32, 9, 17, 0x3000003);
    fn_80229E10(se_work_a, 34, 3);
    fn_80229EA8(se_work_a, 44, 11, 3);
    break;
case 231:
    se_req_frame_set(se_work_a, 10, 17, 10, 0x3000003);
    se_req_frame_set(se_work_a, 74, 16, 10, 0x3000003);
    fn_800DA428(se_work_b, 52, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 54, 3);
    break;
case 232:
    fn_800DA428(se_work_b, 10, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 24, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 40, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 54, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 66, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 84, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 12, 3);
    fn_80229E10(se_work_a, 68, 3);
    fn_80229EA8(se_work_a, 44, 11, 3);
    break;
case 240:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(5, 2, 6, 6), 12, 3);
    se_req_frame_set(se_work_a, 36, 12, 3, 0x3000003);
    fn_80229E10(se_work_a, 56, 3);
    break;
case 241:
    se_req_frame_set(se_work_a, 30, 17, 3, 0x3000003);
    se_req_frame_set(se_work_a, 104, 17, 3, 0x3000003);
    se_req_frame_set(se_work_a, 200, 17, 3, 0x3000003);
    se_req_frame_set(se_work_a, 294, 17, 3, 0x3000003);
    fn_80229E10(se_work_a, 50, 3);
    fn_80229E10(se_work_a, 112, 3);
    fn_80229E10(se_work_a, 208, 3);
    fn_80229E10(se_work_a, 374, 3);
    fn_80229EA8(se_work_a, 72, 11, 3);
    break;
case 242:
    se_req_frame_set(se_work_a, 8, SE_Code_Make(4, 3, 5, 3), 12, 3);
    se_req_frame_set(se_work_a, 60, 13, 16, 0x3000003);
    fn_80229E10(se_work_a, 12, 3);
    fn_80229E10(se_work_a, 70, 3);
    break;
case 243:
    se_req_frame_set(se_work_a, 36, SE_Code_Make(5, 2, 6, 2), 12, 3);
    se_req_frame_set(se_work_a, 36, 11, 3, 0x3000003);
    se_req_frame_set(se_work_a, 110, 13, 3, 0x3000003);
    fn_80229E10(se_work_a, 112, 3);
    break;
case 244:
    se_req_frame_set(se_work_a, 38, SE_Code_Make(4, 2, 5, 2), 12, 3);
    se_req_frame_set(se_work_a, 34, 13, 3, 0x3000003);
    fn_80229E10(se_work_a, 70, 3);
    fn_80229E10(se_work_a, 112, 3);
    break;
case 245:
    se_req_frame_set(se_work_a, 34, SE_Code_Make(4, 4, 6, 4), 12, 3);
    se_req_frame_set(se_work_a, 226, 9, 12, 3);
    se_req_frame_set(se_work_a, 12, 18, 3, 0x3000003);
    se_req_frame_set(se_work_a, 254, 10, 3, 0x3000003);
    fn_80229E10(se_work_a, 40, 3);
    fn_80229E10(se_work_a, 260, 3);
    fn_80229EA8(se_work_a, 60, 11, 3);
    break;
case 246:
    se_req_frame_set(se_work_a, 12, SE_Code_Make(2, 4, 2, 4), 12, 3);
    se_req_frame_set(se_work_a, 116, SE_Code_Make(3, 2, 3, 2), 12, 3);
    se_req_frame_set(se_work_a, 22, 17, 3, 0x3000003);
    se_req_frame_set(se_work_a, 12, 39, 11, 0x3000003);
    fn_80229EA8(se_work_a, 16, 11, 3);
    break;
case 248:
    se_req_frame_set(se_work_a, 68, 18, 3, 0x3000003);
    se_req_frame_set(se_work_a, 36, 46, 3, 0x3000003);
    fn_800DA428(se_work_b, 110, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 208, 3);
    fn_80229EA8(se_work_a, 32, 11, 3);
    fn_80229E10(se_work_a, 66, 3);
    break;
case 250:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 24, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 238, 3, 0x3000003);
    break;
case 251:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 25, 2), 12, 3);
    se_req_frame_set(se_work_a, 14, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 40, 238, 17, 0x3000003);
    break;
case 252:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 24, 2), 12, 3);
    se_req_frame_set(se_work_a, 14, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 44, 238, 17, 0x3000003);
    break;
case 253:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 26, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 114, 234, 13, 0x3000003);
    break;
case 255:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 26, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 80, 234, 13, 0x3000003);
    break;
case 256:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 26, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 56, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 80, 238, 17, 0x3000003);
    break;
case 257:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 26, 2), 12, 3);
    se_req_frame_set(se_work_a, 68, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 6, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 36, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 46, 238, 17, 0x3000003);
    break;
case 258:
    se_req_frame_set(se_work_a, 4, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 22, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 46, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 78, 237, 17, 0x3000003);
    break;
case 259:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 4, 26, 4), 12, 3);
    se_req_frame_set(se_work_a, 4, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 142, 234, 13, 0x3000003);
    break;
case 260:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 24, 2), 12, 3);
    se_req_frame_set(se_work_a, 12, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 18, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 58, 234, 13, 0x3000003);
    break;
case 261:
    se_req_frame_set(se_work_a, 30, 237, 3, 0x3000003);
    break;
case 262:
    se_req_frame_set(se_work_a, 4, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 130, 234, 13, 0x3000003);
    break;
case 263:
    se_req_frame_set(se_work_a, 4, 237, 13, 0x3000003);
    break;
case 264:
    se_req_frame_set(se_work_a, 86, 237, 11, 0x3000003);
    se_req_frame_set(se_work_a, 94, 237, 7, 0x3000003);
    break;
case 265:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 25, 2), 12, 3);
    se_req_frame_set(se_work_a, 6, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 18, 237, 17, 0x3000003);
    break;
case 266:
    se_req_frame_set(se_work_a, 6, 237, 11, 0x3000003);
    se_req_frame_set(se_work_a, 18, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 30, 237, 17, 0x3000003);
    break;
case 267: case 268: case 269: case 270:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 26, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 48, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 154, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 144, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 158, 237, 20, 0x3000003);
    break;
case 271:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(24, 2, 25, 2), 12, 3);
    se_req_frame_set(se_work_a, 4, 245, 13, 0x3000003);
    se_req_frame_set(se_work_a, 48, 234, 13, 0x3000003);
    se_req_frame_set(se_work_a, 104, 234, 13, 0x3000003);
    break;
case 273:
    se_req_frame_set(se_work_a, 68, 237, 11, 0x3000003);
    se_req_frame_set(se_work_a, 106, 237, 20, 0x3000003);
    se_req_frame_set(se_work_a, 96, 237, 17, 0x3000003);
    break;
case 301:
    se_req_frame_set(se_work_a, 22, SE_Code_Make(0, 2, 1, 2), 12, 3);
    se_req_frame_set(se_work_a, 20, 39, 10, 0x3000003);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x3), 17, 2);
    fn_80229EA8(se_work_a, 24, 11, 3);
    fn_80229E10(se_work_a, 40, 3);
    break;
case 302:
    se_req_frame_set(se_work_a, 122, 46, 3, 0x3000003);
    se_req_frame_set(se_work_a, 142, 46, 3, 0x3000003);
    fn_800DA428(se_work_b, 46, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 48, 11, 3);
    fn_80229EA8(se_work_a, 84, 11, 3);
    fn_80229EA8(se_work_a, 142, 11, 3);
    break;
case 303:
    se_req_frame_set(se_work_a, 28, SE_Code_Make(30, 3, 30, 3), 3, 0x3000003);
    se_req_frame_set(se_work_a, 226, SE_Code_Make(30, 3, 30, 3), 3, 0x3000003);
    fn_800DA428(se_work_b, 22, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 22, 3);
    break;
case 305:
    se_req_frame_set(se_work_a, 70, 47, 11, 0x3000003);
    se_req_frame_set(se_work_a, 116, 47, 11, 0x3000003);
    se_req_frame_set(se_work_a, 120, 47, 7, 0x3000003);
    se_req_frame_set(se_work_a, 146, 47, 11, 0x3000003);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 230, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 254, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229EA8(se_work_a, 40, 11, 3);
    fn_80229EA8(se_work_a, 72, 11, 3);
    fn_80229EA8(se_work_a, 120, 11, 3);
    fn_80229EA8(se_work_a, 192, 11, 3);
    fn_80229E10(se_work_a, 230, 3);
    break;
case 306:
    se_req_frame_set(se_work_a, 36, 45, 12, 0x3000003);
    break;
case 307:
    se_req_frame_set(se_work_a, 38, 28, 10, 0x3000003);
    fn_80229EA8(se_work_a, 52, 10, 3);
    fn_80229E10(se_work_a, 50, 3);
    break;
case 308:
    se_req_frame_set(se_work_a, 18, SE_Code_Make(10, 5, 11, 2), 12, 3);
    se_req_frame_set(se_work_a, 38, SE_Code_Make(10, 2, 11, 4), 12, 3);
    se_req_frame_set(se_work_a, 66, SE_Code_Make(10, 4, 11, 2), 12, 3);
    se_req_frame_set(se_work_a, 84, SE_Code_Make(10, 2, 11, 5), 12, 3);
    break;
case 309:
    se_req_frame_set(se_work_a, 20, 46, 11, 0x3000002);
    fn_80229EA8(se_work_a, 10, 11, 3);
    break;
case 310:
    se_req_frame_set(se_work_a, 42, 46, 11, 0x3000002);
    se_req_frame_set(se_work_a, 78, 46, 11, 0x3000002);
    break;
case 311:
    se_req_frame_set(se_work_a, 10, 16, 19, 0x3000003);
    break;
case 312:
    fn_800DA428(se_work_b, 12, ((((u8)(v)) << 24) | 0x1), 17, 2);
    se_req_frame_set(se_work_a, 16, 16, 17, 0x3000003);
    break;
case 313:
    fn_80244E88((&self->field_0xAF4), ((u8)(v)), motion, self->field_0x002);
    break;
case 314:
    fn_800DA428(se_work_b, 22, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 48, (((u8)(v)) << 24), 20, 2);
    se_req_frame_set(se_work_a, 62, 28, 10, 0x3000003);
    fn_80229E10(se_work_a, 22, 3);
    fn_80229EA8(se_work_a, 46, 11, 3);
    break;
case 315:
    se_req_frame_set(se_work_a, 8, 39, 11, 0x3000003);
    se_req_frame_set(se_work_a, 26, 41, 11, 0x3000003);
    se_req_frame_set(se_work_a, 42, 42, 11, 0x3000003);
    fn_800DA428(se_work_b, 46, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_800DA428(se_work_b, 78, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 92, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 130, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 156, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 48, 3);
    fn_80229E10(se_work_a, 94, 3);
    fn_80229E10(se_work_a, 158, 3);
    break;
case 316:
    se_req_frame_set(se_work_a, 4, 28, 3, 0x3000003);
    se_req_frame_set(se_work_a, 54, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 68, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 78, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 94, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 110, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 132, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 146, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 152, 31, 12, 0x3000003);
    se_req_frame_set(se_work_a, 172, 45, 12, 0x3000003);
    fn_800DA428(se_work_b, 4, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 176, ((((u8)(v)) << 24) | 0x1), 20, 2);
    break;
case 317:
    se_req_frame_set(se_work_a, 22, SE_Code_Make(0, 2, 1, 2), 12, 3);
    se_req_frame_set(se_work_a, 12, 40, 11, 0x3000003);
    fn_800DA428(se_work_b, 12, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 70, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229EA8(se_work_a, 16, 11, 3);
    fn_80229E10(se_work_a, 70, 3);
    break;
case 318:
    se_req_frame_set(se_work_a, 32, 43, 11, 0x3000003);
    fn_800DA428(se_work_b, 28, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 194, (((u8)(v)) << 24), 17, 2);
    fn_80229EA8(se_work_a, 14, 11, 3);
    fn_80229EA8(se_work_a, 104, 11, 3);
    fn_80229E10(se_work_a, 194, 3);
    break;
case 319:
    se_req_frame_set(se_work_a, 30, 32, 7, 0x3000003);
    se_req_frame_set(se_work_a, 64, 33, 7, 0x3000003);
    se_req_frame_set(se_work_a, 98, 34, 7, 0x3000003);
    se_req_frame_set(se_work_a, 172, 34, 7, 0x3000003);
    se_req_frame_set(se_work_a, 228, 33, 7, 0x3000003);
    se_req_frame_set(se_work_a, 228, 32, 7, 0x3000003);
    se_req_frame_set(se_work_a, 286, 46, 11, 0x3000003);
    fn_800DA428(se_work_b, 24, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 58, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 316, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 336, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 56, 3);
    fn_80229E10(se_work_a, 318, 3);
    break;
case 320:
    se_req_frame_set(se_work_a, 6, 28, 3, 0x3000003);
    se_req_frame_set(se_work_a, 70, 6, 13, 0x3000003);
    fn_80229EA8(se_work_a, 118, 11, 3);
    fn_800DA428(se_work_b, 24, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 26, 3);
    break;
case 321:
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 28, 3);
    break;
case 322:
    se_req_frame_set(se_work_a, 28, 32, 7, 0x3000003);
    se_req_frame_set(se_work_a, 58, 33, 7, 0x3000003);
    fn_800DA428(se_work_b, 14, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 48, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 18, 3);
    fn_80229E10(se_work_a, 50, 3);
    break;
case 323:
    fn_800DA428(se_work_b, 18, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 30, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 20, 3);
    fn_80229E10(se_work_a, 32, 3);
    break;
case 324:
    se_req_frame_set(se_work_a, 10, 28, 13, 0x3000003);
    se_req_frame_set(se_work_a, 72, 7, 13, 0x3000003);
    fn_80229EA8(se_work_a, 30, 11, 3);
    fn_800DA428(se_work_b, 24, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 26, 3);
    break;
case 325:
    fn_800DA428(se_work_b, 48, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 78, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 50, 3);
    fn_80229E10(se_work_a, 80, 3);
    break;
case 326:
    se_req_frame_set(se_work_a, 168, 39, 3, 0x3000003);
    se_req_frame_set(se_work_a, 204, 11, 3, 0x3000003);
    se_req_frame_set(se_work_a, 234, 13, 14, 0x3000003);
    fn_800DA428(se_work_b, 46, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 74, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 166, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 172, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_80229E10(se_work_a, 210, 3);
    fn_80229E10(se_work_a, 234, 3);
    fn_80229EA8(se_work_a, 332, 11, 3);
    break;
case 328:
    fn_80229EA8(se_work_a, 30, 7, 3);
    break;
case 329:
    se_req_frame_set(se_work_a, 30, 16, 14, 0x3000003);
    se_req_frame_set(se_work_a, 192, 46, 7, 0x3000002);
    se_req_frame_set(se_work_a, 240, 17, 14, 0x3000003);
    se_req_frame_set(se_work_a, 305, 17, 14, 0x3000003);
    fn_800DA428(se_work_b, 350, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 414, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 244, 3);
    fn_80229E10(se_work_a, 306, 3);
    fn_80229E10(se_work_a, 352, 3);
    fn_80229E10(se_work_a, 416, 3);
    fn_80229EA8(se_work_a, 56, 7, 3);
    fn_80229EA8(se_work_a, 122, 7, 3);
    break;
case 331:
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 70, 7, 3);
    fn_80229E10(se_work_a, 30, 3);
    se_req_frame_set(se_work_a, 132, SE_Code_Make(7, 3, 8, 1), 12, 3);
    se_req_frame_set(se_work_a, 242, SE_Code_Make(7, 1, 8, 3), 12, 3);
    break;
case 332:
    se_req_frame_set(se_work_a, 30, 3, 12, 3);
    fn_800DA428(se_work_b, 8, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 106, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 8, 3);
    fn_80229EA8(se_work_a, 46, 7, 3);
    break;
case 333:
    se_req_frame_set(se_work_a, 12, 28, 11, 0x3000003);
    se_req_frame_set(se_work_a, 24, 13, 11, 0x3000002);
    se_req_frame_set(se_work_a, 68, 13, 14, 0x3000002);
    fn_800DA428(se_work_b, 54, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 54, 3);
    break;
case 334:
    se_req_frame_set(se_work_a, 4, 28, 3, 0x3000003);
    fn_800DA428(se_work_b, 18, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 20, 3);
    break;
case 335:
    se_req_frame_set(se_work_a, 4, 28, 6, 0x3000003);
    se_req_frame_set(se_work_a, 56, 9, 12, 3);
    fn_800DA428(se_work_b, 118, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 118, 3);
    break;
case 336:
    se_req_frame_set(se_work_a, 4, 26, 6, 0x3000002);
    break;
case 338:
    se_req_frame_set(se_work_a, 100, 2, 6, 0x3000002);
    fn_800DA428(se_work_b, 60, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 94, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 142, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 94, 3);
    fn_80229EA8(se_work_a, 106, 11, 3);
    break;
case 340:
    se_req_frame_set(se_work_a, 44, 46, 6, 0x3000002);
    se_req_frame_set(se_work_a, 54, 3, 6, 0x3000002);
    se_req_frame_set(se_work_a, 96, 3, 6, 0x3000002);
    se_req_frame_set(se_work_a, 210, 3, 6, 0x3000002);
    se_req_frame_set(se_work_a, 160, 3, 6, 0x3000002);
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x4), 17, 2);
    fn_800DA428(se_work_b, 52, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 84, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 34, 3);
    fn_80229E10(se_work_a, 54, 3);
    fn_80229E10(se_work_a, 94, 3);
    fn_80229E10(se_work_a, 220, 3);
    fn_80229EA8(se_work_a, 34, 7, 3);
    break;
case 341:
    se_req_frame_set(se_work_a, 16, 3, 12, 3);
    se_req_frame_set(se_work_a, 12, 2, 6, 0x3000002);
    se_req_frame_set(se_work_a, 22, 3, 6, 0x3000002);
    se_req_frame_set(se_work_a, 84, 46, 6, 0x3000002);
    fn_800DA428(se_work_b, 24, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 76, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 222, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 24, 3);
    break;
case 342:
    se_req_frame_set(se_work_a, 28, 2, 6, 0x3000002);
    fn_800DA428(se_work_b, 66, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 94, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 212, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 94, 3);
    break;
case 343:
    se_req_frame_set(se_work_a, 18, SE_Code_Make(6, 4, 6, 4), 12, 3);
    se_req_frame_set(se_work_a, 54, 11, 14, 0x3000002);
    se_req_frame_set(se_work_a, 4, 3, 6, 0x3000002);
    se_req_frame_set(se_work_a, 10, 2, 6, 0x3000002);
    fn_800DA428(se_work_b, 176, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 126, 3);
    break;
case 344:
    se_req_frame_set(se_work_a, 60, SE_Code_Make(1, 4, 1, 4), 12, 3);
    se_req_frame_set(se_work_a, 24, 46, 14, 0x3000002);
    fn_80229EA8(se_work_a, 26, 7, 3);
    fn_800DA428(se_work_b, 128, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 130, 3);
    break;
case 345:
    se_req_frame_set(se_work_a, 100, 9, 12, 3);
    se_req_frame_set(se_work_a, 32, 46, 14, 0x3000002);
    fn_80229EA8(se_work_a, 68, 7, 3);
    fn_800DA428(se_work_b, 164, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 164, 3);
    break;
case 346:
    se_req_frame_set(se_work_a, 8, SE_Code_Make(0, 4, 1, 4), 12, 3);
    se_req_frame_set(se_work_a, 4, 28, 11, 0x3000002);
    fn_80229EA8(se_work_a, 12, 11, 3);
    break;
case 347:
    se_req_frame_set(se_work_a, 8, SE_Code_Make(0, 4, 1, 4), 12, 3);
    se_req_frame_set(se_work_a, 4, 28, 7, 0x3000002);
    fn_80229EA8(se_work_a, 12, 7, 3);
    break;
case 348:
    se_req_frame_set(se_work_a, 52, SE_Code_Make(7, 4, 8, 4), 12, 3);
    fn_80229EA8(se_work_a, 12, 11, 3);
    break;
case 349:
    se_req_frame_set(se_work_a, 4, 26, 7, 0x3000002);
    se_req_frame_set(se_work_a, 44, 13, 11, 0x3000002);
    break;
case 350:
    se_req_frame_set(se_work_a, 4, 26, 7, 0x3000002);
    se_req_frame_set(se_work_a, 44, 13, 7, 0x3000002);
    se_req_frame_set(se_work_a, 56, 46, 7, 0x3000002);
    break;
case 351:
    se_req_frame_set(se_work_a, 12, 234, 7, 0x3000003);
    se_req_frame_set(se_work_a, 72, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 102, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 186, 234, 3, 0x3000003);
    break;
case 352:
    se_req_frame_set(se_work_a, 8, 139, 10, 0x3000003);
    se_req_frame_set(se_work_a, 26, 141, 10, 0x3000003);
    se_req_frame_set(se_work_a, 28, 142, 11, 0x3000003);
    se_req_frame_set(se_work_a, 92, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 116, 237, 20, 0x3000003);
    break;
case 353:
    se_req_frame_set(se_work_a, 32, 144, 11, 0x3000003);
    break;
case 354:
    se_req_frame_set(se_work_a, 34, 234, 3, 0x3000003);
    break;
case 355:
    se_req_frame_set(se_work_a, 42, SE_Code_Make(29, 5, 30, 2), 12, 3);
    se_req_frame_set(se_work_a, 66, SE_Code_Make(29, 2, 30, 4), 12, 3);
    se_req_frame_set(se_work_a, 80, 234, 3, 0x3000003);
    break;
case 356:
    fn_80244E88((&self->field_0xAF4), ((u8)(v)), motion, self->field_0x002);
    break;
case 357:
    se_req_frame_set(se_work_a, 6, 234, 3, 0x3000003);
    se_req_frame_set(se_work_a, 28, 237, 11, 0x3000003);
    break;
case 358:
    se_req_frame_set(se_work_a, 20, 132, 6, 0x3000003);
    se_req_frame_set(se_work_a, 54, 133, 6, 0x3000003);
    se_req_frame_set(se_work_a, 76, 134, 6, 0x3000003);
    se_req_frame_set(se_work_a, 136, 134, 6, 0x3000003);
    se_req_frame_set(se_work_a, 220, 132, 6, 0x3000003);
    se_req_frame_set(se_work_a, 286, 146, 10, 0x3000003);
    se_req_frame_set(se_work_a, 280, 234, 3, 0x3000003);
    break;
case 359:
    se_req_frame_set(se_work_a, 18, SE_Code_Make(21, 2, 22, 2), 12, 3);
    se_req_frame_set(se_work_a, 8, 146, 10, 0x3000003);
    se_req_frame_set(se_work_a, 16, 140, 10, 0x3000003);
    se_req_frame_set(se_work_a, 56, 234, 3, 0x3000003);
    break;
case 360:
    se_req_frame_set(se_work_a, 28, SE_Code_Make(130, 3, 130, 3), 3, 0x3000003);
    se_req_frame_set(se_work_a, 226, SE_Code_Make(130, 3, 130, 3), 3, 0x3000003);
    break;
case 361:
    se_req_frame_set(se_work_a, 18, SE_Code_Make(21, 2, 22, 2), 12, 3);
    se_req_frame_set(se_work_a, 16, 139, 10, 0x3000003);
    se_req_frame_set(se_work_a, 46, 238, 17, 0x3000003);
    se_req_frame_set(se_work_a, 54, 238, 20, 0x3000003);
    break;
case 362:
    se_req_frame_set(se_work_a, 18, SE_Code_Make(21, 2, 22, 2), 12, 3);
    se_req_frame_set(se_work_a, 8, 139, 10, 0x3000003);
    se_req_frame_set(se_work_a, 44, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 60, 237, 20, 0x3000003);
    break;
case 363:
    se_req_frame_set(se_work_a, 16, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 184, 234, 11, 0x3000003);
    break;
case 364:
    se_req_frame_set(se_work_a, 4, 234, 11, 0x3000003);
    se_req_frame_set(se_work_a, 16, 238, 11, 0x3000003);
    break;
case 365:
    se_req_frame_set(se_work_a, 8, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 12, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 60, 234, 3, 0x3000003);
    se_req_frame_set(se_work_a, 94, 238, 20, 0x3000003);
    break;
case 401:
    se_req_frame_set(se_work_a, 26, 65, 11, 0x3000003);
    se_req_frame_set(se_work_a, 70, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 68, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 70, 3);
    break;
case 402:
    se_req_frame_set(se_work_a, 6, 60, 11, 0x3000003);
    se_req_frame_set(se_work_a, 20, 63, 11, 0x3000003);
    se_req_frame_set(se_work_a, 48, 28, 11, 0x3000003);
    fn_800DA428(se_work_b, 64, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 66, 3);
    break;
case 403:
    se_req_frame_set(se_work_a, 20, SE_Code_Make(0, 1, 1, 1), 12, 3);
    se_req_frame_set(se_work_a, 18, 61, 11, 0x3000003);
    fn_800DA428(se_work_b, 14, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 26, 3);
    break;
case 404:
    se_req_frame_set(se_work_a, 14, 60, 11, 0x3000003);
    fn_80229EA8(se_work_a, 18, 11, 3);
    break;
case 406:
    se_req_frame_set(se_work_a, 14, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 24, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 26, 3);
    break;
case 407:
    se_req_frame_set(se_work_a, 10, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 28, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 18, 3);
    break;
case 408:
    se_req_frame_set(se_work_a, 20, SE_Code_Make(0, 1, 1, 1), 12, 3);
    se_req_frame_set(se_work_a, 18, 61, 11, 0x3000003);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 34, 3);
    break;
case 409:
    se_req_frame_set(se_work_a, 14, 60, 11, 0x3000003);
    fn_800DA428(se_work_b, 12, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 18, 11, 3);
    break;
case 411:
    se_req_frame_set(se_work_a, 14, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 30, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 32, 3);
    break;
case 412:
    se_req_frame_set(se_work_a, 10, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 28, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 42, ((((u8)(v)) << 24) | 0x1), 20, 2);
    break;
case 413:
    se_req_frame_set(se_work_a, 56, SE_Code_Make(7, 1, 8, 1), 12, 3);
    fn_800DA428(se_work_b, 2, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 50, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 74, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_80229E10(se_work_a, 52, 3);
    fn_80229E10(se_work_a, 76, 3);
    break;
case 414:
    se_req_frame_set(se_work_a, 6, 64, 11, 0x3000003);
    fn_800DA428(se_work_b, 8, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_800DA428(se_work_b, 20, (((u8)(v)) << 24), 17, 2);
    fn_80229E10(se_work_a, 12, 3);
    break;
case 420:
    se_req_frame_set(se_work_a, 22, 13, 3, 0x3000003);
    se_req_frame_set(se_work_a, 24, 9, 3, 0x3000003);
    se_req_frame_set(se_work_a, 56, 15, 3, 0x3000003);
    se_req_frame_set(se_work_a, 92, 12, 3, 0x3000003);
    se_req_frame_set(se_work_a, 110, 9, 3, 0x3000003);
    se_req_frame_set(se_work_a, 148, 10, 3, 0x3000003);
    se_req_frame_set(se_work_a, 172, 14, 3, 0x3000003);
    se_req_frame_set(se_work_a, 188, 9, 3, 0x3000003);
    se_req_frame_set(se_work_a, 204, 10, 3, 0x3000003);
    fn_80229E10(se_work_a, 64, 3);
    fn_80229E10(se_work_a, 152, 3);
    fn_80229E10(se_work_a, 210, 3);
    fn_80229E10(se_work_a, 260, 3);
    break;
case 421:
    se_req_frame_set(se_work_a, 34, 10, 3, 0x3000003);
    se_req_frame_set(se_work_a, 36, 9, 3, 0x3000003);
    se_req_frame_set(se_work_a, 72, 13, 3, 0x3000003);
    se_req_frame_set(se_work_a, 112, 10, 3, 0x3000003);
    se_req_frame_set(se_work_a, 6, 8, 16, 0x3000003);
    se_req_frame_set(se_work_a, 70, 8, 16, 0x3000003);
    se_req_frame_set(se_work_a, 120, 9, 3, 0x3000003);
    se_req_frame_set(se_work_a, 148, 8, 16, 0x3000003);
    se_req_frame_set(se_work_a, 186, 9, 19, 0x3000003);
    fn_80229E10(se_work_a, 24, 3);
    fn_80229E10(se_work_a, 90, 3);
    fn_80229E10(se_work_a, 140, 3);
    fn_80229E10(se_work_a, 224, 3);
    break;
case 422:
    se_req_frame_set(se_work_a, 44, SE_Code_Make(3, 1, 3, 1), 12, 3);
    se_req_frame_set(se_work_a, 6, 35, 11, 0x3000003);
    se_req_frame_set(se_work_a, 42, 28, 11, 0x3000003);
    se_req_frame_set(se_work_a, 38, 8, 16, 0x3000003);
    se_req_frame_set(se_work_a, 4, 8, 16, 0x3000003);
    fn_80229E10(se_work_a, 8, 3);
    fn_80229E10(se_work_a, 48, 3);
    break;
case 423:
    se_req_frame_set(se_work_a, 112, SE_Code_Make(3, 2, 3, 2), 12, 3);
    se_req_frame_set(se_work_a, 80, 46, 16, 0x3000003);
    se_req_frame_set(se_work_a, 6, 35, 7, 0x3000003);
    se_req_frame_set(se_work_a, 42, 35, 11, 0x3000003);
    se_req_frame_set(se_work_a, 130, 18, 16, 0x3000003);
    fn_800DA428(se_work_b, 126, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 144, (((u8)(v)) << 24), 17, 2);
    fn_80229EA8(se_work_a, 24, 7, 3);
    fn_80229EA8(se_work_a, 94, 7, 3);
    fn_80229E10(se_work_a, 128, 3);
    fn_80229E10(se_work_a, 146, 3);
    break;
case 500:
    fn_800DA428(se_work_b, 16, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 42, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 42, 3);
    fn_80229EA8(se_work_a, 62, 7, 3);
    break;
case 501:
    fn_800DA428(se_work_b, 114, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 52, 3);
    fn_80229EA8(se_work_a, 46, 7, 3);
    se_req_frame_set(se_work_a, 10, 17, 20, 0x3000003);
    break;
case 502:
    se_req_frame_set(se_work_a, 78, SE_Code_Make(2, 4, 2, 4), 12, 3);
    fn_800DA428(se_work_b, 22, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 142, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x3), 20, 2);
    fn_80229EA8(se_work_a, 34, 7, 3);
    fn_80229E10(se_work_a, 24, 3);
    fn_80229E10(se_work_a, 70, 3);
    se_req_frame_set(se_work_a, 80, 39, 7, 0x3000003);
    break;
case 503:
    se_req_frame_set(se_work_a, 48, 9, 12, 3);
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 28, 3);
    fn_80229EA8(se_work_a, 82, 7, 3);
    se_req_frame_set(se_work_a, 64, 13, 7, 0x3000003);
    se_req_frame_set(se_work_a, 80, 10, 20, 0x3000003);
    break;
case 504:
    se_req_frame_set(se_work_a, 4, 17, 20, 0x3000003);
    break;
case 505:
    se_req_frame_set(se_work_a, 52, SE_Code_Make(3, 1, 3, 1), 12, 3);
    fn_800DA428(se_work_b, 28, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 56, ((((u8)(v)) << 24) | 0x1), 20, 2);
    se_req_frame_set(se_work_a, 28, 13, 20, 0x3000003);
    se_req_frame_set(se_work_a, 56, 13, 20, 0x3000003);
    fn_80229E10(se_work_a, 32, 3);
    fn_80229E10(se_work_a, 58, 3);
    break;
case 506:
    fn_80229E10(se_work_a, 18, 3);
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x1), 20, 2);
    se_req_frame_set(se_work_a, 30, 80, 7, 0x3000003);
    se_req_frame_set(se_work_a, 42, 80, 11, 0x3000003);
    break;
case 507:
    fn_800DA428(se_work_b, 20, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 44, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 174, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 12, 3);
    fn_80229E10(se_work_a, 46, 3);
    fn_80229E10(se_work_a, 176, 3);
    fn_80229E10(se_work_a, 292, 3);
    fn_80229EA8(se_work_a, 40, 11, 3);
    fn_80229EA8(se_work_a, 70, 7, 3);
    fn_80229EA8(se_work_a, 176, 11, 3);
    fn_80229EA8(se_work_a, 114, 11, 3);
    fn_80229EA8(se_work_a, 168, 7, 3);
    fn_80229EA8(se_work_a, 230, 11, 3);
    fn_80229EA8(se_work_a, 246, 7, 3);
    se_req_frame_set(se_work_a, 38, 39, 11, 0x3000003);
    se_req_frame_set(se_work_a, 54, 39, 7, 0x3000003);
    se_req_frame_set(se_work_a, 176, 39, 7, 0x3000003);
    break;
case 508:
    fn_800DA428(se_work_b, 20, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 68, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 126, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 148, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 180, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 208, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 246, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 272, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 312, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 86, 3);
    fn_80229E10(se_work_a, 150, 3);
    fn_80229E10(se_work_a, 216, 3);
    fn_80229E10(se_work_a, 266, 3);
    fn_80229EA8(se_work_a, 138, 11, 3);
    fn_80229EA8(se_work_a, 204, 7, 3);
    fn_80229EA8(se_work_a, 248, 11, 3);
    fn_80229EA8(se_work_a, 312, 7, 3);
    break;
case 509:
    fn_800DA428(se_work_b, 36, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 194, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 38, 3);
    fn_80229E10(se_work_a, 196, 3);
    break;
case 510:
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 34, 3);
    fn_80229EA8(se_work_a, 46, 7, 3);
    fn_80229EA8(se_work_a, 70, 7, 3);
    fn_80229EA8(se_work_a, 118, 7, 3);
    fn_80229EA8(se_work_a, 134, 7, 3);
    break;
case 511:
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 140, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 28, 3);
    fn_80229E10(se_work_a, 142, 3);
    se_req_frame_set(se_work_a, 62, 16, 20, 0x3000003);
    fn_80229EA8(se_work_a, 80, 7, 3);
    fn_80229EA8(se_work_a, 154, 7, 3);
    se_req_frame_set(se_work_a, 78, 39, 7, 0x3000003);
    break;
case 512:
    fn_800DA428(se_work_b, 12, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 22, 7, 3);
    break;
case 513:
    se_req_frame_set(se_work_a, 34, 9, 12, 3);
    fn_80229EA8(se_work_a, 22, 7, 3);
    se_req_frame_set(se_work_a, 54, 10, 14, 0x3000003);
    se_req_frame_set(se_work_a, 104, 13, 20, 0x3000003);
    fn_80229E10(se_work_a, 118, 3);
    fn_80229E10(se_work_a, 182, 3);
    fn_80229E10(se_work_a, 224, 3);
    fn_80229E10(se_work_a, 270, 3);
    fn_80229E10(se_work_a, 322, 3);
    fn_80229E10(se_work_a, 366, 3);
    fn_80229E10(se_work_a, 406, 3);
    break;
case 514:
    fn_800DA428(se_work_b, 32, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 68, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 114, (((u8)(v)) << 24), 17, 2);
    se_req_frame_set(se_work_a, 52, 17, 14, 0x3000003);
    se_req_frame_set(se_work_a, 82, 28, 14, 0x3000003);
    fn_80229E10(se_work_a, 46, 3);
    fn_80229E10(se_work_a, 116, 3);
    fn_80229EA8(se_work_a, 56, 7, 3);
    break;
case 515:
    se_req_frame_set(se_work_a, 32, 80, 7, 0x3000003);
    se_req_frame_set(se_work_a, 50, 80, 11, 0x3000003);
    se_req_frame_set(se_work_a, 68, 80, 7, 0x3000003);
    se_req_frame_set(se_work_a, 86, 80, 11, 0x3000003);
    se_req_frame_set(se_work_a, 130, 39, 7, 0x3000003);
    se_req_frame_set(se_work_a, 176, 39, 7, 0x3000003);
    se_req_frame_set(se_work_a, 228, 39, 7, 0x3000003);
    se_req_frame_set(se_work_a, 300, 16, 20, 0x3000003);
    se_req_frame_set(se_work_a, 316, 13, 14, 0x3000003);
    fn_800DA428(se_work_b, 24, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 120, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 132, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 282, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 306, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 24, 3);
    fn_80229E10(se_work_a, 120, 3);
    fn_80229E10(se_work_a, 130, 3);
    fn_80229E10(se_work_a, 176, 3);
    fn_80229E10(se_work_a, 228, 3);
    fn_80229EA8(se_work_a, 362, 7, 3);
    fn_80229EA8(se_work_a, 386, 7, 3);
    fn_80229EA8(se_work_a, 436, 7, 3);
    break;
case 516:
    fn_800DA428(se_work_b, 18, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 106, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 190, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 462, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 514, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 146, 7, 3);
    fn_80229EA8(se_work_a, 210, 7, 3);
    fn_80229EA8(se_work_a, 296, 7, 3);
    fn_80229EA8(se_work_a, 396, 7, 3);
    fn_80229E10(se_work_a, 22, 3);
    fn_80229E10(se_work_a, 108, 3);
    fn_80229E10(se_work_a, 194, 3);
    fn_80229E10(se_work_a, 464, 3);
    fn_80229E10(se_work_a, 514, 3);
    break;
case 517:
    fn_800DA428(se_work_b, 30, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 58, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 92, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 120, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 148, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 176, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229EA8(se_work_a, 46, 7, 3);
    fn_80229EA8(se_work_a, 146, 7, 3);
    fn_80229E10(se_work_a, 32, 3);
    fn_80229E10(se_work_a, 94, 3);
    fn_80229E10(se_work_a, 150, 3);
    break;
case 518:
    se_req_frame_set(se_work_a, 16, 18, 14, 0x3000003);
    fn_800DA428(se_work_b, 26, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 28, 3);
    break;
case 520:
    se_req_frame_set(se_work_a, 26, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 48, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 74, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 98, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 124, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 148, 237, 7, 0x3000003);
    break;
case 521:
    se_req_frame_set(se_work_a, 26, 237, 7, 0x3000003);
    break;
case 522:
    se_req_frame_set(se_work_a, 78, SE_Code_Make(22, 4, 22, 4), 12, 3);
    se_req_frame_set(se_work_a, 22, 234, 20, 0x3000003);
    se_req_frame_set(se_work_a, 82, 139, 7, 0x3000003);
    break;
case 523:
    se_req_frame_set(se_work_a, 96, 237, 7, 0x3000003);
    break;
case 524:
    se_req_frame_set(se_work_a, 12, 245, 3, 0x3000003);
    break;
case 525:
    se_req_frame_set(se_work_a, 24, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 60, 238, 20, 0x3000003);
    se_req_frame_set(se_work_a, 90, 238, 17, 0x3000003);
    break;
case 526:
    se_req_frame_set(se_work_a, 10, 237, 7, 0x3000003);
    se_req_frame_set(se_work_a, 26, 180, 7, 0x3000003);
    se_req_frame_set(se_work_a, 42, 180, 11, 0x3000003);
    se_req_frame_set(se_work_a, 58, 180, 11, 0x3000003);
    se_req_frame_set(se_work_a, 74, 180, 11, 0x3000003);
    se_req_frame_set(se_work_a, 90, 180, 11, 0x3000003);
    se_req_frame_set(se_work_a, 106, 180, 11, 0x3000003);
    se_req_frame_set(se_work_a, 122, 180, 11, 0x3000003);
    break;
case 527:
    se_req_frame_set(se_work_a, 42, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 88, 238, 13, 0x3000003);
    se_req_frame_set(se_work_a, 88, 238, 13, 0x3000003);
    se_req_frame_set(se_work_a, 178, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 226, 238, 11, 0x3000003);
    break;
case 529:
    se_req_frame_set(se_work_a, 12, 237, 11, 0x3000003);
    break;
case 530:
    se_req_frame_set(se_work_a, 12, 237, 17, 0x3000003);
    break;
case 531:
    se_req_frame_set(se_work_a, 12, 238, 7, 0x3000003);
    se_req_frame_set(se_work_a, 74, 139, 7, 0x3000003);
    se_req_frame_set(se_work_a, 26, 238, 11, 0x3000003);
    se_req_frame_set(se_work_a, 120, 238, 11, 0x3000003);
    break;
case 532:
    se_req_frame_set(se_work_a, 12, 237, 11, 0x3000003);
    break;
case 533:
    se_req_frame_set(se_work_a, 22, 234, 14, 0x3000003);
    se_req_frame_set(se_work_a, 212, 237, 11, 0x3000003);
    se_req_frame_set(se_work_a, 246, 237, 17, 0x3000003);
    se_req_frame_set(se_work_a, 300, 237, 11, 0x3000003);
    se_req_frame_set(se_work_a, 336, 237, 17, 0x3000003);
    break;
case 534:
    se_req_frame_set(se_work_a, 8, 238, 17, 0x3000003);
    break;
case 600:
    se_req_frame_set(se_work_a, 4, 16, 17, 0x3000003);
    se_req_frame_set(se_work_a, 158, 16, 20, 0x3000003);
    fn_800DA428(se_work_b, 16, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 46, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 74, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 98, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 144, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 178, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 16, 3);
    fn_80229E10(se_work_a, 46, 3);
    fn_80229E10(se_work_a, 144, 3);
    fn_80229E10(se_work_a, 178, 3);
    break;
case 603:
    fn_800DA428(se_work_b, 16, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_800DA428(se_work_b, 48, ((((u8)(v)) << 24) | 0x1), 20, 2);
    se_req_frame_set(se_work_a, 34, 80, 7, 0x3000003);
    se_req_frame_set(se_work_a, 48, 80, 7, 0x3000003);
    se_req_frame_set(se_work_a, 62, 80, 7, 0x3000003);
    fn_80229E10(se_work_a, 120, 3);
    break;
case 604:
    se_req_frame_set(se_work_a, 12, 17, 17, 0x3000003);
    break;
case 614:
    se_req_frame_set(se_work_a, 38, 16, 12, 0x3000003);
    fn_80229EA8(se_work_a, 62, 7, 3);
    break;
case 615:
    se_req_frame_set(se_work_a, 42, 46, 12, 0x3000003);
    fn_80229EA8(se_work_a, 70, 11, 3);
    fn_80229EA8(se_work_a, 154, 11, 3);
    fn_80229EA8(se_work_a, 220, 11, 3);
    break;
case 617:
    se_req_frame_set(se_work_a, 46, 16, 14, 0x3000003);
    fn_800DA428(se_work_b, 62, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_800DA428(se_work_b, 102, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 140, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 176, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 104, 3);
    fn_80229E10(se_work_a, 142, 3);
    fn_80229E10(se_work_a, 176, 3);
    break;
case 619:
    fn_800DA428(se_work_b, 40, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 54, ((((u8)(v)) << 24) | 0x1), 20, 2);
    fn_80229E10(se_work_a, 40, 3);
    break;
case 620:
    fn_80229EA8(se_work_a, 24, 11, 3);
    break;
case 621:
    fn_80229EA8(se_work_a, 28, 11, 3);
    fn_80229EA8(se_work_a, 82, 11, 3);
    break;
case 623:
    se_req_frame_set(se_work_a, 26, SE_Code_Make(5, 4, 6, 4), 12, 3);
    se_req_frame_set(se_work_a, 22, 28, 14, 0x3000003);
    se_req_frame_set(se_work_a, 22, 46, 14, 0x3000003);
    se_req_frame_set(se_work_a, 150, 13, 19, 0x3000003);
    fn_800DA428(se_work_b, 26, (((u8)(v)) << 24), 17, 2);
    fn_800DA428(se_work_b, 56, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 68, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 80, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 90, ((((u8)(v)) << 24) | 0x2), 17, 2);
    fn_800DA428(se_work_b, 100, ((((u8)(v)) << 24) | 0x2), 20, 2);
    fn_800DA428(se_work_b, 126, ((((u8)(v)) << 24) | 0x4), 17, 2);
    fn_80229E10(se_work_a, 26, 3);
    fn_80229E10(se_work_a, 56, 3);
    fn_80229E10(se_work_a, 68, 3);
    fn_80229E10(se_work_a, 80, 3);
    fn_80229E10(se_work_a, 90, 3);
    fn_80229E10(se_work_a, 100, 3);
    fn_80229E10(se_work_a, 110, 3);
    fn_80229E10(se_work_a, 126, 3);
    break;
case 627:
    se_req_frame_set(se_work_a, 70, 46, 11, 0x3000003);
    se_req_frame_set(se_work_a, 72, 10, 14, 0x3000003);
    se_req_frame_set(se_work_a, 52, 18, 20, 0x3000003);
    se_req_frame_set(se_work_a, 120, 13, 20, 0x3000003);
    se_req_frame_set(se_work_a, 318, 17, 20, 0x3000003);
    fn_800DA428(se_work_b, 26, (((u8)(v)) << 24), 20, 2);
    fn_80229E10(se_work_a, 28, 3);
    fn_80229E10(se_work_a, 86, 3);
    fn_80229E10(se_work_a, 116, 3);
    fn_80229E10(se_work_a, 346, 3);
    fn_80229E10(se_work_a, 476, 3);
    fn_80229EA8(se_work_a, 108, 7, 3);
    fn_80229EA8(se_work_a, 596, 7, 3);
    break;
case 628:
    se_req_frame_set(se_work_a, 38, 18, 14, 0x3000003);
    fn_800DA428(se_work_b, 12, (((u8)(v)) << 24), 20, 2);
    fn_800DA428(se_work_b, 40, ((((u8)(v)) << 24) | 0x1), 17, 2);
    fn_80229E10(se_work_a, 34, 3);
    break;
case 634:
    se_req_frame_set(se_work_a, 4, SE_Code_Make(6, 4, 5, 4), 12, 3);
    se_req_frame_set(se_work_a, 4, 41, 17, 0x3000003);
    se_req_frame_set(se_work_a, 46, 12, 14, 0x3000003);
    se_req_frame_set(se_work_a, 76, 14, 14, 0x3000003);
    se_req_frame_set(se_work_a, 210, 17, 11, 0x3000003);
    se_req_frame_set(se_work_a, 270, 16, 11, 0x3000003);
    se_req_frame_set(se_work_a, 306, 13, 11, 0x3000003);
    se_req_frame_set(se_work_a, 340, 13, 11, 0x3000003);
    fn_80229E10(se_work_a, 78, 3);
    fn_80229E10(se_work_a, 270, 3);
    fn_80229EA8(se_work_a, 308, 11, 3);
    fn_80229EA8(se_work_a, 344, 11, 3);
    }
}
