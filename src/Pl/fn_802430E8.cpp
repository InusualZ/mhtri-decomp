/*
 * Player motion -> SE (sound-effect) frame dispatch for `_PLW`, part two: the motions the sibling
 * dispatcher `Pl/fn_80241558.cpp` does not arm.
 *
 * The whole `.text` range 0x802430E8-0x802489D4 is 30 functions.  The first one, `fn_802430E8`
 * (0x1DA0 B), is the sibling of `Pl/fn_80241558.cpp`'s single function: it reads the current motion
 * number (`Get_motion_no(_PLW*)`, 0x8026A308), switches on it, and arms the fixed set of frame-timed
 * sound requests on the SE work objects hanging off the player work (`_PLW`+0xAF4 / +0xAF8 / +0xAFC).
 * MWCC turns its 261-case switch into the 261-entry `.data` jump table `jumptable_805C4134`
 * (0x805C4134-0x805C4548), which this unit owns.  52 of its 59 arms are instruction-for-instruction
 * identical to `fn_80241558`'s; the 7 that differ are motions 0x3F4/0x3F5 (different bodies here)
 * plus 0x3F6/0x410/0x43D/0x46C/0x49E, which the sibling sends to its default.
 *
 * Final home: module `Pl`, file stem kept as the map's `fn_802430E8` - the map has only
 * `fn_XXXXXXXX` for the range and no `__FILE__` string covers it (the `.data` pool between
 * `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at 0x805CDFC8 carries no source name for the
 * band; the one `enemy_control` string the proposal's seam note named is at 0x805A1BB8, 0x2A000
 * bytes below this range and referenced by nothing in it).  The subsystem is the player: the first
 * argument is passed straight to `Get_motion_no`, whose map spelling is `Get_motion_no__FP4_PLW`,
 * and the three SE works are the `_se_w` pointers at `_PLW`+0xAF4/+0xAF8/+0xAFC the sibling unit
 * uses.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `grep -n "^fn_8024" config/RMHE08/symbols.txt` - every entry in the range is a bare `fn_<addr>`
 * stem, and `dumpmap.py lookup <addr>` answers only `zz_<addr>_` for all 30).
 *
 * Language: C++. The range's calls carry the mangled `Get_motion_no__FP4_PLW`,
 * `se_req_frame_set__FP5_se_wllll` and `SE_Code_Make__Flsls`, and the retail object has an
 * extab/extabindex pair (58 unwind-only records) - which is why the Pl lib sets -Cpp_exceptions on.
 *
 * Flags: none beyond the lib's `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * mw_version Wii/1.0).
 *
 * Residual: 9 of the range's 30 functions are written and all nine measure 100.0 %
 * (`fn_802430E8`, `fn_80245DA0`, `fn_80245E20`, `fn_80246158`, `fn_802466C4`, `fn_802478A4`,
 * `fn_80247C2C`, `fn_80247EF0`, `fn_802488D4`).  The other 21 - `fn_80244E88` (0xE68 B, the
 * per-motion effect dispatcher `Pl/fn_80229ECC.cpp` calls with `&_PLW::field_0xAF4`) and
 * `fn_80245CF0`/`fn_80245E6C`/`fn_80245F40`/`fn_80245FD8`/`fn_802461F0`/`fn_802462FC`/`fn_80246654`/
 * `fn_8024676C`/`fn_80246AB4`/`fn_80246B7C`/`fn_80246D7C`/`fn_802470E4`/`fn_80247534`/`fn_802476A0`/
 * `fn_8024794C`/`fn_80247CC4`/`fn_80247D74`/`fn_80248018`/`fn_802482C4`/`fn_802485C8` - are declared
 * here only as far as a written caller needs them, and build as undefined relocations; the inventory
 * is `config/RMHE08/symbols.txt`.  Two of them are half-blocked on a header clash rather than on
 * codegen: `fn_80246654` needs `get_move_work_adrs` from `include/enemy/fn_80165FC8.h`, which cannot
 * be included beside `sound/se.h` (both declare `fn_800532DC`/`fn_80041E8C` with different types), and
 * `fn_80247CC4` needs `fn_80043EA8`/`fn_8012A624` and a full `VEC3` local through `include/ef.h`.
 */

#include "types.h"
#include "pl.h"
/* `incldue/pl.h` pulls in `ef.h`, and `ef.h` and `sound/se.h` declare `fn_80041E8C` with different
 * record types (`Vec*` vs `nwbr::math::VEC3*`), which MWCC rejects as `(10197) illegal function
 * overloading`.  This unit needs both headers and never calls it, so se.h's copy is renamed out of
 * the way for the include - the same workaround `enemy/fn_801550FC.cpp` uses. */
#define fn_80041E8C mhtri_se_h_fn_80041E8C
#include "sound/se.h"
#undef fn_80041E8C
#include "sound/fn_800D7F54.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "ef/fn_800CDB2C.h"
/* `include/Pl/fn_8025F088.h` is the owner of `fn_80260198` (0x80260198 sits inside that unit's
 * range 0x8025F088-0x80262940) and carries the unregistered `GetItemData` declaration, so this unit
 * takes both from it rather than keeping copies (rule 2).  The header also declares
 * `fn_80041E40(void*, const void*)`, whose owner is `mh3_pad.cpp` (`include/mh3_pad.h`), while
 * `sound/se.h` below declares the same name over `VEC3*`; this unit calls neither, so the header's
 * copy is renamed out of the way for the include - the same workaround the se.h copy already uses. */
#define fn_80041E40 mhtri_fn8025f088_h_fn_80041E40
#define fn_80335CE8 mhtri_fn8025f088_h_fn_80335CE8
#include "Pl/fn_8025F088.h"
#undef fn_80041E40
#undef fn_80335CE8
#include "Pl/fn_802693C4.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"

/* The unit's own unwritten siblings, in address order: their callers below need the signature the
 * target's call sites set up.  Each is declared here (the owner's file, docs/plan.md 6.5 rule 2) and
 * is added with its body. */
extern "C" void fn_802462FC(_PLW* self, u32 a, u16 b, u8 c, u8 d, u32 e);
extern "C" void fn_80247CC4(_PLW* self);

extern "C" void fn_802430E8(_PLW* work, u8 part) {
    u32 partHi;
    /* The three per-part `_se_w` work objects; the declaration order and the assignment order are
     * deliberately different - it is the shape that reproduces retail's colouring
     * (r31 = +0xAF4, r29 = +0xAF8, r30 = +0xAFC) together with its load order, exactly as in
     * `Pl/fn_80241558.cpp`. */
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
    case 0x424:
    case 0x425:
    case 0x42C:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x03000003);
        return;
    case 0x42D:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x11, 0x03000003);
        return;
    case 0x43D:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
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
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
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
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x459:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(2, 3, 1, 3), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 0x27, 0x14, 0x03000003);
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
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x48A:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x14, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xE, 0x8B, 3, 0x03000003);
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
    case 0x49E:
        se_req_frame_set(seWorkMain, 6, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x410:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xA, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        return;
    case 0x3F4:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 6, partHi, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x11, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x3F5:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 6, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x3F6:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 4, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x46C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xC, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1C, partHi | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x14, 2);
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

/* Clears the act's timer/latch block on entry and, when the alternate path is taken on a live master,
 * drops the 0x300 pair from the act bitfield. */
extern "C" void fn_80245DA0(_PLW* self, u8 a) {
    if (Pl_master_ck(self) == 1 && a == 0) {
        self->field_0x3D8 &= ~0x300;
    }
    if (a == 0) {
        self->field_0x418 = 0;
        self->field_0x41E = 0;
        self->field_0x444 = 0;
    }
    self->field_0x416 = 0;
    self->field_0x41C = 0;
}

/* Reports the input-hold latch, defaulting to "held" while the pad layer has nothing to say. */
extern "C" s32 fn_80245E20(_PLW* self) {
    if (fn_8042CB9C() == 0) {
        return 1;
    }
    return self->field_0x645 != 0;
}

/* Steps the actor's chase one frame along the chunk's own rotation-offset pair. */
extern "C" void fn_80246158(_PLW* self) {
    VEC3 vec;
    VEC3 scratch;
    fn_80043EA8(&vec);
    fn_80043EA8(&scratch);
    vec.x = lbl_805C4898[self->chunk_ofs * 2];
    vec.y = lbl_80799E00;
    vec.z = lbl_805C4898[self->chunk_ofs * 2 + 1];
    rotVecY(&vec, self->field_0x058);
    self->motion_pos_0x3C = self->motion_pos_0x3C + vec.x;
    self->motion_pos_0x44 = self->motion_pos_0x44 + vec.z;
}

/* Scans the equipment slots for the first crafted item and records its index. */
extern "C" void fn_802466C4(_PLW* self) {
    s32 i;
    self->field_0x304 = 0;
    for (i = 0; i < 0x1A; i++) {
        u16 item_id = self->slot_id[i].item_id;
        if (self->slot_id[i].value > 0) {
            u8* item = GetItemData(item_id);
            if (item_id != 0 && (item[2] & 8) != 0 && item[0] != 1) {
                self->field_0x304 = (u8)i;
                break;
            }
        }
    }
}

/* Drives the act's first two steps: arm the motion and hold, then wait for it. */
extern "C" void fn_802478A4(_PLW* self) {
    fn_80277C48(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8026A224(self, 0x12, 4, 0);
        fn_80275B04(self, 0, 0, 0);
        fn_80277C58(self);
        break;
    case 1:
        if (fn_8026A33C(self) == 1) {
            fn_802761B8(self, 0, 6, 0);
        }
        break;
    }
}

/* Halves (or thirds) the act's damage figure from the equip skills and hands it to the health change
 * helper. */
extern "C" void fn_80247C2C(_PLW* self) {
    s16 amount;
    if (Pl_Skill_ck(self, 0xA8) == 1) {
        amount = -0x10;
    } else if (Pl_Skill_ck(self, 0xA9) == 1) {
        amount = -0x24;
    } else {
        amount = -0x1E;
    }
    if (Pl_cat_skill_ck(self, 0x21) == 1) {
        amount = (s16)amount / 2;
    }
    fn_80276868(self, amount);
}

/* Runs the first two act steps of the alternate chase and then the shared chase update. */
extern "C" void fn_80247EF0(_PLW* self, u32 a) {
    fn_80277C48(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            fn_80275B04(self, 2, 0, 0);
            fn_8026A224(self, 0x22, 4, 0);
        } else {
            fn_80275B04(self, 3, 0, 0);
            fn_8026A224(self, 0x8B, 4, 0);
        }
        self->field_0x585 = 0;
        break;
    case 1:
        if (Pl_frame_check(self, 1, lbl_80799E24, lbl_80799E00) == 1) {
            if (a == 0) {
                fn_80275AC4(self, 0, 0xB, 0);
            } else {
                fn_80275AC4(self, 0, 0x4B, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The close-quarters variant of the same two-step chase. */
extern "C" void fn_802488D4(_PLW* self, u32 a) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            fn_80275B04(self, 0, 0, 0);
            fn_8026A224(self, 0x24, 6, 0);
        } else {
            fn_80275B04(self, 3, 0, 0);
            fn_8026A224(self, 0x8D, 6, 0);
        }
        break;
    case 1:
        if (fn_8026A33C(self) == 1) {
            if (a == 0) {
                fn_802761B8(self, 0, 6, 0);
            } else {
                fn_802761B8(self, 3, 6, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}
