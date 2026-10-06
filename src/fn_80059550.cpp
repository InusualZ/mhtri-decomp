/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80059550`, which answers the runtime dump's `zz_0059550_`
 * placeholder for every function here; every `.text` entry in 0x80059550..0x8005AA28 in
 * config/RMHE08/symbols.txt is a bare `fn_XXXXXXXX`).  The mangled names the range does carry
 * (`drawSpr2TF__FUcP9fltSpr2TFUc`, `subTransSet__FUllPUl`, `disp_beta_tex__FP3VecP4Vec28_GXColorP9_GXTexObj`,
 * `Panic__Q24nw4r2dbFPCciPCce`, `CosFIdx__Q24nw4r4mathFf`) are spelled by their real declarations, never
 * as callable identifiers (rule 9).
 *
 * The game-root "system message / save-create" callback unit.  `.text` 0x80059550..0x8005AA28 (0x14D8 B,
 * 12 functions), extab 0x800072D0..0x80007310, extabindex 0x8001F5A8..0x8001F608 (8 framed functions).
 * The range is one maximal unclaimed run (attribute.py, `fn_80059550.cpp`); its seam is
 * unproven - it is a proposal cap, not a TU boundary (see below).
 *
 * Naming - which evidence class decided it.
 *   * Class 1 (`__FILE__` string) FAILS: the unit's `.data` pool references are the NW4R inlined-assert
 *     strings `g3d_resmat_ac.h` (0x8058B308/0x8058B340), `%s::%s: Object not valid.` (0x8058B2E8) and
 *     `NW4R:Failed assertion !((u32)p & 0x3)` (0x8058B318), plus the small s16 sprite/message tables
 *     (0x8058B290/2B0/2C0/2CC); there is NO bare game source-file name in the pool.
 *   * Class 2 (runtime-dump name) FAILS: `dumpmap.py lookup 0x80059550` answers `zz_0059550_` only.
 *   * Class 3 decides it: the file keeps the map's stem `fn_80059550` and is registered in the game-root
 *     band - the `main` lib at the `src/` root with `cflags_main`, exactly where its link neighbours sit
 *     (`mh3_pad.cpp`, `fn_80047398.cpp`, the 0x8004C9A0/0x8004CAD8/draw_shape runs, `fn_80056F24` - which
 *     ends where this unit begins).  Module `main` is a recorded decision, not an invention.
 *
 * What the unit does (from the bodies): `fn_800595E4` is the per-frame system-message callback the
 * game-root task registrar `fn_80046D34` installs through `fn_800417F0(fn_800595E4, 0)`; its state machine
 * (state byte +0x08, substate +0x09) drives the NAND/save-create flow (`createSystemFile`, `createDataFile`,
 * `chg_nand_err2msgcode`), the Wii system message (`wii_sysmsg_gen`) and the on-screen message box
 * (`font_print_ex` / `font_set_size`).  `fn_80059550`/`fn_8005A36C`/`fn_8005A648` draw the message box's
 * background/beta sprite and text with `disp_beta_tex` and the 2D sprite pipeline `drawSpr2TF`; the
 * `fn_8005A8E0..fn_8005AA20` cluster is the NW4R `ResMat`-style validity-checked handle wrapper.
 *
 * Measurement path: the unit has no single registered target object yet (it is registered here for the
 * first time), so the source is compiled with MAIN's real `cflags_main` command line and each symbol is
 * scored with `report generate` against the retired per-range object that contains it - the
 * `auto_fn_<ADDR>_text.o` / `auto_03_<ADDR>_text.o` objects under `build/RMHE08/obj/`.
 *
 * Results (objdiff `report generate`, the official metric; each symbol scored against the retired
 * per-range object that contains it, source compiled with MAIN's real `cflags_main` command line):
 *   100.00: fn_8005A63C, fn_8005A8E0, fn_8005A910, fn_8005A91C, fn_8005A950, fn_8005A9B4, fn_8005AA20
 *    99.80: fn_8005A9BC
 *    98.38: fn_80059550
 *    94.38: fn_8005A36C
 *    56.01: fn_8005A648 (residual below)
 *     none: fn_800595E4 (0x D88, unwritten - residual below)
 * 10 of 12 symbols >= 80 %; the two below the bar are the two largest functions.
 *
 * Residuals (recorded, not worked around):
 *   * fn_8005A648 (56.01 %, ours 0x24C vs 0x298): retail keeps `id` in r29 and the horizontal position
 *     `x` in r31; MWCC gives this source `id`->r31 and `x`->r29, which recolours most of the body's
 *     compares/argument moves.  The instruction sequence is otherwise the same shape (the two duplicated
 *     `lbl_8060B1A8[sw9][id]` call sites are written separately, matching retail's two copies; merging
 *     them scores 53.7).  Declaration/usage order permutations (x before y, id as unsigned) were tried and
 *     are worse (47.6-53.7).
 *   * fn_800595E4 (0xD88) is NOT written: it is the per-frame callback's 8-state switch (`system_w`,
 *     `Psw`, the save-create/NAND flow, the jump tables at 0x8058B220/0x8058B24C/0x8058B26C).  Its
 *     3464 bytes are a residual for a later pass.
 *   * fn_8005A36C (94.38 %) keeps the whole shape but retail sign-extends each `spr.x/y = base + off`
 *     (`add`+`extsh`) where MWCC folds the `extsh` away because the `sth` truncates anyway - four
 *     instructions; an `s16` local forces a different (worse) colouring.  `#pragma peephole off` around
 *     the body is applied (it restores the un-fused colour pack: 90.9 -> 94.4).
 *   * fn_80059550 (98.38 %) is instruction-for-instruction except one relocation/argument difference.
 *   * fn_8005A9BC (99.80 %) is byte-equal in `.text`; the 0.2 is a relocation-row difference only.
 *   * `fltSpr2TF` is owned with `drawSpr2TF` (`src/fn_80047398.cpp`) but that unit has no header yet, so
 *     the type is defined here (rule 1's second-user move is recorded as a `shared-file` config request);
 *     `_MH_VEC2` is left incomplete and only viewed through `Vec2`'s identical layout (rule 1).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "unsplit/unknown.h" /* system_w (undecided module, rule 2's unsplit gap) */
#include "ef.h"               /* Vec / Vec2 / Panic legacy declarations */
#include "main.h"             /* get_ScreenSize / ck_WideMode (rule 2) */
#include "fn_80047398.h"      /* drawSpr2TF / subTransSet / fltSpr2TF (rule 2) */
#include "fn_8004CAD8.h"      /* wii_sysmsg_gen (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat (rule 2) */
#include "ef/fn_800CDB2C.h"   /* fn_800D0568 (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * The range's pooled data.
 * ------------------------------------------------------------------------------------------------- */

/* The NW4R inlined-assert strings (`fn_8005A950`, `fn_8005A9BC`).  `.data`, no registered owner. */
extern char lbl_8058B340[]; /* "g3d_resmat_ac.h"                                0x8058B340 */
extern char lbl_8058B318[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          0x8058B318 */
extern char lbl_8058B308[]; /* "g3d_resmat_ac.h"                                0x8058B308 */
extern char lbl_8058B2E8[]; /* "%s::%s: Object not valid."                      0x8058B2E8 */
extern char lbl_80791118[]; /* "%s"                                             0x80791118 */

/* The two s16 sprite tables `fn_8005A36C` uses. */
extern s16 lbl_8058B2B0[]; /* 8 s16: {0x1E0,0xF8, 0x1E0,0x82, 0xF0,0x50, 0xF0,0x50}    0x8058B2B0 */
extern s16 lbl_8058B290[]; /* 16 s16 RGBA colour pairs                                          0x8058B290 */
extern s16 lbl_8058B2C0[]; /* 6 s16: {0x7,0xA4, 0xE5,0xA4, 0x76,0xA4}                         0x8058B2C0 */

/* `fn_8005A648`'s message-string tables, indexed by `system_w.field_0x09`. */
extern char** lbl_8060B1A8[]; /* [6] string tables indexed by system_w.field_0x09      0x8060B1A8 */
extern char** lbl_8060B200[]; /* [6] string tables indexed by system_w.field_0x09      0x8060B200 */
extern char** lbl_8060B2F0[]; /* [6] string tables indexed by system_w.field_0x09      0x8060B2F0 */
extern s16 lbl_8058B2CC[];  /* 14 s16                                                  0x8058B2CC */

/* The SDATA scratch/constants the range touches. */
extern s16 lbl_80791114; /* font size base (22)                                      0x80791114 */
extern _GXColor lbl_80791110; /* the default white colour                           0x80791110 */
extern u32 lbl_807948C8; /* task argument scratch (forked to fn_800595E4)            0x807948C8 */
extern u32 lbl_807948D0[2]; /* {counter, arg}                                         0x807948D0 */

/* _GXTexObj / Vec2 are SDK types with no shared header yet; forward-declared for the mangled spelling. */
struct _GXTexObj;

/* The sprite rectangle's 2-float position, `disp_beta_tex`'s second parameter. size: 0x08 */
typedef struct Vec2 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
} Vec2; /* size: 0x08 */

/* `main.cpp` owns `_MH_VEC2` (the `get_ScreenSize` tag); kept incomplete here and only ever viewed
 * through `Vec2`'s identical layout (rule 1: the definition is not copied). */
struct _MH_VEC2;

/* The message-box work block `fn_80059550` reads through its second argument. size: 0x10 */
typedef struct BetaWork {
    /* +0x00 */ u8 pad_0x00[0x0E];
    /* +0x0E */ s16 taskIndex;
} BetaWork; /* size: 0x10 */

/* `fltSpr2TF` now lives in its owner's header `fn_80047398.h` (rule 1: one definition). */

/* The NW4R-`ResMat`-style validity-checked handle the `fn_8005A8E0..fn_8005AA20` cluster wraps.
 * size: 0x04 */
typedef struct ResHandle {
    /* +0x00 */ u32 mPtr;
} ResHandle; /* size: 0x04 */

namespace nw4r {
namespace db {
/* `Panic(const char*, int, const char*, ...)`; the map name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
} // namespace db
namespace math {
/* `nw4r::math::CosFIdx(f32)` - the table-cosine the sine wave in `fn_8005A63C` uses. */
f32 CosFIdx(f32 idx);
} // namespace math
} // namespace nw4r

/* The message-box draw helpers whose map names are C++ manglings - declared at global scope with C++
 * linkage, so the linker sees the mangling (`font_print_ex__FsssPSce` etc.), never the `extern "C"`
 * unmangled stem.  `drawSpr2TF`/`subTransSet`/`get_ScreenSize`/`ck_WideMode`/`wii_sysmsg_gen`/
 * `fn_800D0568` come from their owner headers (rule 2). */
void disp_beta_tex(Vec* a, Vec2* b, _GXColor color, _GXTexObj* c);
void font_print_ex(s16 x, s16 y, s16 flag, signed char* fmt, ...);
void font_set_size(s16 w, s16 h);

extern "C" {

/* --- foreign declarations whose map name is a plain `fn_XXXXXXXX` C stem --------------------------- */

/* 0x8005A648 - draw the message-box text for the system-message ids. */
void fn_8005A648(u32 a, s32* idPtr) {
    s32 id = *idPtr;
    Vec2 scr;
    char buf[512];

    get_ScreenSize((_MH_VEC2*)&scr);
    s16 y = (s16)((scr.y - (f32)lbl_8058B2B0[1]) * 0.5f);
    s16 x = (scr.x == 640.0f) ? 64 : 171;
    font_set_size(lbl_80791114 + 2, lbl_80791114 + 2);

    if ((id >= 2 && id <= 7) || id == 1) {
        font_print_ex(x, 144, 0, (signed char*)lbl_80791118, lbl_8060B1A8[system_w.field_0x09][id]);
    } else if (id == 16 || id == 17 || id == 19) {
        wii_sysmsg_gen(id, buf, 0);
        font_print_ex(x, 134, 0, (signed char*)lbl_80791118, buf);
    } else if (id == -200) {
        font_print_ex(x, 144, 0, (signed char*)lbl_80791118, lbl_8060B1A8[system_w.field_0x09][0]);
    }

    if (id == 1) {
        font_print_ex(x, (s16)(y + lbl_8058B2CC[13]), 5, (signed char*)lbl_80791118, lbl_8060B200[system_w.field_0x09][0]);
    } else if (id == 16 || id == 17) {
        font_print_ex(x, (s16)(y + lbl_8058B2CC[9]), 5, (signed char*)lbl_80791118, lbl_8060B2F0[system_w.field_0x09][0]);
    }
}

/* 0x80059550 - the message-box beta background draw (a `subTransSet` callback). */
void fn_80059550(u32 unused, BetaWork** pp) {
    BetaWork* work = *pp;
    Vec2 pos;
    Vec size;
    pos.x = 853.0f;
    pos.y = 480.0f;
    if (ck_WideMode() == 1) {
        size.z = 0.0f;
        size.y = 0.0f;
        size.x = 0.0f;
    } else {
        size.x = -107.0f;
        size.z = 0.0f;
        size.y = 0.0f;
    }
    disp_beta_tex(&size, &pos, lbl_80791110, (_GXTexObj*)fn_800D0568(work->taskIndex + 5));
}

/* ---------------------------------------------------------------------------------------------------
 * The NW4R ResMat-style handle cluster, 0x8005A8E0..0x8005AA20.
 * ------------------------------------------------------------------------------------------------- */

/* 0x8005A910 - copy the handle word. */
void fn_8005A910(ResHandle* dst, ResHandle* src) {
    dst->mPtr = src->mPtr;
}

/* 0x8005A8E0 - copy-construct from `src` and return the new handle. */
ResHandle* fn_8005A8E0(ResHandle* self, ResHandle* src) {
    fn_8005A910(self, src);
    return self;
}

/* 0x8005A9B4 - store the handle word. */
} /* extern "C" */

/* 0x8005A9B4 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatChanData)

extern "C" {

} /* extern "C" */

/* 0x8005AA20 (0x8): returns the material block. */
nw4r::g3d::ResMatData* nw4r::g3d::ResMat::ptr() {
    return mpData;
}

extern "C" {

/* 0x8005A950 - the checked setter: store the word, assert it is 4-byte aligned, return self. */
#pragma peephole off
} /* extern "C" */

/* 0x8005A950 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatChan::ResMatChan(void* pData) : ResCommon<ResMatChanData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)lbl_8058B340, 465, (const char*)lbl_8058B318);
    }
}

extern "C" {
#pragma peephole on

} /* extern "C" */

/* 0x8005A9BC (0x64): returns the material block, panicking on a NULL handle. */
nw4r::g3d::ResMatData& nw4r::g3d::ResMat::ref() {
    if (!IsValid()) {
        nw4r::db::Panic(lbl_8058B308, 621, lbl_8058B2E8, GetClassName(), 0);
    }
    return *ptr();
}

extern "C" {

/* 0x8005A91C - checked setter applied to the word at +0x3F0 of the dereferenced handle. */
} /* extern "C" */

/* 0x8005A91C (0x34): returns the material's MatChan block. */
nw4r::g3d::ResMatChan nw4r::g3d::ResMat::GetResMatChan() {
    return ResMatChan((u8*)&ref() + 0x3F0);
}

extern "C" {

/* 0x8005A63C - the table cosine of `x` (256/(2*pi) == 0x4222F983 40.7437 scaled). */
f32 fn_8005A63C(f32 x) {
    return nw4r::math::CosFIdx(x * 40.7437f);
}

/* 0x8005A36C - draw the two beta/line sprite overlays for a message-box background. */
#pragma peephole off
void fn_8005A36C(s32 id, s32 flag) {
    Vec2 scr;
    fltSpr2TF spr;

    get_ScreenSize((_MH_VEC2*)&scr);

    spr.w = lbl_8058B2B0[0];
    spr.h = lbl_8058B2B0[1];
    spr.x = (s16)((scr.x - (f32)lbl_8058B2B0[0]) * 0.5f);
    spr.y = (s16)((scr.y - (f32)lbl_8058B2B0[1]) * 0.5f);
    spr.angle = 0;
    spr.r = lbl_8058B290[0];
    spr.g = lbl_8058B290[1];
    spr.b = lbl_8058B290[2];
    spr.a = lbl_8058B290[3];
    spr.color = 0xFFFFFFFF;
    drawSpr2TF(12, &spr, 0);

    s16 x = spr.x;
    s16 y = spr.y;

    if ((id >= 16 && id <= 17) || id == 1) {
        spr.w = lbl_8058B2B0[6];
        spr.h = lbl_8058B2B0[7];
        spr.x = (s16)(x + lbl_8058B2C0[4]);
        spr.y = (s16)(y + lbl_8058B2C0[5]);
        spr.angle = 0;
        spr.r = lbl_8058B290[12];
        spr.g = lbl_8058B290[13];
        spr.b = lbl_8058B290[14];
        spr.a = lbl_8058B290[15];
        spr.color = (flag == 0) ? 0xFFFFFFFF : 0xC0C0C0FF;
        drawSpr2TF(12, &spr, 0);

        spr.w = lbl_8058B2B0[4];
        spr.h = lbl_8058B2B0[5];
        spr.x = (s16)(x + lbl_8058B2C0[4]);
        spr.y = (s16)(y + lbl_8058B2C0[5]);
        spr.angle = 0;
        spr.r = lbl_8058B290[8];
        spr.g = lbl_8058B290[9];
        spr.b = lbl_8058B290[10];
        spr.a = lbl_8058B290[11];
        f32 c = fn_8005A63C((f32)(lbl_807948D0[0] & 0xFFFF) * 6.2831855f / 65536.0f);
        if (c < 0.0f) {
            c = c * -1.0f;
        }
        u8 a8 = (u8)((u32)(c * 95.0f) + 160);
        spr.color = ((u32)a8 << 24) | ((u32)a8 << 16) | ((u32)a8 << 8) | 0xFF;
        drawSpr2TF(12, &spr, 0);
    }
}

#pragma peephole on

} /* extern "C" */
