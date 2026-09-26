/* enemy/fn_8018B3B8.cpp - the enemy multi-motion action band between `fn_80182D5C` and
 * `fn_80191598`.
 *
 * `.text` 0x8018B3B8..0x80191598 (24 functions, 0x61E0 B), extab 0x8000ECC4..0x8000ED64,
 * extabindex 0x8002A210..0x8002A300.  Registered from `proposal/8018B3B8_fn_8018B3B8.cpp`.
 *
 * Module `enemy`, decided by class 3 (what the code does plus the neighbours' scheme): both
 * bracketing registered units are `enemy/*` (the unit below ends exactly at 0x8018B3B8 and
 * `enemy/fn_80191598.cpp` starts exactly at 0x80191598), every callee out of the range is an
 * enemy-band function, and the four dispatchers ([`fn_8018B3C8`] on the work's +0x1E6, `fn_8018D250`
 * on +0x1E6, `fn_8018D2B0` on +0x1E5, and the whole +0x5 state-machine family) key on the same
 * `_ENEMY_WORK` the neighbours read.  Class 1 fails: nothing in the range names a source file (the
 * `.data`/`.sdata2` pool holds only numeric constants and the two jump tables), and class 2 fails:
 * `python tools/symbols/dumpmap.py lookup 0x8018B3B8` answers the `zz_018b3b8_` placeholder form,
 * which is not evidence.  The file keeps the map's own stem (class 4).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit` over
 * config/RMHE08/symbols.txt - every symbol defined here is a bare `.text` entry with no owner name -
 * and `python tools/symbols/dumpmap.py lookup` answers the `zz_XXXXXXXX_` placeholder form for the
 * whole inventory, which is not evidence).
 *
 * Language C++ (`-lang=c++` through the lib's `cflags_main`): the range reaches mangled callees
 * (`em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check__FP11_ENEMY_WORKUsff`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor`) through
 * their real signatures (rule 9), and `_ENEMY_WORK::char_0x024` is the `MHchar` base `pl.h` owns.
 *
 * Types.  `_ENEMY_WORK` comes from its single home `include/enemy/ENEMY_WORK.h` (rule 1), not from
 * the older `include/enemy.h` copy.  This band reads the +0x350..+0x35E bytes as a run of signed
 * 16-bit TEV colour words (`fn_80191038` does `lha`/`sth` at +0x350/+0x352/+0x354/+0x356/+0x358 and
 * `fn_801913FC` `lha` at +0x35A/+0x35C), where the header had byte views; the three union members
 * the range needs (`tev_0x350`, `tev_0x354`, the +0x358 union member) were added there with the
 * offsets preserved, so no other unit's layout moved.
 *
 * The +0x5 state byte is a step counter (`fn_8018B418` steps 0..10, the `fn_8018BBD8` family 0..1);
 * +0x1E6 is the multi-motion class (`fn_8018B3C8`/`fn_8018D250` dispatch 0..13), +0x1E5 the action
 * (`fn_8018D2B0` dispatches 0..13) and +0x3 the team (0x10/0x11/0x15).  `fn_8018D8C8` is the
 * per-(team,motion) frame-window driver: for each motion it runs the `em_after_frame_check` windows
 * and calls `fn_8018D558` (the effect-spawn router) with the window's id/kind.
 *
 * Status (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`):
 * all 24 bodies are written and every one is above the 80 % bar; 13 are byte-identical.
 *   fn_8018B3B8 100, fn_8018B3BC 100, fn_8018B3C8 100, fn_8018BBD8 100, fn_8018BC7C 100,
 *   fn_8018BF4C 100, fn_8018C2CC 100, fn_8018C528 100, fn_8018C998 100, fn_8018CDE0 100,
 *   fn_8018D1AC 100, fn_8018D250 100, fn_8018D2B0 100;
 *   fn_8018B418 98.31 (1984 B), fn_8018BFF0 98.91 (732 B), fn_8018C370 97.27 (440 B),
 *   fn_8018C5CC 97.94 (972 B), fn_8018CA3C 97.42 (932 B), fn_8018CE84 97.52 (808 B),
 *   fn_8018D370 91.76 (488 B), fn_8018D558 93.16 (880 B), fn_8018D8C8 96.98 (14192 B),
 *   fn_80191038 83.84 (964 B), fn_801913FC 92.22 (412 B).  Unit: 96.6689 % fuzzy, 24 functions,
 *   13 matched.  Sections: extab 0xA0 and extabindex 0xF0 match the target's exactly; `.text` is
 *   0x6028 against the target's 0x61E0 (the shortfall is inside the non-identical functions).
 *
 * Residuals, by measurement (all are codegen shapes, not comprehension):
 *   * `fnmsubs` FUSION - `fn_801913FC` 92.22 (412 B; ours 392).  The target keeps the
 *     `(lbl_80797ED0 + fn_8013026C(self)) * scale` as `fmuls` + `fsubs`; this build's -O3 fuses it
 *     into `fnmsubs f1,f31,f1,f0`.  A `#pragma peephole off` was not applied (it moves the other
 *     twelve functions); the residual is the fused pair only.
 *   * ARGUMENT EVALUATION ORDER - `fn_8018D370` 91.76 (488 B; ours 464) and `fn_80191038` 83.84
 *     (964 B; ours 912).  The target evaluates the float argument of `em_water_check`/
 *     `fn_801048B4` before the integer ones, and materialises the `_GXColor` byte record in the
 *     order b,g,r,a; MWCC orders by declaration, so a few rows still differ.  Both spellings are
 *     ABI-equivalent.
 *   * `_GXColor` record - `fn_80191038` stores the four colour bytes through `_GXColor`; the
 *     target's store order (b/g/r/a, then a) is the compiler's, and the 52-byte frame difference is
 *     this unit's view of the same record.
 *   * The near-identical multi-window machines (`fn_8018B418` 98.31, `fn_8018C5CC` 97.94,
 *     `fn_8018CA3C` 97.42, `fn_8018CE84` 97.52, `fn_8018D558` 93.16, `fn_8018D8C8` 96.98) each miss
 *     a handful of rows on the shared `fn_8018D558` call tail and on a `b` that has become a
 *     fall-through (or the reverse); the control flow and every callee are correct.
 *
 * Type and declaration follow-ups (the outbox carries each as a `shared-file`/`config_requests`
 * entry):
 *   * the plain band callees declared in this file (`fn_8018479C`, `fn_80184D98`, `fn_80183DCC`,
 *     `fn_80184488`, `fn_80184BF8`, `fn_801861E4`, `fn_80186960`, `fn_80189C7C`, `fn_8018A974`,
 *     `fn_8018AB64`, `fn_8018AB94`..`fn_8018B258`) belong in `include/unsplit/enemy.h` or their
 *     owner's header (rule 2); they are elected `_ENEMY_WORK*`-typed here.
 *   * `EmShellTbl` is the same 0x40-byte record as `enemy/fn_80147CE0.cpp`'s `EmShellSetFunc` (rule-1
 *     follow-up: fold both into one header).
 *   * the +0x350..+0x35E TEV union members added to `include/enemy/ENEMY_WORK.h` (`tev_0x350`,
 *     `tev_0x354`, the +0x358 union member) are this unit's signed-short view; they should be named
 *     once for the band.
 */
#include "types.h"

#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"
#include "pl.h"              /* MHchar / setTevKColor / _CP_VECTOR */
#include "unsplit/unknown.h" /* SystemWork / system_w */

/* ------------------------------------------------------------------------------------------------ *
 * The mangled callees, outside `extern "C"` so the front-end mangles them the way the map spells
 * them (rule 9).  Each parameter width is the one the map's mangling encodes.
 * ------------------------------------------------------------------------------------------------ */
u16 calcVecAngX(nw4r::math::VEC3* v);
void eft009_set_pos(u8 id, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 arg);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u32 em_water_check(struct _ENEMY_WORK* self);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
f32 get_em_scale(struct _ENEMY_WORK* self);
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* The shell callback table `shell_set_func_ptr` points at, as this band reads it (`fn_8018D8C8`
 * calls +0x2C and +0x30).  Same record as `enemy/fn_80147CE0.cpp`'s `EmShellTbl` (rule-1
 * follow-up: the two copies should move to one header).
 * size: 0x40 */
typedef struct EmShellTbl {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ void (*field_0x2c)(_ENEMY_WORK* self, u32 a, u32 b, void* params, u32 c, void* table, void* table2);
    /* +0x30 */ void (*field_0x30)(_ENEMY_WORK* self, u32 a, void* a1, void* a2, u32 b, void* table, void* table2);
    /* +0x34 */ u8 unused_0x34[0x40 - 0x34];
} EmShellTbl;
extern "C" EmShellTbl* shell_set_func_ptr;

extern "C" {

/* ------------------------------------------------------------------------------------------------ *
 * This unit's own functions, declared up front so the dispatchers can call them.
 * ------------------------------------------------------------------------------------------------ */
void fn_8018B3B8(_ENEMY_WORK* self);
void fn_8018B3BC(_ENEMY_WORK* self);
void fn_8018B3C8(_ENEMY_WORK* self);
void fn_8018B418(_ENEMY_WORK* self);
void fn_8018BBD8(_ENEMY_WORK* self);
void fn_8018BC7C(_ENEMY_WORK* self);
void fn_8018BF4C(_ENEMY_WORK* self);
void fn_8018BFF0(_ENEMY_WORK* self);
void fn_8018C2CC(_ENEMY_WORK* self);
void fn_8018C370(_ENEMY_WORK* self);
void fn_8018C528(_ENEMY_WORK* self);
void fn_8018C5CC(_ENEMY_WORK* self);
void fn_8018C998(_ENEMY_WORK* self);
void fn_8018CA3C(_ENEMY_WORK* self);
void fn_8018CDE0(_ENEMY_WORK* self);
void fn_8018CE84(_ENEMY_WORK* self);
void fn_8018D1AC(_ENEMY_WORK* self);
void fn_8018D250(_ENEMY_WORK* self);
void fn_8018D2B0(_ENEMY_WORK* self);
void fn_8018D370(_ENEMY_WORK* self);
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);
void fn_8018D8C8(_ENEMY_WORK* self);
void fn_80191038(_ENEMY_WORK* self);
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1);

/* ------------------------------------------------------------------------------------------------ *
 * The plain band callees the target names (no registered owner / not in a public header yet - the
 * lint's counted "band interleaves modules" gap; the outbox carries the list for rule 2).
 * ------------------------------------------------------------------------------------------------ */
void fn_8018479C(_ENEMY_WORK* self);
void fn_80184D98(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80183DCC(_ENEMY_WORK* self);
void fn_80184488(_ENEMY_WORK* self);
void fn_80184BF8(_ENEMY_WORK* self);
void fn_801861E4(_ENEMY_WORK* self);
void fn_80186960(_ENEMY_WORK* self);
void fn_80189C7C(_ENEMY_WORK* self);
void fn_8018A974(_ENEMY_WORK* self);
void fn_8018AB64(_ENEMY_WORK* self);
void fn_8018AB94(_ENEMY_WORK* self);
void fn_8018AC64(_ENEMY_WORK* self);
void fn_8018ACEC(_ENEMY_WORK* self);
void fn_8018AD68(_ENEMY_WORK* self);
void fn_8018ADE8(_ENEMY_WORK* self);
void fn_8018AE7C(_ENEMY_WORK* self);
void fn_8018B1C4(_ENEMY_WORK* self);
void fn_8018B258(_ENEMY_WORK* self);
void fn_80127F48(_ENEMY_WORK* self);
void fn_80129668(_ENEMY_WORK* self, u32 a, u32 b);
void fn_8012CF20(_ENEMY_WORK* self);
u32 fn_8012EC60(_ENEMY_WORK* self);
u32 fn_8012EC3C(_ENEMY_WORK* self);
void fn_8012F504(_ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
void fn_8012F5B8(_ENEMY_WORK* self, s32 a, s32 b, s32 c);
void fn_8012F8C8(_ENEMY_WORK* self, f32 a);
u32 fn_8012F93C(_ENEMY_WORK* self);
f32 fn_8013026C(_ENEMY_WORK* self);
void fn_80130478(_ENEMY_WORK* self, u32 a);
void fn_801305C4(_ENEMY_WORK* self, f32 a);
void fn_80131D84(_ENEMY_WORK* self);
void fn_80131E74(_ENEMY_WORK* self);
u32 fn_801321DC(_ENEMY_WORK* self);
void fn_80133C3C(_ENEMY_WORK* self);
u16 fn_80133DB0(u16 a, u16 b, u16 c);
void fn_80133E3C(_ENEMY_WORK* self, s32 a, f32 b, f32 c);
u32 fn_80135748(_ENEMY_WORK* self, u32 a);
void fn_80136B50(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80136D14(_ENEMY_WORK* self);
s16 fn_80145FE4(void);
u32 fn_80146008(u32 a);
void fn_80146058(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014610C(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014616C(_ENEMY_WORK* self, u32 a);
void fn_8014619C(_ENEMY_WORK* self);
void fn_801461A8(_ENEMY_WORK* self, s16 a, const void* tbl, u32 b);
void fn_801462A4(_ENEMY_WORK* self, s16 a, const void* tbl, const void* tbl2, u32 b, u32 c);
void fn_801048B4(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_801049D0(_ENEMY_WORK* self, u32 a, u32 b, u32 c, nw4r::math::VEC3* p, f32 d);
void fn_8010562C(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 d);
void fn_80106694(_ENEMY_WORK* self, nw4r::math::VEC3* p, u32 a, f32 d);
void fn_8010D2B0(nw4r::math::VEC3* p, u8 a, u32 b, u32 c, f32 d);
void fn_80117E58(_ENEMY_WORK* self, u32 a, nw4r::math::VEC3* p, u32 b, f32 d);
void fn_8011D448(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8011D4FC(_ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d, f32 e);
void fn_8011D690(_ENEMY_WORK* self, u32 a, u32 b, f32 c);
void fn_801E2D04(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, u32 c, f32 d);
void fn_80304508(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 c);
void* fn_80041E40(void* dst, const void* src);
void fn_8005050C(void* out);
void fn_80051378(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 fn_8005024C(u16 a);
void fn_80191EF8(_ENEMY_WORK* self);
void fn_80192080(_ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------ *
 * The pool literals and jump tables the range reads (declared, never defined - the data pass owns
 * them; playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern f32 lbl_80797E88;
extern f32 lbl_80797E9C;
extern f32 lbl_80797EA0;
extern f32 lbl_80797EA4;
extern f32 lbl_80797EB4;
extern f32 lbl_80797EB8;
extern f32 lbl_80797EC0;
extern f32 lbl_80797EC4;
extern f32 lbl_80797EC8;
extern f32 lbl_80797ECC;
extern f32 lbl_80797ED0;
extern f32 lbl_80797ED4;
extern f32 lbl_80797EE4;
extern f32 lbl_80797EE8;
extern f32 lbl_80797EEC;
extern f32 lbl_80797EF8;
extern f32 lbl_80797F00;
extern f32 lbl_80797F08;
extern f32 lbl_80797F0C;
extern f32 lbl_80797F10;
extern f32 lbl_80797F14;
extern f32 lbl_80797F34;
extern f32 lbl_80797F38;
extern f32 lbl_80797F40;
extern f32 lbl_80797F44;
extern f32 lbl_80797F4C;
extern f32 lbl_80797F50;
extern f32 lbl_80797F58;
extern f32 lbl_80797F5C;
extern f32 lbl_80797F64;
extern f32 lbl_80797F68;
extern f32 lbl_80797F6C;
extern f32 lbl_80797F70;
extern f32 lbl_80797F78;
extern f32 lbl_80797F7C;
extern f32 lbl_80797F80;
extern f32 lbl_80797F84;
extern f32 lbl_80797F90;
extern f32 lbl_80797F98;
extern f32 lbl_80797F9C;
extern f32 lbl_80797FA4;
extern f32 lbl_80797FA8;
extern f32 lbl_80797FB0;
extern f32 lbl_80797FC8;
extern f32 lbl_80797FCC;
extern f32 lbl_80797FE0;
extern f32 lbl_80797FF4;
extern f32 lbl_80797FF8;
extern f32 lbl_80798000;
extern f32 lbl_80798004;
extern f32 lbl_8079800C;
extern f32 lbl_80798010;
extern f32 lbl_80798014;
extern f32 lbl_80798018;
extern f32 lbl_8079801C;
extern f32 lbl_80798020;
extern f32 lbl_80798024;
extern f32 lbl_80798028;
extern f32 lbl_8079802C;
extern f32 lbl_80798030;
extern f32 lbl_80798034;
extern f32 lbl_80798038;
extern f32 lbl_8079803C;
extern f32 lbl_80798040;
extern f32 lbl_80798044;
extern f32 lbl_80798048;
extern f32 lbl_8079804C;
extern f32 lbl_80798050;
extern f32 lbl_80798054;
extern f32 lbl_80798058;
extern f32 lbl_8079805C;
extern f32 lbl_80798060;
extern f32 lbl_80798064;
extern f32 lbl_80798068;
extern f32 lbl_8079806C;
extern f32 lbl_80798070;
extern f32 lbl_80798074;
extern f32 lbl_80798078;
extern f32 lbl_8079807C;
extern f32 lbl_80798080;
extern f32 lbl_80798084;
extern f32 lbl_80798088;
extern f32 lbl_8079808C;
extern f32 lbl_80798090;
extern f32 lbl_80798094;
extern f32 lbl_80798098;
extern f32 lbl_8079809C;
extern f32 lbl_807980A0;
extern f32 lbl_807980A4;
extern f32 lbl_807980A8;
extern f32 lbl_807980AC;
extern f32 lbl_807980B0;
extern f32 lbl_807980B4;
extern f32 lbl_807980B8;
extern f32 lbl_807980BC;
extern f32 lbl_807980C0;
extern f32 lbl_807980C4;
extern f32 lbl_807980C8;
extern f32 lbl_807980CC;
extern f32 lbl_807980D0;
extern f32 lbl_807980D4;
extern f32 lbl_807980D8;
extern f32 lbl_807980DC;
extern f32 lbl_807980E0;
extern f32 lbl_807980E4;
extern f32 lbl_807980E8;
extern f32 lbl_807980EC;
extern f32 lbl_807980F0;
extern f32 lbl_807980F4;
extern f32 lbl_807980F8;
extern f32 lbl_807980FC;
extern f32 lbl_80798100;
extern f32 lbl_80798104;
extern f32 lbl_80798108;
extern f32 lbl_8079810C;
extern f32 lbl_80798110;
extern f32 lbl_80798114;
extern f32 lbl_80798118;
extern f32 lbl_8079811C;
extern f32 lbl_80798120;
extern f32 lbl_80798124;
extern f32 lbl_80798128;
extern f32 lbl_8079812C;
extern f32 lbl_80798130;
extern f32 lbl_80798134;
extern f32 lbl_80798138;
extern f32 lbl_8079813C;
extern f32 lbl_80798140;
extern f32 lbl_80798144;
extern f32 lbl_80798148;
extern f32 lbl_8079814C;
extern f32 lbl_80798150;
extern f32 lbl_80798154;
extern f32 lbl_80798158;
extern f32 lbl_8079815C;
extern f32 lbl_80798160;
extern f32 lbl_80798164;
extern f32 lbl_80798168;
extern f32 lbl_8079816C;
extern f32 lbl_80798170;
extern f32 lbl_80798174;
extern f32 lbl_80798178;
extern f32 lbl_8079817C;
extern f32 lbl_80798180;
extern f32 lbl_80798184;
extern f32 lbl_80798188;
extern f32 lbl_8079818C;
extern f32 lbl_80798190;
extern f32 lbl_80798194;
extern f32 lbl_80798198;
extern f32 lbl_8079819C;
extern f32 lbl_807981A0;
extern f32 lbl_807981A4;
extern f32 lbl_807981A8;
extern f32 lbl_807981AC;
extern f32 lbl_807981B0;
extern f32 lbl_807981B4;
extern f32 lbl_807981B8;
extern f32 lbl_807981BC;
extern f32 lbl_807981C0;
extern f32 lbl_807981C4;
extern f32 lbl_807981C8;
extern f32 lbl_807981CC;
extern f32 lbl_807981D0;
extern f32 lbl_807981D4;
extern f32 lbl_807981D8;
extern f32 lbl_807981DC;
extern f32 lbl_807981E0;
extern f32 lbl_807981E4;
extern f32 lbl_807981E8;
extern f32 lbl_807981EC;
extern f32 lbl_807981F0;
extern f32 lbl_807981F4;
extern f32 lbl_807981F8;
extern f32 lbl_807981FC;
extern f32 lbl_80798200;
extern f32 lbl_80798204;
extern f32 lbl_80798208;
extern f32 lbl_8079820C;
extern f32 lbl_80798210;
extern f32 lbl_80798214;
extern f32 lbl_80798218;
extern f32 lbl_8079821C;
extern f32 lbl_80798220;
extern f32 lbl_80798224;
extern f32 lbl_80798228;
extern u32 lbl_805ABDB8[];
extern u32 lbl_805ABF30[];
extern u32 lbl_805AC1F0[];
extern u32 lbl_805AC3C8[];
extern u32 lbl_805AC698[];
extern u32 lbl_805AC9A8[];
extern u32 lbl_805ACC00[];
extern u32 lbl_805ACD80[];

}

/* ------------------------------------------------------------------------------------------------ *
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------ */

/* 0x8018B3B8 - `state_sub` 8 of the multi-motion dispatch: a tail call into the armed-motion step. */
void fn_8018B3B8(_ENEMY_WORK* self) {
    fn_8018479C(self);
}

/* 0x8018B3BC - `state_sub` 9: the motion-table step with id 8, no sub. */
void fn_8018B3BC(_ENEMY_WORK* self) {
    fn_80184D98(self, 8, 0);
}

/* 0x8018B3C8 - the multi-motion dispatcher keyed on the work's +0x1E6 class (0..9). */
void fn_8018B3C8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018AB94(self);
        return;
    case 1:
        fn_8018AC64(self);
        return;
    case 2:
        fn_8018ACEC(self);
        return;
    case 3:
        fn_8018AD68(self);
        return;
    case 4:
        fn_8018ADE8(self);
        return;
    case 5:
        fn_8018AE7C(self);
        return;
    case 6:
        fn_8018B1C4(self);
        return;
    case 7:
        fn_8018B258(self);
        return;
    case 8:
        fn_8018B3B8(self);
        return;
    case 9:
        fn_8018B3BC(self);
        return;
    default:
        return;
    }
}

/* 0x8018B418 - the run's opening step machine (states 0..10) on the work's +0x5 byte. */
void fn_8018B418(_ENEMY_WORK* self) {
    VEC3 sp8;

    fn_80043EA8(&sp8);
    fn_80131D84(self);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x28, 0, 0);
        fn_8014616C(self, 0);
        fn_80146058(self, lbl_80798010, lbl_80798014, lbl_80798018);
        fn_8014610C(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
        return;
    case 1:
        if (fn_80146008(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x37, 0, 0);
            fn_8014619C(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
            fn_80304508(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        if (fn_80146008(0x1DE) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 0x2C, 0x18, 0, 1);
            fn_80146058(self, lbl_80798020, lbl_80798014, lbl_80798024);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (fn_80145FE4() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            fn_80304508(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (fn_80146008(0x1FA) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x2C, 0, 0x1E);
            fn_80146058(self, lbl_80798028, lbl_8079802C, lbl_80798030);
            fn_8014610C(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
            fn_801462A4(self, fn_80145FE4(), lbl_805ABDB8, NULL, 5, 0);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (fn_80145FE4() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            fn_80304508(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            fn_80304508(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        self->pos.y = lbl_8079802C;
        fn_801462A4(self, fn_80145FE4(), lbl_805ABDB8, NULL, 5, 0);
        if (fn_80146008(0x2B2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 0x29, 0xE, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (fn_80145FE4() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            fn_80304508(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (fn_80145FE4() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            fn_80304508(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            fn_80304508(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805ABF30, 0);
        if (fn_80146008(0x382) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_801305C4(self, lbl_80797E88);
            fn_8012F504(self, 0x3C, 0xE, 0x6C, 1);
            fn_80146058(self, lbl_8079803C, lbl_80798040, lbl_80798044);
        }
        break;
    case 6:
        if (em_frame_check(self, 0, lbl_80798048, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797EA4);
            fn_80304508(self, 0xCC, 0x16, &sp8, lbl_80797E9C);
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805ABF30, 0);
        if (fn_80146008(0x3B0) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x40, 0, 0);
            fn_8012F8C8(self, lbl_8079804C);
            fn_80146058(self, lbl_80798050, lbl_80798054, lbl_80798058);
            fn_8014610C(self, lbl_80797E88, lbl_8079805C, lbl_80797E88);
            fn_801462A4(self, fn_80145FE4(), lbl_805AC1F0, NULL, 3, 0);
            fn_801048B4(self, 3U, 0x11U, 0, lbl_80797EB8);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797F10, lbl_80797E88);
            fn_80304508(self, 0x32, 3, &sp8, lbl_8079804C);
        }
        fn_801462A4(self, fn_80145FE4(), lbl_805AC1F0, NULL, 3, 0);
        if (fn_80146008(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x3D, 0, 0);
            fn_80146058(self, lbl_80798060, lbl_80798064, lbl_80798068);
            fn_8014610C(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        }
        break;
    case 8:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            fn_801048B4(self, 0x15U, 0x24U, 0, lbl_80797EC0);
        }
        if (fn_80146008(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_80146058(self, lbl_80798070, lbl_80798074, lbl_80798078);
        }
        break;
    case 9:
        if (fn_80146008(0x4A6) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 0x1A, 0xA, 0, 1);
        }
        break;
    case 10:
        if (fn_80146008(0x582) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 1, 0x1E, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BBD8 - the first arm/step pair: a two-state opener for the motion-table step. */
void fn_8018BBD8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_80798070, lbl_80798074, lbl_80798078);
        fn_8014610C(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BC7C - the run's second step machine (states 0..7). */
void fn_8018BC7C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_8079807C, lbl_80798080, lbl_80798084);
        fn_8014610C(self, lbl_80797E88, lbl_80797F44, lbl_80797E88);
        return;
    case 1:
        if (fn_80146008(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 4, 0, 0);
            return;
        }
        break;
    case 2:
        if (fn_80146008(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 5, 0, 0x2A);
            fn_80146058(self, lbl_80798088, lbl_80798080, lbl_8079808C);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F9C, lbl_80797E88) == 1U) {
            fn_801048B4(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (fn_80146008(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_80146058(self, lbl_80798094, lbl_80798098, lbl_8079809C);
            fn_8014610C(self, lbl_80797E88, lbl_807980A0, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (fn_80146008(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014616C(self, 0);
            return;
        }
        break;
    case 5:
        if (fn_80146008(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_8014619C(self);
            fn_80146058(self, lbl_807980A4, lbl_807980A8, lbl_807980AC);
            fn_8014610C(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (fn_80146008(0x4AE) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 2, 0x1E, 0, 1);
            return;
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
            fn_801048B4(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (em_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
            fn_801048B4(self, 0xDU, 0x92U, 0, lbl_80798090);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BF4C - the second arm/step pair opener. */
void fn_8018BF4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 2, 0, 0);
        fn_80146058(self, lbl_807980B4, lbl_807980A8, lbl_807980B8);
        fn_8014610C(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BFF0 - the run's third step machine (states 0..8). */
void fn_8018BFF0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
        fn_8014610C(self, lbl_80797E88, lbl_807980C8, lbl_80797E88);
        return;
    case 1:
        if (fn_80146008(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            return;
        }
        break;
    case 2:
        if (fn_80146008(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014616C(self, 0);
            return;
        }
        break;
    case 3:
        if (fn_80146008(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_8014619C(self);
            fn_80146058(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
            fn_8014610C(self, lbl_80797E88, lbl_807980CC, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (fn_80146008(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014616C(self, 0);
            return;
        }
        break;
    case 5:
        if (fn_80146008(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_8014619C(self);
            fn_80146058(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            fn_8014610C(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (fn_80146008(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x18, 0, 0);
            fn_80146058(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            fn_8014610C(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 7:
        fn_80133E3C(self, 0x4000, lbl_807980DC, lbl_807980E0);
        if (fn_80146008(0x49A) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 1, 0x1E, 0, 1);
            fn_8014610C(self, lbl_80797E88, lbl_807980E4, lbl_80797E88);
            return;
        }
        break;
    case 8:
        if (fn_80146008(0x51E) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 0x14, 0x28, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C2CC - the third arm/step pair opener. */
void fn_8018C2CC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 7, 0, 0);
        fn_80146058(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
        fn_8014610C(self, lbl_80797E88, lbl_807980E8, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C370 - the run's fourth step machine (states 0..3). */
void fn_8018C370(_ENEMY_WORK* self) {
    VEC3 sp8;

    fn_80043EA8(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_807980EC, lbl_807980F0, lbl_807980F4);
        fn_8014610C(self, lbl_80797E88, lbl_807980F8, lbl_80797E88);
        return;
    case 1:
        if (fn_80146008(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 2, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797ED0, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_807980FC, lbl_80797F34);
            fn_801049D0(self, 8, 0x16, 0, &sp8, lbl_80798100);
        }
        if (fn_80146008(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014616C(self, 0);
            return;
        }
        break;
    case 3:
        if (fn_80146008(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 0, 0);
            fn_8014619C(self);
            fn_80146058(self, lbl_80798104, lbl_80798108, lbl_8079810C);
            fn_8014610C(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}


/* 0x8018C528 - the fourth arm/step pair opener. */
void fn_8018C528(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_80798104, lbl_80798108, lbl_8079810C);
        fn_8014610C(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C5CC - the run's fifth step machine (states 0..6). */
void fn_8018C5CC(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    fn_80043EA8(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_8014616C(self, 0);
        return;
    case 1:
        if (fn_80146008(0x168) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014619C(self);
            fn_801305C4(self, lbl_80797E88);
            fn_8012F5B8(self, 0x40, 0, 0);
            fn_801461A8(self, fn_80145FE4(), lbl_805AC3C8, 0);
            fn_8014610C(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            fn_801048B4(self, 0x13U, 0xDU, 0x8000, lbl_80798090);
            fn_801048B4(self, 0x13U, 0xFU, 0, lbl_80798090);
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805AC3C8, 0);
        if (fn_80146008(0x19E) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_80130478(self, 0);
            fn_8012F504(self, 0x3B, 6, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            fn_8010D2B0(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805AC3C8, 0);
        if (fn_80146008(0x242) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 2, 0xE, 0x6A);
            return;
        }
        break;
    case 4:
        fn_801461A8(self, fn_80145FE4(), lbl_805AC3C8, 0);
        if (fn_80146008(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_80146058(self, lbl_80798114, lbl_80798118, lbl_8079811C);
            fn_8014610C(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
        }
        /* fallthrough */
    case 5:
        if (fn_80146008(0x57A) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x18, 0, 0);
            fn_8012F8C8(self, lbl_80798120);
            fn_80146058(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            fn_8014610C(self, lbl_80797E88, lbl_80798130, lbl_80797E88);
            return;
        }
        break;
    case 6:
        self->field_0x1C0 = fn_80133DB0(0x49F5, (u16)self->field_0x1C0, 0x160);
        if (fn_80146008(0x652) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x14, 0xA, 0);
            fn_80146058(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            fn_8014610C(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C998 - the fifth arm/step pair opener. */
void fn_8018C998(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x14, 0, 0);
        fn_80146058(self, lbl_80798124, lbl_80798128, lbl_8079812C);
        fn_8014610C(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CA3C - the run's sixth step machine (states 0..7). */
void fn_8018CA3C(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    fn_80043EA8(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_8014616C(self, 0);
        return;
    case 1:
        if (fn_80146008(0x1C2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014619C(self);
            fn_801305C4(self, lbl_80797E88);
            fn_8012F5B8(self, 0x40, 0, 0);
            fn_8014610C(self, lbl_80797F14, lbl_80798138, lbl_80797E88);
            fn_801462A4(self, fn_80145FE4(), lbl_805AC698, lbl_805AC9A8, 7, 3);
            fn_80133C3C(self);
            return;
        }
        break;
    case 2:
        fn_801462A4(self, fn_80145FE4(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (fn_80146008(0x1F8) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x3B, 0xC, 6);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            fn_8010D2B0(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        fn_801462A4(self, fn_80145FE4(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (fn_80146008(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_80146058(self, lbl_8079813C, lbl_80797ED0, lbl_80798140);
            fn_8014610C(self, lbl_80797E88, lbl_80797F70, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (fn_80146008(0x29C) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 2, 0x14, 0, 1);
            return;
        }
        break;
    case 5:
        if (fn_80146008(0x566) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x19, 0, 0x14);
            fn_80146058(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            fn_8014610C(self, lbl_80797E88, lbl_80798150, lbl_80797E88);
            return;
        }
        break;
    case 6:
        fn_80133E3C(self, -0x4000, lbl_80797ED0, lbl_80798154);
        if (fn_80146008(0x5B8) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 1, 0xA, 0, 1);
            fn_80146058(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            fn_8014610C(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
            return;
        }
        break;
    case 7:
        if (fn_80146008(0x6F2) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 0x14, 0xE, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018CDE0 - the sixth arm/step pair opener. */
void fn_8018CDE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x14, 0, 0);
        fn_80146058(self, lbl_80798144, lbl_80798148, lbl_8079814C);
        fn_8014610C(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CE84 - the run's seventh step machine (states 0..5). */
void fn_8018CE84(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    fn_80043EA8(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_8014616C(self, 0);
        return;
    case 1:
        if (fn_80146008(0x4A8) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8014619C(self);
            fn_801305C4(self, lbl_80797E88);
            fn_8012F5B8(self, 0x40, 0, 0);
            fn_8014610C(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
            fn_801461A8(self, fn_80145FE4(), lbl_805ACC00, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            fn_801048B4(self, 0x13U, 0x10U, 0, lbl_8079804C);
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805ACC00, 0);
        if (fn_80146008(0x4DE) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 0x3B, 8, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            fn_8010D2B0(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        if (fn_80146008(0x4E6) == 0) {
            fn_801461A8(self, fn_80145FE4(), lbl_805ACC00, 0);
        } else {
            fn_801462A4(self, fn_80145FE4(), lbl_805ACD80, NULL, 5, 0);
        }
        if (fn_80146008(0x584) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 2, 0, 0);
            fn_80146058(self, lbl_8079815C, lbl_80798160, lbl_80798164);
            return;
        }
        break;
    case 4:
        if (fn_80146008(0x662) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F504(self, 1, 0xA, 0, 1);
            fn_80146058(self, lbl_80798168, lbl_8079816C, lbl_80798170);
            return;
        }
        break;
    case 5:
        if (fn_80146008(0x768) == 1U) {
            self->state = (u8)(self->state + 1);
            fn_8012F5B8(self, 1, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018D1AC - the seventh arm/step pair opener. */
void fn_8018D1AC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_80146058(self, lbl_80798168, lbl_8079816C, lbl_80798170);
        fn_8014610C(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018D250 - the `state_sub` class dispatcher (0..13) into this unit's twelve step machines. */
void fn_8018D250(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018B418(self);
        return;
    case 1:
        fn_8018BBD8(self);
        return;
    case 2:
        fn_8018BC7C(self);
        return;
    case 3:
        fn_8018BF4C(self);
        return;
    case 4:
        fn_8018BFF0(self);
        return;
    case 5:
        fn_8018C2CC(self);
        return;
    case 6:
        fn_8018C370(self);
        return;
    case 7:
        fn_8018C528(self);
        return;
    case 8:
        fn_8018C5CC(self);
        return;
    case 9:
        fn_8018C998(self);
        return;
    case 10:
        fn_8018CA3C(self);
        return;
    case 11:
        fn_8018CDE0(self);
        return;
    case 12:
        fn_8018CE84(self);
        return;
    case 13:
        fn_8018D1AC(self);
        return;
    default:
        return;
    }
}

/* 0x8018D2B0 - the action dispatcher (0..13) plus the two shared post-checks. */
void fn_8018D2B0(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80183DCC(self);
        break;
    case 1:
        fn_80184488(self);
        break;
    case 2:
        fn_80184BF8(self);
        break;
    case 5:
        fn_801861E4(self);
        break;
    case 6:
        fn_80186960(self);
        break;
    case 7:
        fn_80189C7C(self);
        break;
    case 10:
        fn_8018A974(self);
        break;
    case 11:
        fn_8018AB64(self);
        break;
    case 12:
        fn_8018B3C8(self);
        break;
    case 13:
        fn_8018D250(self);
        break;
    }
    if (self->team != 0x15 && self->field_0x1E2 == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
}

/* 0x8018D370 - the per-frame effect spawner: measures the joint point against the water/height
 * gates and picks one of the four ground/air effect ids. */
void fn_8018D370(_ENEMY_WORK* self) {
    VEC3 sp8;
    MTX34 sp18;
    u16 ang;
    u16 mot;

    fn_80043EA8(&sp8);
    fn_8005050C(&sp18);
    if (fn_8012EC60(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            mot = em_get_mot_no(self);
            if (mot != 0x4A && mot != 0x57 && mot != 0x5B && mot != 0x63 && mot != 0xC8) {
                setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
                get_joint_wmat_em(self, 0x16, &sp18);
                mulVecMat(&sp8, &sp18);
                sp8.x += sp18.m[0][3];
                sp8.y += sp18.m[1][3];
                sp8.z += sp18.m[2][3];
                if (em_water_check(self) == 1U && sp8.y < self->field_0x210) {
                    if ((system_w.field_0x0c & 0x1F) == 0) {
                        if ((u16)(ang + 0x8000) > 0x6E38U) {
                            fn_8010562C(self, 3, 0x16, &sp8, lbl_8079800C);
                            return;
                        }
                        fn_8010562C(self, 5, 0x16, &sp8, lbl_8079800C);
                    }
                } else if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38U) {
                        fn_8010562C(self, 0x1D, 0x16, &sp8, lbl_80797E9C);
                        return;
                    }
                    fn_8010562C(self, 0x1C, 0x16, &sp8, lbl_80797E9C);
                }
            }
        }
    }
}

/* 0x8018D558 - the effect-spawn router: builds the spawn position from the work (or a joint) and
 * spawns through `eft009_set_pos` (a fixed spawn point) or `fn_801048B4` (a joint spawn).  `arg1`
 * selects the position/height source (0 = the +0x210 height, 1 = a joint, 2 = the +0x20C height),
 * `arg2` the effect kind, `arg3` the joint (0xFF = the work's own position), `arg4` the joint flag
 * and `arg5` the scale. */
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    VEC3 vec;
    u8 var;

    var = arg2;
    fn_80043EA8(&vec);
    switch (arg1) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (arg2) {
            case 2:
                var = 0xC;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    fn_801048B4(self, arg3, 0xE, arg4, arg5);
                }
                break;
            case 4:
            case 6:
                var = 0x13;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xF, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    fn_801048B4(self, arg3, 0xF, arg4, arg5);
                }
                break;
            default:
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)(arg2 - 12) <= 13) {
                return;
            }
            if (arg2 == 0x26) {
                return;
            }
            if (arg3 == 0xFF) {
                vec.x = self->pos.x;
                vec.y = lbl_80797EE8 + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (arg3 == 0xFF) {
            eft009_set_pos(var, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
            return;
        }
        fn_801048B4(self, arg3, var, arg4, arg5);
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            if (arg2 == 0 || arg2 == 3) {
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x11, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                    return;
                }
                fn_801048B4(self, arg3, 0x11, arg4, arg5);
                return;
            }
            return;
        }
        if (arg3 == 0xFF) {
            fn_80041E40(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        fn_8010D2B0(&vec, self->area_no, arg2, arg4, arg5 * get_em_chg_scale(self));
        return;
    case 2:
        if (arg3 == 0xFF) {
            fn_80041E40(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        {
            f32 scale = arg5 * get_em_chg_scale(self);
            if (self->pos.y <= lbl_80798174 + self->field_0x20C) {
                fn_80106694(self, &vec, arg2, scale);
            }
        }
        return;
    default:
        return;
    }
}

/* 0x80191038 - the team-0x10 TEV tint stepper: fades the two +0x350/+0x352 highlight words and the
 * +0x354..+0x358 fade triple toward the team's key colour. */
void fn_80191038(_ENEMY_WORK* self) {
    _GXColor color;
    s16 v;
    u8 flag;

    if (self->team == 0x10) {
        flag = (self->field_0x1E2 - 2) == 0;
        if ((fn_8012EC3C(self) - 1) == 0) {
            v = self->tev_0x350 - 2;
            self->tev_0x350 = v;
            if (v < 0xAA) {
                self->tev_0x350 = 0xAA;
            }
            v = self->tev_0x352 - 2;
            self->tev_0x352 = v;
            if (v < 0xB4) {
                self->tev_0x352 = 0xB4;
            }
        } else {
            v = self->tev_0x350 + 2;
            self->tev_0x350 = v;
            if (v > 0xFF) {
                self->tev_0x350 = 0xFF;
            }
            v = self->tev_0x352 + 2;
            self->tev_0x352 = v;
            if (v > 0xFF) {
                self->tev_0x352 = 0xFF;
            }
        }
        v = self->tev_0x350;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
        v = self->tev_0x352;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
        if (flag == 1) {
            v = self->tev_0x354 + 7;
            self->tev_0x354 = v;
            if (v > 0xFF) {
                self->tev_0x354 = 0xFF;
            }
            v = self->tev_0x356 + 6;
            self->tev_0x356 = v;
            if (v > 0xFF) {
                self->tev_0x356 = 0xFF;
            }
            v = self->tev_0x358 + 6;
            self->tev_0x358 = v;
            if (v > 0xFF) {
                self->tev_0x358 = 0xFF;
            }
        } else {
            v = self->tev_0x354 - 7;
            self->tev_0x354 = v;
            if (v < 0x28) {
                self->tev_0x354 = 0x28;
            }
            v = self->tev_0x356 - 6;
            self->tev_0x356 = v;
            if (v < 0x3C) {
                self->tev_0x356 = 0x3C;
            }
            v = self->tev_0x358 - 6;
            self->tev_0x358 = v;
            if (v < 0x3C) {
                self->tev_0x358 = 0x3C;
            }
        }
        color.r = (u8)self->tev_0x354;
        color.g = (u8)self->tev_0x356;
        color.b = (u8)self->tev_0x358;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
        if (fn_80135748(self, 1) == 1U) {
            if (self->tev_0x35E == 1) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xFF;
                color.a = 0;
            } else {
                color.r = 0x28;
                color.g = 0x3C;
                color.b = 0x3C;
                color.a = 0;
            }
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
        } else {
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
            self->tev_0x35E = flag;
        }
        ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
        if (fn_8012EC3C(self) == 1U) {
            color.a = 0;
        } else if (fn_8012EC60(self) == 1U) {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0xE1;
        } else {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0x87;
        }
        ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    }
}

/* 0x801913FC - per-(team,kind) "the effect may fire" predicate: 0 = no, 1 = the main window,
 * 2 = the secondary window. */
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1) {
    f32 d;
    f32 scale;

    switch (self->team) {
    case 16:
        switch (arg1) {
        case 0:
            d = self->vec_0x36C.y - self->pos.y;
            if (d >= lbl_80797EA0) {
                return 1;
            }
            if (d <= lbl_80797EE4) {
                return 2;
            }
            return 0;
        case 1:
            if (self->tev_0x35A <= 0) {
                return 1;
            }
            break;
        case 2:
            if (self->field_0x1E2 == 2) {
                scale = get_em_chg_scale(self);
                if (self->pos.y >= self->field_0x210 - (lbl_80797ED0 + fn_8013026C(self)) * scale) {
                    return 1;
                }
            }
            break;
        case 5:
            if (self->tev_0x35C > 0) {
                return 1;
            }
            break;
        }
        break;
    case 17:
        if (arg1 == 4 && self->field_0x011 != 0) {
            return 1;
        }
        break;
    case 21:
        switch (arg1) {
        case 3:
            if (fn_801321DC(self) == 1U) {
                return 1;
            }
            break;
        case 4:
            if (self->field_0x011 != 0) {
                return 1;
            }
            break;
        }
        break;
    }
    return 0;
}

/* 0x8018D8C8 - the per-(team,motion) frame-window driver.  For the work's team it reads the motion
 * number `em_get_mot_no(self)` and runs that motion's set of `em_after_frame_check` windows; each
 * window that fires calls `fn_8018D558` (the effect-spawn router) with the window's id/kind, or
 * `fn_8011D448`/`fn_8011D4FC`/`fn_8011D690`/`fn_801E2D04`/`fn_80304508` directly.  The `field_0x228 & 6`
 * bit pair selects the "large/air" variant of each window; the team-0x10 tail runs `fn_80191EF8`
 * and `fn_80192080`. */
void fn_8018D8C8(_ENEMY_WORK* self) {
    VEC3 sp8;
    VEC3 sp14;
    VEC3 sp20;
    VEC3 sp2C;
    f32 temp_f1;
    u16 temp_r3;
    u16 temp_r3_2;
    u16 temp_r3_3;
    u8 temp_r0;

    fn_80043EA8(&sp2C);
    fn_80043EA8(&sp20);
    fn_80043EA8(&sp14);
    temp_r0 = self->team;
    switch ((s32) temp_r0) {                        /* switch 1; irregular */
    case 16:                                        /* switch 1 */
        fn_8018D370(self);
        temp_r3 = em_get_mot_no(self);
        switch (temp_r3) {                          /* switch 2 */
        case 0x2:                                   /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807980E0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            break;
        case 0x9:                                   /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            break;
        case 0x15:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797EEC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797FC8, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x16:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x18:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x19:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x1B:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            break;
        case 0x2D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981A4, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                fn_80304508(self, 0x46, 0x15, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079804C);
            }
            break;
        case 0x33:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_80129668(self, 0, 9);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797F38, lbl_80797E88, lbl_80797E88);
                fn_801E2D04(self, 8, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                fn_801E2D04(self, 8, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x34:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_80129668(self, 0, 0xA);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_807981B4, lbl_80797E88, lbl_80797E88);
                fn_801E2D04(self, 0xD, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                fn_801E2D04(self, 0xD, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x39:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8011D448(self, 0, 0x26, 8, lbl_8079800C);
                sp14.x = lbl_80797E88;
                sp14.y = lbl_807981B8;
                sp14.z = lbl_80797F5C;
                shell_set_func_ptr->field_0x2c(self, 3, 1, &sp14, 0xFFFF, shell_set_func_ptr, (void*)&lbl_807981BC);
            }
            break;
        case 0x3B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x3D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x3E:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x1DU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x1DU, self->field_0x1C0, lbl_8079804C);
                }
            }
            break;
        case 0x43:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0x10, &sp14, 0x1E, lbl_8079800C);
                fn_801E2D04(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x46:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x47:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x49:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x4D:                                  /* switch 2 */
            if ((s32) (self->flags_0x836 & 1) == 0) {
                if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 8, 0x28, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 9, 0x29, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_807981C8 * get_em_chg_scale(self));
                    fn_80117E58(self, 1, &sp14, 3, lbl_8079800C * get_em_chg_scale(self));
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797F58 * get_em_chg_scale(self));
                    rotVecY(&sp14, self->field_0x1C0);
                    fn_80051378(&sp8, &self->pos, &sp14);
                    fn_80041E40(&sp2C, fn_80041E40(&sp20, &sp8));
                    temp_f1 = lbl_807981CC * get_em_chg_scale(self);
                    sp2C.y -= temp_f1;
                    sp20.y += lbl_807981CC * get_em_chg_scale(self);
                    shell_set_func_ptr->field_0x30(self, 4, &sp2C, &sp20, 0xFFFF, shell_set_func_ptr, (void*)&lbl_807981BC);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                fn_80136B50(self, 0x28, 7);
            }
            break;
        case 0x4E:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                fn_801E2D04(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x4F:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F08, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                fn_801E2D04(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x50:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x51:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0xE000, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0xE000, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            break;
        case 0x52:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981DC, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x55:                                  /* switch 2 */
        case 0x56:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x57:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x58:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xEU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xEU, 0, lbl_8079800C);
                }
                fn_80136B50(self, 0xE, 1);
            }
            break;
        case 0x59:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 8U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 8U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x5B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981F0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            break;
        case 0x5C:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x26U, 0, lbl_80797EF8);
                    fn_8018D558(self, 0U, 0xFU, 0x26U, 0, lbl_80797EF8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x26U, 0, lbl_80797EF8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x11U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x11U, self->field_0x1C0, lbl_80797EB8);
                }
                fn_80136B50(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_80797FE0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797FCC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x5E:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                fn_80136B50(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            break;
        case 0x5F:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                fn_80136B50(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            break;
        case 0x60:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 0xDU, 0x2AAB, lbl_80797F0C);
            }
            break;
        case 0x61:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 8U, 0xD555, lbl_80797F0C);
            }
            break;
        case 0x62:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797EC8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x28U, 0, lbl_80798090);
                    fn_8018D558(self, 0U, 0xFU, 0x28U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x28U, 0, lbl_80798090);
                }
            }
            break;
        case 0x63:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x6F:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981F8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981FC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                fn_80304508(self, 0x46, 3, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798200, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                fn_801E2D04(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x79:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x7A:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0x4000, lbl_80797F0C);
            }
            break;
        case 0x7B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7C:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x7D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0xC000, lbl_80797F0C);
            }
            break;
        case 0x7E:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798134, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7F:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x81:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x11U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x11U, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x82:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80798090);
            }
            break;
        case 0x83:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798208, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x10U, 0xFU, 0, lbl_807981E8);
                } else {
                    fn_8018D558(self, 1U, 1U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                }
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x84:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x88:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 2U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0x89:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x8B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 0xFU, self->field_0x1C0, lbl_80797E9C);
            }
            break;
        case 0xC8:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798210, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                fn_80136B50(self, 8, 0);
            }
            if ((em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798214, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798218, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                fn_80136B50(self, 0xD, 0);
            }
            break;
        case 0xC9:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 4U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCA:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                fn_80136B50(self, -1, 7);
            }
            break;
        case 0xCB:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCC:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        }
        break;
    case 17:                                        /* switch 1 */
        temp_r3_2 = em_get_mot_no(self);
        switch ((s32) temp_r3_2) {                  /* switch 3; irregular */
        case 0x3B:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x10U, self->field_0x1C0, lbl_80797EC0);
                }
            }
            break;
        case 0x3C:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_8079804C);
            }
            break;
        case 0x3E:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x17U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x3F:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                fn_8018D558(self, 0x27U, 0x30U, 3U, 0, lbl_80797EB8);
            }
            break;
        case 0x5C:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x17U, 0x8000, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x17U, 0x8000, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x10U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x10U, 0x8000, lbl_80797E9C);
                }
            }
            break;
        case 0x81:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xC9:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xCU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xCU, 0, lbl_8079804C);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 7U, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 7U, 0, lbl_8079804C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_8079821C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797F0C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797ECC, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_807981E8);
                }
            }
            break;
        }
        break;
    case 21:                                        /* switch 1 */
        temp_r3_3 = em_get_mot_no(self);
        switch ((s32) temp_r3_3) {                  /* switch 4; irregular */
        case 0x70:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                fn_801048B4(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            fn_80136D14(self);
            break;
        case 0x73:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_8079818C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, self->field_0x1C0, lbl_80798220);
                }
            }
            break;
        case 0x81:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xCD:                                  /* switch 4 */
        case 0xC9:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80798224, lbl_80797E88) == 1U) {
                fn_801048B4(self, 0x13U, 0x67U, 0, lbl_807981BC * get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                fn_801048B4(self, 0x13U, 0x68U, 0, lbl_807981BC * get_em_scale(self));
            }
            if ((em_after_frame_check(self, 3, lbl_80797F08, lbl_80798194) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                fn_801048B4(self, 0x13U, 0x69U, 0, lbl_807981BC * get_em_scale(self));
            }
            break;
        case 0xD0:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                fn_801048B4(self, 0x13U, 0x66U, 0, get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                fn_801048B4(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            break;
        case 0xD2:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 3U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80798228);
                }
            }
            break;
        }
        break;
    }
    if ((u8) self->team == 0x10) {
        fn_80191EF8(self);
        fn_80192080(self);
    }
}

