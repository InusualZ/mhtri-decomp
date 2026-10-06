/*
 * Pl/fn_8023C2D0.cpp - the player motion -> SE frame dispatch for motions 0x3EA-0x4F7 (`Pl/fn_80241558.cpp` is the
 *   0x3EA-0x4EE sibling); `fn_8023C2D0` gates two motion groups on `Pl_act_ck`.
 * RANGE. .text 0x8023C2D0-0x80241558 (2 functions); .data 0x805C34D4-0x805C3D20 (`fn_8023C2D0`'s 270-entry
 *   `jumptable_805C34D4` and `fn_8023FC20`'s 261-entry `jumptable_805C390C`), extab, extabindex.  The `.data` claim
 *   starts at the 4-mod-8 0x805C34D4: `tools/elf/objalign.py` lowers the object's section alignment so it links in
 *   place (playbook 55).
 * NAMES. The functions keep the map's stems (no `__FILE__` string covers the band; the runtime dump answers a
 *   placeholder).
 * RESIDUALS. none.
 * SHAPES. The three `_se_w` locals are declared in a different order than they are assigned: the only shape that gives
 *   retail's r30/r28/r29 colouring together with its load order.
 *  - the two arms that OR `part << 0x18` (0x450, 0x4C7) spell the shift in each argument instead of through `partHi`,
 *    or MWCC colours their two temporaries the other way round;
 *  - both functions' last arm falls through into `default:`, where the shared epilogue lives (retail keeps no branch).
 */

#include "types.h"
#include "sound/se.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/Pl.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"

/* Arms the frame-timed sound requests for every motion the player can be in above 0x3EA, with the
 * two motion groups whose extra frames depend on the active action gated on `Pl_act_ck`. */
extern "C" void fn_8023C2D0(_PLW* work, u8 part) {
    /* `part` shifted into the top byte once, the shape the shifted arguments need. */
    u32 partHi;
    /* The three per-part `_se_w` work objects; the declaration order and the assignment order are
     * deliberately different (the `Pl/fn_80241558.cpp` shape) - it is what reproduces retail's
     * colouring (r30 = +0xAF4, r28 = +0xAF8, r29 = +0xAFC) together with retail's load order. */
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 0x3EA:
        se_req_frame_set(seWorkMain, 0xA, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x14, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x18, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3A, 0x5, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x34, ((part << 0x18) | 0x4), 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x36, 0x3);
        return;

    case 0x3EB:
        se_req_frame_set(seWorkMain, 0x6, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2A, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x38, 0x6, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        fn_800DA428(seWorkFrame, 0x52, ((part << 0x18) | 0x4), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x54, 0x3);
        return;

    case 0x3EC:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0x7, 0x1, 0x8, 0x1), 0xC, 0x3);
        partHi = ((part << 0x18) | 0x2);
        fn_800DA428(seWorkFrame, 0x10, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x2E, partHi, 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x12, 0x3);
        fn_80229E10(seWorkPart, 0x32, 0x3);
        return;

    case 0x3F1:
        se_req_frame_set(seWorkMain, 0x6, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x16, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2A, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3A, 0x6, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x26, partHi, 0x11, 0x2);
        fn_800DA428(seWorkFrame, 0x4C, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x62, (partHi | 0x1), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x4E, 0x3);
        return;

    case 0x3F4:
        partHi = ((part << 0x18) | 0x2);
        fn_800DA428(seWorkFrame, 0x14, partHi, 0x11, 0x2);
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x14, 0x2);
        se_req_frame_set(seWorkPart, 0x6, 0x28, 0xC, 0x03000003);
        se_req_frame_set(seWorkMain, 0x16, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        fn_80229E10(seWorkPart, 0x22, 0x3);
        return;

    case 0x3F5:
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x14, (partHi | 0x2), 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x11, 0x2);
        se_req_frame_set(seWorkPart, 0x6, 0x28, 0xC, 0x03000003);
        se_req_frame_set(seWorkMain, 0x16, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        fn_80229E10(seWorkPart, 0x22, 0x3);
        return;

    case 0x400:
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x11, 0x2);
        fn_800DA428(seWorkFrame, 0x3E, partHi, 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x1C, 0x3);
        fn_80229E10(seWorkPart, 0x40, 0x3);
        return;

    case 0x41B:
        se_req_frame_set(seWorkMain, 0xA, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x14, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x18, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3A, 0x19, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x18, 0x6, 0xB, 0x3);
        return;

    case 0x41C:
        se_req_frame_set(seWorkMain, 0x6, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x38, 0x1A, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x32, 0xEA, 0x3, 0x03000003);
        return;

    case 0x41D:
        se_req_frame_set(seWorkPart, 0x2, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x2A, 0xEE, 0x14, 0x03000003);
        return;

    case 0x421:
        se_req_frame_set(seWorkMain, 0x6, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3A, 0x1A, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x32, 0xEA, 0x3, 0x03000003);
        return;

    case 0x424:
        se_req_frame_set(seWorkPart, 0x8, 0xF5, 0x3, 0x03000003);
        return;

    case 0x425:
        se_req_frame_set(seWorkPart, 0x8, 0xF5, 0x3, 0x03000003);
        return;

    case 0x426:
        se_req_frame_set(seWorkPart, 0x8, 0xF5, 0x3, 0x03000003);
        return;

    case 0x428:
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x36, 0xED, 0x14, 0x03000003);
        return;

    case 0x429:
        se_req_frame_set(seWorkPart, 0x8, 0xEA, 0x3, 0x03000003);
        return;

    case 0x42A:
        se_req_frame_set(seWorkPart, 0x8, 0xEA, 0x3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x22, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x38, 0xED, 0x14, 0x03000003);
        return;

    case 0x42B:
        se_req_frame_set(seWorkPart, 0x8, 0xEA, 0x3, 0x03000003);
        return;

    case 0x42C:
        se_req_frame_set(seWorkPart, 0x6, 0xF5, 0x3, 0x03000003);
        return;

    case 0x42D:
        se_req_frame_set(seWorkPart, 0x6, 0xF5, 0x3, 0x03000003);
        return;

    case 0x44D:
        se_req_frame_set(seWorkPart, 0x38, SE_Code_Make(0x0, 0x4, 0x1, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x12, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2E, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x8A, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x38, (partHi | 0x4), 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0xA8, (partHi | 0x1), 0x11, 0x2);
        se_req_frame_set(seWorkPart, 0x44, 0x11, 0x14, 0x03000003);
        fn_80229EA8(seWorkPart, 0x52, 0x7, 0x3);
        fn_80229E10(seWorkPart, 0xAA, 0x3);
        return;

    case 0x44E:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x0, 0x1, 0x1, 0x1), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x20, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x5A, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x1E, (partHi | 0x4), 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x72, (partHi | 0x1), 0x11, 0x2);
        fn_80229EA8(seWorkPart, 0x44, 0x7, 0x3);
        fn_80229E10(seWorkPart, 0x74, 0x3);
        return;

    case 0x44F:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(0x2, 0x3, 0x2, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x6, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x38, 0xF, 0x11, 0x03000003);
        se_req_frame_set(seWorkMain, 0x6A, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x84, 0xB, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x92, 0xC, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x36, ((part << 0x18) | 0x4), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x3A, 0x3);
        fn_80229EA8(seWorkPart, 0x52, 0x7, 0x3);
        return;

    case 0x450:
        se_req_frame_set(seWorkPart, 0x1C, SE_Code_Make(0x3, 0x4, 0x3, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x58, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        fn_800DA428(seWorkFrame, 0x8, (part << 0x18) | 0x3, 0x11, 0x2);
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18) | 0x3, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x72, (part << 0x18) | 0x1, 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x2A, 0x3);
        fn_80229EA8(seWorkPart, 0x30, 0x7, 0x3);
        fn_80229E10(seWorkPart, 0x74, 0x3);
        return;

    case 0x451:
        se_req_frame_set(seWorkPart, 0x1E, SE_Code_Make(0x0, 0x6, 0x1, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkPart, 0x46, SE_Code_Make(0x0, 0x2, 0x1, 0x6), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xC, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x8, 0x11, 0x14, 0x03000003);
        se_req_frame_set(seWorkMain, 0x3A, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x64, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_80229EA8(seWorkPart, 0x2C, 0x7, 0x3);
        fn_80229EA8(seWorkPart, 0x5E, 0x7, 0x3);
        fn_80229E10(seWorkPart, 0x3E, 0x3);
        fn_80229E10(seWorkPart, 0x4A, 0x3);
        return;

    case 0x452:
        se_req_frame_set(seWorkMain, 0x8, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x20, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x34, ((part << 0x18) | 0x1), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x36, 0x3);
        return;

    case 0x453:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x1C, (part << 0x18), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x26, 0x3);
        return;

    case 0x455:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1E, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x18, 0x11, 0x14, 0x03000003);
        return;

    case 0x456:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x0, 0x1, 0x1, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xA, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x20, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x1E, ((part << 0x18) | 0x4), 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x2C, 0x3);
        return;

    case 0x457:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x14, 0x11, 0x14, 0x03000003);
        se_req_frame_set(seWorkMain, 0x28, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x458:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x0, 0x3, 0x0, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x6, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xC, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x5, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0xA, 0xA, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x1E, ((part << 0x18) | 0x4), 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x22, 0x3);
        return;

    case 0x459:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x26, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0xA, ((part << 0x18) | 0x4), 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x18, 0x3);
        return;

    case 0x45A:
        se_req_frame_set(seWorkPart, 0x8, 0x1C, 0x14, 0x03000003);
        se_req_frame_set(seWorkMain, 0x36, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x54, 0x3, 0xB, 0x3);
        fn_80229EA8(seWorkPart, 0x1E, 0x7, 0x3);
        return;

    case 0x460:
        se_req_frame_set(seWorkPart, 0x22, SE_Code_Make(0x0, 0x3, 0x1, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x50, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x28, (partHi | 0x4), 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x6E, (partHi | 0x1), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x42, 0x3);
        se_req_frame_set(seWorkPart, 0x4C, 0x11, 0x14, 0x03000003);
        return;

    case 0x461:
        se_req_frame_set(seWorkPart, 0x30, SE_Code_Make(0x0, 0x1, 0x1, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xC, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2E, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x5C, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x52, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x70, (partHi | 0x1), 0x11, 0x2);
        fn_80229E10(seWorkPart, 0x78, 0x3);
        return;

    case 0x462:
        se_req_frame_set(seWorkPart, 0x4A, SE_Code_Make(0x2, 0x4, 0x2, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkPart, 0x6E, SE_Code_Make(0x3, 0x3, 0x3, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x14, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x1A, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x22, 0xE, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x4E, 0x7, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x4A, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x6E, 0x7, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x78, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x92, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA8, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        partHi = ((part << 0x18) | 0x4);
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x4C, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0xAC, partHi, 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x1A, 0x3);
        fn_80229E10(seWorkPart, 0x5C, 0x3);
        return;

    case 0x463:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        fn_80229E10(seWorkPart, 0x78, 0x3);
        return;

    case 0x465:
        se_req_frame_set(seWorkMain, 0x4, 0x1, 0x3, 0x4);
        se_req_frame_set(seWorkPart, 0x4, 0x39, 0xB, 0x03000003);
        se_req_frame_set(seWorkMain, 0x2A, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x40, 0x2, 0xB, 0x2);
        se_req_frame_set(seWorkMain, 0x42, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x52, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x68, 0x5, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x86, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA8, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xAC, 0xC, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        partHi = (part << 0x18);
        fn_800DA428(seWorkFrame, 0x28, (partHi | 0x4), 0x14, 0x2);
        se_req_frame_set(seWorkPart, 0x36, 0x8, 0x11, 0x03000003);
        fn_800DA428(seWorkFrame, 0x9E, partHi, 0x14, 0x2);
        return;

    case 0x466:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x467:
        se_req_frame_set(seWorkPart, 0x1E, SE_Code_Make(0x0, 0x2, 0x1, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x20, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x468:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x469:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3E, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x10, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        partHi = ((part << 0x18) | 0x4);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x62, partHi, 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x2E, 0x3);
        fn_80229E10(seWorkPart, 0x6E, 0x3);
        return;

    case 0x46A:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0x0, 0x2, 0x1, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xA, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x46B:
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xC, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x46C:
        se_req_frame_set(seWorkPart, 0x22, SE_Code_Make(0x0, 0x3, 0x1, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x474:
        se_req_frame_set(seWorkMain, 0x6, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xE, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x16, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x1E, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x34, 0x7, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2A, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x3A, ((part << 0x18) | 0x4), 0x14, 0x2);
        return;

    case 0x475:
        se_req_frame_set(seWorkMain, 0xC, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x10, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2C, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x5A, 0xC, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        if (Pl_act_ck(work, 0x4, 0x46) == 0) {
            se_req_frame_set(seWorkMain, 0x8, 0x2, 0xB, 0x2);
            se_req_frame_set(seWorkMain, 0x44, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
            se_req_frame_set(seWorkMain, 0x48, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
            se_req_frame_set(seWorkMain, 0x5A, 0x5, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        }
        return;

    case 0x476:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(0x2, 0x4, 0x2, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x10, 0x4, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x12, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x1C, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2A, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x2A, 0x8, 0x11, 0x03000003);
        return;

    case 0x477:
        se_req_frame_set(seWorkPart, 0x28, SE_Code_Make(0x1, 0x4, 0x0, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x10, 0x2, 0xB, 0x2);
        se_req_frame_set(seWorkMain, 0x14, 0x1, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x18, 0x4, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1A, 0x3, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x8, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x22, 0x5, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x9, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        fn_800DA428(seWorkFrame, 0x28, ((part << 0x18) | 0x4), 0x14, 0x2);
        fn_80229E10(seWorkPart, 0x2C, 0x3);
        return;

    case 0x47F:
        se_req_frame_set(seWorkPart, 0x38, SE_Code_Make(0x14, 0x3, 0x15, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x12, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2C, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x8A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x8E, 0xED, 0x14, 0x03000003);
        return;

    case 0x480:
        se_req_frame_set(seWorkPart, 0x20, SE_Code_Make(0x14, 0x1, 0x15, 0x1), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2A, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2A, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x5A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x56, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x6C, 0xEE, 0x14, 0x03000003);
        return;

    case 0x481:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(0x16, 0x3, 0x16, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x6, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x16, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x6A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x84, 0x1F, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x92, 0x20, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x3E, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x48, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x78, 0xEA, 0x3, 0x03000003);
        return;

    case 0x482:
        se_req_frame_set(seWorkPart, 0x1C, SE_Code_Make(0x17, 0x4, 0x17, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1A, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x58, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x483:
        se_req_frame_set(seWorkPart, 0x1E, SE_Code_Make(0x14, 0x6, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkPart, 0x46, SE_Code_Make(0x14, 0x2, 0x15, 0x6), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0xC, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x3A, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x3C, 0x7, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x62, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x64, 0x7, 0xB, 0x4);
        return;

    case 0x484:
        se_req_frame_set(seWorkMain, 0x8, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x20, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkPart, 0x32, 0xEE, 0x11, 0x03000003);
        return;

    case 0x485:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x486:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x14, 0x2, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xA, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x22, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x22, 0x7, 0xB, 0x4);
        return;

    case 0x487:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1E, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1E, 0x7, 0xB, 0x4);
        return;

    case 0x488:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x14, 0x2, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xA, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x22, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x22, 0x7, 0xB, 0x4);
        return;

    case 0x489:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x7, 0xB, 0x4);
        return;

    case 0x48A:
        se_req_frame_set(seWorkPart, 0x24, SE_Code_Make(0x14, 0x2, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x6, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xC, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x19, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0xA, 0x1E, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x48B:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x26, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x26, 0x7, 0xB, 0x4);
        return;

    case 0x48C:
        se_req_frame_set(seWorkPart, 0x8, 0xEA, 0x3, 0x03000003);
        se_req_frame_set(seWorkMain, 0x36, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x54, 0x9, 0xB, 0x3);
        se_req_frame_set(seWorkPart, 0x66, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x7E, 0xED, 0x14, 0x03000003);
        return;

    case 0x492:
        se_req_frame_set(seWorkPart, 0x22, SE_Code_Make(0x14, 0x3, 0x15, 0x3), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x26, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x26, 0x8, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x50, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x493:
        se_req_frame_set(seWorkMain, 0xC, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2E, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2E, 0x8, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x5C, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x64, 0xEE, 0x11, 0x03000003);
        return;

    case 0x494:
        se_req_frame_set(seWorkPart, 0x4A, SE_Code_Make(0x16, 0x4, 0x16, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkPart, 0x6E, SE_Code_Make(0x17, 0x4, 0x17, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkPart, 0x8, 0xEA, 0x3, 0x03000003);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x14, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x1A, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x22, 0xF, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x46, 0x8, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x4E, 0x1B, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x4A, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x6C, 0x8, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x78, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x92, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xB2, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0xAE, 0x7, 0xB, 0x4);
        return;

    case 0x495:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x497:
        se_req_frame_set(seWorkMain, 0x4, 0x1, 0x3, 0x4);
        se_req_frame_set(seWorkPart, 0x4, 0x9D, 0xB, 0x03000003);
        se_req_frame_set(seWorkMain, 0x2A, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2A, 0x8, 0xB, 0x4);
        se_req_frame_set(seWorkMain, 0x40, 0x2, 0xB, 0x2);
        se_req_frame_set(seWorkPart, 0x40, 0xFA, 0xB, 0x03000002);
        se_req_frame_set(seWorkMain, 0x42, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x52, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x68, 0x19, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x86, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xA8, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xAC, 0x20, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x498:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x499:
        se_req_frame_set(seWorkPart, 0x1E, SE_Code_Make(0x14, 0x2, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x20, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x20, 0x8, 0xB, 0x4);
        return;

    case 0x49A:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        return;

    case 0x49B:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x3E, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x10, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x10, 0x8, 0xB, 0x4);
        return;

    case 0x49C:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(0x14, 0x2, 0x15, 0x2), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0xA, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x16, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x16, 0x8, 0xB, 0x4);
        return;

    case 0x49D:
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0xC, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0xC, 0x8, 0xB, 0x4);
        return;

    case 0x49E:
        se_req_frame_set(seWorkPart, 0x22, SE_Code_Make(0x16, 0x4, 0x16, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x8, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x8, 0xB, 0x4);
        return;

    case 0x4A6:
        se_req_frame_set(seWorkMain, 0x6, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0xE, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x16, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x1E, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x34, 0x1B, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x20, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        return;

    case 0x4A7:
        se_req_frame_set(seWorkMain, 0xC, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x10, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x2A, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x56, 0x20, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        if (Pl_act_ck(work, 0x4, 0x47) == 0) {
            se_req_frame_set(seWorkMain, 0x42, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
            se_req_frame_set(seWorkMain, 0x46, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
            se_req_frame_set(seWorkMain, 0x56, 0x19, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
            se_req_frame_set(seWorkMain, 0x8, 0x2, 0x3, 0x2);
            se_req_frame_set(seWorkPart, 0x8, 0xFA, 0xB, 0x03000002);
        }
        return;

    case 0x4A8:
        se_req_frame_set(seWorkPart, 0x30, SE_Code_Make(0x16, 0x4, 0x16, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x4, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x10, 0x5, 0xB, 0x3);
        se_req_frame_set(seWorkMain, 0x12, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x1C, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2C, 0x1C, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x2C, 0x8, 0xB, 0x4);
        return;

    case 0x4A9:
        se_req_frame_set(seWorkPart, 0x28, SE_Code_Make(0x15, 0x4, 0x14, 0x4), 0xC, 0x3);
        se_req_frame_set(seWorkMain, 0x10, 0x2, 0xB, 0x2);
        se_req_frame_set(seWorkPart, 0x10, 0xFA, 0xB, 0x03000002);
        se_req_frame_set(seWorkMain, 0x14, 0x15, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x18, 0x18, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x1A, 0x17, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x8, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x22, 0x19, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x1D, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x4));
        se_req_frame_set(seWorkMain, 0x24, 0x7, 0xB, 0x4);
        return;

    case 0x4B1:
        se_req_frame_set(seWorkPart, 0x8, SE_Code_Make(0x4, 0x1, 0x5, 0x1), 0xC, 0x3);
        return;

    case 0x4B2:
        se_req_frame_set(seWorkPart, 0x8, SE_Code_Make(0x4, 0x1, 0x5, 0x1), 0xC, 0x3);
        return;

    case 0x4C5:
        fn_800DA428(seWorkFrame, 0x18, ((part << 0x18) | 0x2), 0x14, 0x2);
        se_req_frame_set(seWorkPart, 0x2C, 0x8, 0xE, 0x03000003);
        fn_800DA428(seWorkFrame, 0x2A, ((part << 0x18) | 0x3), 0x11, 0x2);
        se_req_frame_set(seWorkMain, 0x26, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x54, 0x1C, 0xE, 0x03000003);
        fn_80229E10(seWorkPart, 0x1A, 0x3);
        return;

    case 0x4C7:
        fn_800DA428(seWorkFrame, 0xE, (part << 0x18) | 0x2, 0x14, 0x2);
        fn_800DA428(seWorkFrame, 0x18, (part << 0x18) | 0x2, 0x11, 0x2);
        fn_800DA428(seWorkFrame, 0x5E, (part << 0x18), 0x14, 0x2);
        se_req_frame_set(seWorkMain, 0x34, 0x2, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkPart, 0x4A, 0x8, 0xE, 0x03000003);
        fn_80229E10(seWorkPart, 0x24, 0x3);
        fn_80229E10(seWorkPart, 0x60, 0x3);
        fn_80229EA8(seWorkPart, 0x54, 0x7, 0x3);
        return;

    case 0x4E3:
        se_req_frame_set(seWorkPart, 0x8, SE_Code_Make(0x18, 0x1, 0x19, 0x1), 0xC, 0x3);
        return;

    case 0x4F7:
        se_req_frame_set(seWorkMain, 0x42, 0x16, 0xB, (((seWorkMain->field_0x0C + 1) << 0x18) | 0x3));
        se_req_frame_set(seWorkMain, 0x24, 0x6, 0xB, 0x4);
        se_req_frame_set(seWorkPart, 0x8, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1A, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4E, 0xEE, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x56, 0xEE, 0x11, 0x03000003);
        /* fallthrough */
    default:
        return;
    }
}

/* The same 261-slot walk (0x3EA-0x4EE) as `fn_80241558`, with six of its arms absent (those motions take the
 * default) and three arms whose SE codes differ. */
extern "C" void fn_8023FC20(_PLW* work, u8 part) {
    u32 partHi;
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 0x3EA:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4A, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x60, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 4, 0x14, 2);
        return;
    case 0x3EB:
        fn_80229E10(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x66, part << 0x18, 0x11, 2);
        fn_80229E10(seWorkPart, 0x68, 3);
        return;
    case 0x3EC:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        return;
    case 0x3F1:
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x30, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x4A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x66, partHi | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 6, 3);
        fn_80229E10(seWorkPart, 0x20, 3);
        fn_80229E10(seWorkPart, 0x40, 3);
        fn_80229E10(seWorkPart, 0x5A, 3);
        return;
    case 0x3F3:
        se_req_frame_set(seWorkPart, 6, SE_Code_Make(0, 3, 1, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0x29, 0xC, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xB, 0xE, 0x03000002);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 4, partHi | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x2E, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x34, 3);
        return;
    case 0x408:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        return;
    case 0x409:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x14, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x16, 3);
        fn_80229E10(seWorkPart, 0x28, 3);
        return;
    case 0x40A:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        return;
    case 0x40B:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x14, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0x1C, 3);
        return;
    case 0x40C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x38, part << 0x18, 0x14, 2);
        fn_80229E10(seWorkPart, 0x3A, 3);
        return;
    case 0x40D:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x10, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x24, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x34, 3);
        return;
    case 0x40E:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3A, partHi | 4, 0x11, 2);
        fn_80229EA8(seWorkPart, 0x26, 0xB, 3);
        fn_80229E10(seWorkPart, 0x3C, 3);
        return;
    case 0x40F:
        fn_80229E10(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x14, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x36, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xE, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x5A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        return;
    case 0x411:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2E, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        fn_80229E10(seWorkPart, 0x30, 3);
        return;
    case 0x412:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 2, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 4, 3);
        fn_80229E10(seWorkPart, 0x1C, 3);
        return;
    case 0x41B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3A, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x5C, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        return;
    case 0x41C:
        se_req_frame_set(seWorkMain, 0xA, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4A, 0xEA, 3, 0x03000003);
        return;
    case 0x41D:
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        return;
    case 0x421:
        se_req_frame_set(seWorkMain, 0xC, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1E, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xEA, 3, 0x03000003);
        return;
    case 0x426:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x03000003);
        return;
    case 0x428:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x14, 0x03000003);
        return;
    case 0x429:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x42A:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x11, 0x03000003);
        return;
    case 0x42B:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43A:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x43B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43C:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 6, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x22, 0xEE, 0x14, 0x03000003);
        return;
    case 0x44D:
    case 0x44E:
    case 0x44F:
        se_req_frame_set(seWorkPart, 8, 8, 0x11, 0x03000003);
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x5A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x6C, partHi | 1, 0x11, 2);
        fn_80229E10(seWorkPart, 0x60, 3);
        return;
    case 0x450:
        se_req_frame_set(seWorkMain, 0x20, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x78, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x1A, 3);
        fn_80229E10(seWorkPart, 0x7A, 3);
        return;
    case 0x451:
    case 0x452:
    case 0x453:
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        return;
    case 0x454:
        se_req_frame_set(seWorkMain, 0x24, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x2C, partHi | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3E, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x60, partHi | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x7C, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x46, 3);
        fn_80229E10(seWorkPart, 0x7E, 3);
        return;
    case 0x455:
    case 0x456:
        se_req_frame_set(seWorkMain, 0x24, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x68, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x84, partHi | 1, 0x11, 2);
        se_req_frame_set(seWorkPart, 6, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x6C, 3);
        return;
    case 0x458:
        se_req_frame_set(seWorkMain, 4, 1, 3, 4);
        return;
    case 0x459:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(2, 3, 1, 3), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 0x28, 0x14, 0x03000003);
        fn_80229EA8(seWorkPart, 0x36, 7, 3);
        return;
    case 0x45A:
    case 0x45B:
        se_req_frame_set(seWorkMain, 0x24, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2E, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3C, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x78, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 4, 3);
        fn_80229E10(seWorkPart, 0x7A, 3);
        return;
    case 0x45C:
    case 0x45D:
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x46A:
        se_req_frame_set(seWorkMain, 2, 1, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5C, 4, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x12, partHi | 1, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x58, partHi | 4, 0x14, 2);
        return;
    case 0x46B:
        se_req_frame_set(seWorkMain, 2, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        fn_800DA428(seWorkFrame, 0x24, part << 0x18, 0x14, 2);
        fn_80229E10(seWorkPart, 0x26, 3);
        return;
    case 0x47E:
    case 0x47F:
    case 0x480:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x14, 0x03000003);
        return;
    case 0x481:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x74, 0xED, 3, 0x03000003);
        return;
    case 0x482:
    case 0x483:
    case 0x484:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x11, 0x03000003);
        return;
    case 0x485:
    case 0x486:
    case 0x487:
        se_req_frame_set(seWorkMain, 0x24, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x36, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xEE, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x64, 0xED, 0x11, 0x03000003);
        return;
    case 0x489:
        se_req_frame_set(seWorkMain, 4, 2, 3, 4);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x48A:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x14, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xE, 0x8C, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xED, 0x14, 0x03000003);
        return;
    case 0x48B:
    case 0x48C:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x68, 0xED, 0x14, 0x03000003);
        return;
    case 0x48D:
    case 0x48E:
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x49C:
        se_req_frame_set(seWorkMain, 4, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5A, 0xE, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x49D:
        se_req_frame_set(seWorkMain, 6, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x4B1:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(4, 2, 4, 2), 0xC, 3);
        fn_800DA428(seWorkFrame, 2, part << 0x18, 0x14, 2);
        return;
    case 0x4BB:
        fn_800DA428(seWorkFrame, 0x3A, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 2, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x3C, 3);
        return;
    case 0x4BC:
        fn_800DA428(seWorkFrame, 0x72, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 8, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x74, 3);
        return;
    case 0x4E3:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x18, 1, 0x19, 1), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xC, 0xED, 0x11, 0x03000003);
        return;
    case 0x4ED:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1E, 0xED, 0x11, 0x03000003);
        return;
    case 0x4EE:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3A, 0xED, 0x11, 0x03000003);
        /* fallthrough */
    default:
        return;
    }
}
