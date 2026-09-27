/* enemy/fn_801502C8.cpp - the enemy em00x action band, `.text` 0x801502C8..0x801550FC (30 functions).
 *
 * what it is.  One band of an enemy's action/state family, the same kind as the registered
 * `enemy/fn_80147CE0.cpp` below (0x80147CE0..0x80149D6C) and `enemy/fn_801550FC.cpp` above: every
 * function takes the shared `_ENEMY_WORK`, switches on its `state` (+0x05) / `action` (+0x1E5) /
 * `state_sub` (+0x1E6), and drives the motion/effect helpers (`em_frame_check`, `em_after_frame_check`,
 * `em_get_mot_no`, `get_joint_wpos_em`, `get_em_chg_scale`).  The two dispatchers `fn_80151074` /
 * `fn_801510C4` are the band's tail-call tables; `fn_801514BC` is its 0x2CC8-byte case table (a
 * residual, see below).
 *
 * module and name (brief section 2, evidence order).
 *   1. No `__FILE__` string covers the range.  The only `.data`/`.rodata` the range builds are the
 *      `.sdata2` pool, the three `jumptable_*` (`805A30B0`/`805A4064`/`805A408C`), the `.rodata`
 *      parameter tables `lbl_805A2BD8`/`2D68`/`2F28`/`30DC`/`3280`/`3860`/`3AE0`/`3C10`/`3DD0`/`3E90`/
 *      `3FC0`, and the vtable `lbl_805A5D18` - none is a source-file name.  (Discovery notes a
 *      candidate seam inside `enemy_control.cpp`, whose `__FILE__` string at 0x805A1BB8 is referenced
 *      only by the range below; the `tu` verdict is `unproven`, so this unit registers separately.)
 *   2. `python tools/symbols/dumpmap.py lookup` answers `zz_01502c8_` for the range's addresses (a
 *      `zz_` placeholder is not evidence).
 *   3. The code is enemy-band: every function takes the `_ENEMY_WORK` (pinned by the callee
 *      `em_after_frame_check__FP11_ENEMY_WORKUsff`); both bracketing registered units are `enemy`
 *      (`enemy/fn_8014A1BC.c` below, `enemy/fn_801550FC.cpp` above) and the module's naming scheme is
 *      the map's own `fn_XXXXXXXX` stem.
 * The file therefore keeps the map stem (brief option 4); no name was invented.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address of the range answers `zz_XXXXXXXX_` and
 * carries a bare `fn_XXXXXXXX = .text:0x...` entry in config/RMHE08/symbols.txt; no `__FILE__` string
 * is reachable from the range).
 *
 * language.  C++: the range's callees are C++ manglings (`em_after_frame_check__FP11_ENEMY_WORKUsff`,
 * `em_get_mot_no__FP11_ENEMY_WORK`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `calcVecAngX__FPQ34nw4r4math4VEC3`, `setVector3__FPQ34nw4r4math4VEC3fff`, `__nw__FUl`/`__dl__FPv`).
 * Rule 9: those are declared at C++ scope with the signature their mangling encodes and called through
 * it; every `fn_*` definition stays `extern "C"`.
 *
 * seam.  Unproven (the brief's own `tu` verdict).  The left edge at 0x801502C8 is where the registered
 * `enemy/fn_8014A1BC.c` ends and its extab/extabindex runs end exactly where this unit's begin
 * (extab 0x8000DAD4..0x8000DC54, extabindex 0x80028728..0x80028968); the right edge at 0x801550FC is
 * exact (the registered `enemy/fn_801550FC.cpp` starts there).
 *
 * sections.  `.text` 0x801502C8..0x801550FC (0x4E34 B), `extab` 0x8000DC54..0x8000DD14, `extabindex`
 * 0x80028968..0x80028A88, and the `.ctors` word 0x8056F320..0x8056F324.  The last is this unit's own
 * static constructor: its stored entry is `fn_80154D44` (the six-vector seeder above), the word the
 * linker placed between `enemy/enemy_control.cpp`'s `.ctors` (..0x8056F320) and
 * `enemy/fn_801550FC.cpp`'s (0x8056F324..); `dtk dol split` attributed it to this unit and the full
 * worktree `ninja` then linked `main.dol: OK` with it claimed.
 *
 * residuals (not written this pass):
 *   fn_801514BC (0x2CC8, 11464 B) - the band's giant case table over `jumptable_805A4104` (0xDC+1
 *     cases, each a short `em_after_frame_check`/`fn_801512E8` sequence); reconstructing it needs the
 *     whole table read out of the DOL case by case.
 *   fn_801544F0 (0xC8) - reads a 32-bit pointer at `_ENEMY_WORK` +0x04, which the shared header types
 *     as the byte `field_0x004` (`em_work_die_ck`'s aliveness byte); the two views of +0x04 are a
 *     rule-1 header follow-up, not this unit's to force.
 * The measurement for each is in the worker's outbox.
 *
 * shape and residuals, measured (`recompile.py --measure`, the official report metric).  26 of the 28
 * written functions are at or above the 80 % bar, 12 of them byte-identical (`fn_801502C8`,
 * `fn_80150728`, `fn_80150FCC`, `fn_80151074`, `fn_801510C4`, `fn_801545B8`, `fn_80154784`,
 * `fn_80154928`, `fn_8015497C`, `fn_80154988`, `fn_80154CA4`, `fn_80154D44`).  The file default is
 * `#pragma peephole off` - like `enemy/fn_801DB8E0.cpp`, most of the band keeps the unfused
 * `clrlwi`+`cmpwi`/`extsh` forms the peephole pass folds - with a scoped `#pragma peephole on`
 * around the five functions that are the other way (`fn_801502C8` 96.43->100, `fn_80150728`
 * 97.56->100, `fn_801507CC` 96.88->99.79, `fn_80150FCC` 97.62->100, `fn_801545B8` 93.44->100).
 * Two partials sit below the bar and keep their best-scoring shape:
 *   fn_80154184 66.02 % (332 B vs 332 B): the instruction multiset is identical (both the rgb and the
 *     alpha `u32 -> f32 -> int` conversions are there); only the `-O3` scheduling of the call's
 *     argument setup differs (we hoist `lfs f1, 0x1d4(self)` and the `addi r3/li r4/li r5` triple
 *     above the conversion, retail keeps them after it).  Tried: two named vars (same), a `u32 rgba[2]`
 *     array (same codegen), the value-in-both-branches shape (same).
 *   fn_801542D0 76.92 % (544 B vs 524 B): the flow is the surviving shape - `fn_80129DB8`'s
 *     case 1 arms and case 2 returns, then the `armed == 0` `fn_80129E48` test, then the arg3 pick
 *     and `fn_8012A014`.  Retail's `range`-switch keeps four separate `cmpwi`s where MWCC here folds
 *     `case 1/2/3` into a `(kind-1) <= 2` range test (the exact cause of the 20 B gap); the
 *     if-chain that would keep them separate scores 72.79 %, so the folded switch is kept.
 */

#pragma peephole off

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_80147CE0.h" /* EmSpawnRec + fn_80147E2C (the owner's header) */
#include "enemy/fn_801502C8.h" /* this unit's own declarations (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * the band's callees, declared with the signatures their call sites set (rule 2: the plain prototypes
 * are not `extern` declarations of another unit's symbol; the owners' headers carry the canonical
 * spellings and are a follow-up).
 * -------------------------------------------------------------------------------------------------- */

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) - the action/motion arming helpers. */
void fn_8012CF20(struct _ENEMY_WORK* self);
void fn_8012F504(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c, s32 d);
void fn_8012F5B8(struct _ENEMY_WORK* self, s32 motion, s32 arg2, s32 arg3);
u32 fn_8012F93C(struct _ENEMY_WORK* self);
void fn_8012FCC4(struct _ENEMY_WORK* self, s32 a, f32 b);
void fn_80130248(struct _ENEMY_WORK* self);
void fn_801303FC(f32 a);
void fn_801305C4(struct _ENEMY_WORK* self);
void fn_80130CDC(s32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_801369A0(struct _ENEMY_WORK* self, u8 a, s32 b, void* c, f32 d);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 fn_8012EC60(struct _ENEMY_WORK* self);
u32 fn_8012ECF0(struct _ENEMY_WORK* self);
s16 fn_80145FE4(void);
u32 fn_80146008(s32 label);
void fn_80146058(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014610C(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014616C(struct _ENEMY_WORK* self, s32 a);
void fn_8014619C(struct _ENEMY_WORK* self);
void fn_801461A8(struct _ENEMY_WORK* self, s16 a, void* b, void* c);
void fn_801462A4(struct _ENEMY_WORK* self, s16 a, void* b, void* c, s32 d, s32 e);
void fn_80129668(struct _ENEMY_WORK* self, s32 a, s32 b);
u32 fn_8012A014(struct _ENEMY_WORK* self, s32 a, s32 b, u16 c, void* d, void* e);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);u32 fn_80129D3C(struct _ENEMY_WORK* self);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u8 fn_80129E48(struct _ENEMY_WORK* self);
s32 fn_8012A204(struct _ENEMY_WORK* self);
u8 fn_802B0668(u8 a);
void fn_80127FE4(struct _ENEMY_WORK* self);

/* enemy/fn_80147CE0.cpp (0x80147CE0..0x80149D6C) - the band below. */
void fn_801484D4(struct _ENEMY_WORK* self);
void fn_80149004(struct _ENEMY_WORK* self);
void fn_80149814(struct _ENEMY_WORK* self);
void fn_8014AA4C(struct _ENEMY_WORK* self);
void fn_8014B5C0(struct _ENEMY_WORK* self);
void fn_8014E3AC(struct _ENEMY_WORK* self);
void fn_8014EA70(struct _ENEMY_WORK* self);
void fn_8014F030(struct _ENEMY_WORK* self);
void fn_8014F138(struct _ENEMY_WORK* self);
void fn_8014F530(struct _ENEMY_WORK* self);
void fn_8014F5DC(struct _ENEMY_WORK* self);
void fn_8014F5F0(struct _ENEMY_WORK* self);
void fn_8014F71C(struct _ENEMY_WORK* self);
void fn_8014FA34(struct _ENEMY_WORK* self);
void fn_8014FC24(struct _ENEMY_WORK* self);
void fn_8014FE00(struct _ENEMY_WORK* self);
void fn_8014FF10(struct _ENEMY_WORK* self);

/* the ef/effect helpers the range drives. */
void fn_801048B4(struct _ENEMY_WORK* self, s32 a, u8 b, s32 c, f32 d);
void fn_8010562C(struct _ENEMY_WORK* self, u32 a, u32 b, void* pos, f32 c);
void fn_801057A4(struct _ENEMY_WORK* self, u32 a, void* pos, s32 b, f32 c);
void fn_80106694(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void fn_8010D2B0(void* pos, u8 a, u8 b, s32 c, f32 d);

/* the runtime helpers the range reaches. */
f32 fn_80050EF4(const void* a, const void* b);
void fn_80051378(void* out, const void* a, const void* b);
void fn_80051490(void* dst, s32 src);
void fn_800516F0(void* out);
void fn_80056A54(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_800E2F40(void* self, s32 a, s32 b, u8 c, s32 d, s32 e, u8 f, f32 g);
void fn_801390FC(void* self, s32 helper);
s32 fn_801391E8(void* self);
void fn_8013918C(void* self, s32 a);
void fn_80304508(struct _ENEMY_WORK* self, s32 a, s32 b, void* c, f32 d);
void fn_805012E8(void* a, void* b);

#ifdef __cplusplus
}
#endif

/* the C++-mangled callees (rule 9: declared with the real signature, called through it). */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u8 em_magma_check(struct _ENEMY_WORK* self);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
u16 calcVecAngX(nw4r::math::VEC3* v);
s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

/* the shell callback table `shell_set_func_ptr` points at; only its +0x28 entry is used by this unit
 * (the entry `fn_801513FC` calls as `(self, &pos, mode, id, table, scale)`).  Same record as
 * `enemy/fn_80147CE0.cpp`'s private `EmShellSetFunc` (rule-1 follow-up: the copies should move to one
 * header). size: 0x40 */
typedef struct EmShellSetFuncTbl {
    /* +0x00 */ u8 unused_0x00[0x28];
    /* +0x28 */ void (*field_0x28)(struct _ENEMY_WORK* self, void* pos, s32 mode, s32 id,
                                   void* table, f32 scale);
    /* +0x2C */ u8 unused_0x2C[0x3C - 0x2C];
    /* +0x3C */ void (*field_0x3c)(struct _ENEMY_WORK* self, void* rec, u32 mode, void* table);
} EmShellSetFuncTbl;

/* the table pointer itself (`.sbss`, no registered range) */
extern EmShellSetFuncTbl* shell_set_func_ptr;

/* ----------------------------------------------------------------------------------------------------
 * the pool the range reads (owned elsewhere; declared, never defined)
 * -------------------------------------------------------------------------------------------------- */

extern f32 lbl_80796E08;
extern f32 lbl_80796E0C;
extern f32 lbl_80796E14;
extern f32 lbl_80796E18;
extern f32 lbl_80796E1C;
extern f32 lbl_80796E20;
extern f32 lbl_80796E38;
extern f32 lbl_80796E3C;
extern f32 lbl_80796E44;
extern f32 lbl_80796E54;
extern f32 lbl_80796E58;
extern f32 lbl_80796E60;
extern f32 lbl_80796F18;
extern f32 lbl_80796F2C;
extern f32 lbl_80796F40;
extern f32 lbl_80796F44;
extern f32 lbl_80796F48;
extern f32 lbl_80796F84;
extern f32 lbl_80796F88;
extern f32 lbl_80796FB8;
extern f32 lbl_80796FBC;
extern f32 lbl_80796FC0;
extern f32 lbl_80796FC4;
extern f32 lbl_80796FC8;
extern f32 lbl_80796FCC;
extern f32 lbl_80796FD0;
extern f32 lbl_80796FD4;
extern f32 lbl_80796FD8;
extern f32 lbl_80796FDC;
extern f32 lbl_80796FE0;
extern f32 lbl_80796FE4;
extern f32 lbl_80796FE8;
extern f32 lbl_80796FEC;
extern f32 lbl_80796FF0;
extern f32 lbl_80796FF4;
extern f32 lbl_80796FF8;
extern f32 lbl_80796FFC;
extern f32 lbl_80797000;
extern f32 lbl_80797004;
extern f32 lbl_80797044;
extern f32 lbl_80797074;
extern f32 lbl_80797098;
extern f32 lbl_8079709C;
extern f32 lbl_807970A0;
extern f32 lbl_807970A4;
extern f32 lbl_807970A8;
extern f32 lbl_807970AC;
extern f32 lbl_807970B0;
extern f32 lbl_807970B4;
extern f32 lbl_807970B8;
extern f32 lbl_807970C0;
extern f32 lbl_807970C4;
extern f32 lbl_807970C8;
extern f32 lbl_807970CC;

/* this range's own `.rodata` parameter tables and `.data` vtable (no `.data` range is registered for
 * the unit yet, so they stay the shared pool's bytes; only the ones the code loads are declared). */
extern u8 lbl_805A2BD8[];
extern u8 lbl_805A20C8[];
extern u8 lbl_805A20D4[];
extern u8 lbl_805A2D68[];
extern u8 lbl_805A2F28[];
extern u8 lbl_805A30DC[];
extern u8 lbl_805A3280[];
extern u8 lbl_805A3860[];
extern u8 lbl_805A3AE0[];
extern u8 lbl_805A3C10[];
extern u8 lbl_805A3DD0[];
extern u8 lbl_805A3E90[];
extern u8 lbl_805A3FC0[];
extern u8 lbl_805A5D18[];
extern u8 lbl_806A77D8[];
extern u8 lbl_806A77F0[];
extern u8 lbl_806A7808[];

#ifdef __cplusplus
extern "C" {
#endif

/* this unit's own forward declarations (the definitions follow in address order). */
void fn_80154F70(void** self);

/* ----------------------------------------------------------------------------------------------------
 * functions, in address order
 * -------------------------------------------------------------------------------------------------- */

#pragma peephole on

/* 0x801502C8 (0x460) - the 11-state action step (state at +0x05): arm the motion, wait out each
 * effect window through `fn_80146008`/`em_frame_check`, and re-arm the next motion at each
 * transition. */
void fn_801502C8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 0, 0);
        fn_8014616C(self, 0);
        return;
    case 1:
        if (fn_80146008(0x1D6) == 1) {
            self->state = self->state + 1;
            fn_80146058(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
            fn_8014610C(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
            fn_8014619C(self);
        }
        return;
    case 2:
        if (fn_80146008(0x2DA) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x3D, 0, 0);
            fn_801461A8(self, fn_80145FE4(), lbl_805A2BD8, 0);
            fn_8014610C(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        }
        return;
    case 3:
        fn_801461A8(self, fn_80145FE4(), lbl_805A2BD8, 0);
        if (em_after_frame_check(self, 0, lbl_80796F18, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            fn_8014616C(self, 0);
        }
        return;
    case 4:
        fn_801461A8(self, fn_80145FE4(), lbl_805A2BD8, 0);
        if (fn_80146008(0x3A2) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0xC8, 0, 0x1E);
            fn_8014619C(self);
            fn_801461A8(self, fn_80145FE4(), lbl_805A2D68, 0);
            fn_8014610C(self, lbl_80796E1C, lbl_80796FC4, lbl_80796E1C);
        }
        return;
    case 5:
        if (em_frame_check(self, 0, lbl_80796F44, lbl_80796E1C) == 1) {
            fn_80129668(self, 0, 0x17);
        }
        fn_801461A8(self, fn_80145FE4(), lbl_805A2D68, 0);
        if (fn_80146008(0x41A) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x13, 0, 0);
            fn_80146058(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            fn_8014610C(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 6:
        if (fn_80146008(0x578) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x12, 0, 0);
            fn_80146058(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            fn_8014610C(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 7:
        if (fn_80146008(0x6A4) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x29, 0, 0);
            fn_80146058(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
        }
        return;
    case 8:
        if (em_frame_check(self, 0, lbl_80796FD0, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x43, 6, 0);
            fn_801462A4(self, (s16)(fn_80145FE4() + 1), lbl_805A2F28, 0, 2, 0);
        }
        return;
    case 9:
        fn_801462A4(self, (s16)(fn_80145FE4() + 1), lbl_805A2F28, 0, 2, 0);
        if (em_frame_check(self, 0, lbl_80796FD4, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1A, 6, 0);
        }
        return;
    case 10:
        fn_801462A4(self, (s16)(fn_80145FE4() + 1), lbl_805A2F28, 0, 2, 0);
        return;
    }
}

/* 0x80150728 (0xA4) - the two-state step: arm the 0x1A motion, then close the action. */
void fn_80150728(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 0, 0);
        fn_80146058(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
        fn_8014610C(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        return;
    case 1:
        fn_80127FE4(self);
        return;
    }
}

/* 0x801507CC (0x800) - the 17-state step of the long action: each state waits on a frame counter or
 * a motion end and arms the next effect/motion pair. */
void fn_801507CC(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 0, 0);
        fn_8014610C(self, lbl_80796E1C, lbl_80796FD8, lbl_80796E1C);
        fn_8014616C(self, 0);
        return;
    case 1:
        if (fn_80146008(0x15A) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1A, 0, 0);
            fn_80146058(self, lbl_80796FDC, lbl_80796FE0, lbl_80796FE4);
            fn_801462A4(self, fn_80145FE4(), lbl_805A30DC, 0, 6, 0);
            fn_8014619C(self);
        }
        return;
    case 2:
        fn_801462A4(self, fn_80145FE4(), lbl_805A30DC, 0, 6, 0);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1A, 0, 0);
        }
        return;
    case 3:
        fn_801462A4(self, fn_80145FE4(), lbl_805A30DC, 0, 6, 0);
        if (em_after_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            fn_8012F504(self, 0x3D, 0x1E, 0, 1);
        }
        return;
    case 4:
        fn_801462A4(self, fn_80145FE4(), lbl_805A30DC, 0, 6, 0);
        if (fn_80146008(0x280) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1F, 0, 0x16);
            fn_801461A8(self, fn_80145FE4(), lbl_805A3280, lbl_805A3860);
        }
        return;
    case 5:
        fn_801461A8(self, fn_80145FE4(), lbl_805A3280, lbl_805A3860);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x20, 0, 0);
        }
        return;
    case 6:
        fn_801461A8(self, fn_80145FE4(), lbl_805A3280, lbl_805A3860);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1F, 0, 0);
        }
        return;
    case 7:
        fn_801461A8(self, fn_80145FE4(), lbl_805A3280, lbl_805A3860);
        if (em_after_frame_check(self, 0, lbl_80796FE8, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x3D, 0xA, 0);
        }
        return;
    case 8:
        fn_801461A8(self, fn_80145FE4(), lbl_805A3280, lbl_805A3860);
        if (fn_80146008(0x3EE) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0xDB, 0, 0x28);
            fn_801461A8(self, fn_80145FE4(), lbl_805A3AE0, 0);
            fn_8014610C(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        }
        return;
    case 9:
        fn_801461A8(self, fn_80145FE4(), lbl_805A3AE0, 0);
        if (fn_80146008(0x434) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1B, 0, 4);
            fn_801462A4(self, fn_80145FE4(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        }
        return;
    case 10:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (fn_80146008(0x468) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0xCE, 0, 2);
        }
        return;
    case 11:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0xCF, 0, 2);
        }
        return;
    case 12:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (fn_80146008(0x506) == 1) {
            self->state = self->state + 1;
            fn_8012F504(self, 0xCF, 0x14, 0x22, 1);
            fn_80146058(self, lbl_80796FEC, lbl_80796FF0, lbl_80796FF4);
            fn_801462A4(self, fn_80145FE4(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        }
        return;
    case 13:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1B, 0, 0);
        }
        return;
    case 14:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1B, 0, 0);
        }
        return;
    case 15:
        fn_801462A4(self, fn_80145FE4(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (fn_8012F93C(self) == 1) {
            self->state = self->state + 1;
            fn_8012F5B8(self, 0x1C, 0, 0);
            self->timer_0x020 = 0;
        }
        return;
    case 16:
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            fn_80056A54(self, 0x1B, 0xA);
        }
        setVector3(&pos, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            fn_80304508(self, 0, 0x1A, &pos, lbl_80796E20);
        }
        if (em_frame_check(self, 3, lbl_80796FF8, lbl_80796F2C) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                fn_80304508(self, 1, 0x1A, &pos, lbl_80796E20);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        fn_801462A4(self, fn_80145FE4(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        return;
    }
}

/* 0x80150FCC (0xA8) - the closing two-state step. */
void fn_80150FCC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 0, 0);
        fn_80146058(self, lbl_80796FFC, lbl_80797000, lbl_80797004);
        fn_8014610C(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        return;
    }
}

#pragma peephole off

/* 0x80151074 (0x50) - the motion dispatcher: `state_sub` (+0x1E6) selects the step. */
void fn_80151074(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8014F5F0(self);
        return;
    case 1:
        fn_8014F71C(self);
        return;
    case 2:
        fn_8014FA34(self);
        return;
    case 3:
        fn_8014FC24(self);
        return;
    case 4:
        fn_8014FE00(self);
        return;
    case 5:
        fn_8014FF10(self);
        return;
    case 6:
        fn_801502C8(self);
        return;
    case 7:
        fn_80150728(self);
        return;
    case 8:
        fn_801507CC(self);
        return;
    case 9:
        fn_80150FCC(self);
        return;
    }
}

/* 0x801510C4 (0x58) - the action dispatcher: `action` (+0x1E5) selects the band below. */
void fn_801510C4(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801484D4(self);
        return;
    case 1:
        fn_80149004(self);
        return;
    case 2:
        fn_80149814(self);
        return;
    case 3:
        fn_8014AA4C(self);
        return;
    case 4:
        fn_8014B5C0(self);
        return;
    case 7:
        fn_8014E3AC(self);
        return;
    case 8:
        fn_8014EA70(self);
        return;
    case 9:
        fn_8014F030(self);
        return;
    case 10:
        fn_8014F138(self);
        return;
    case 11:
        fn_8014F530(self);
        return;
    case 12:
        fn_8014F5DC(self);
        return;
    case 13:
        fn_80151074(self);
        return;
    }
}

/* 0x8015111C (0x158) - the effect arming helper: only when the enemy faces a valid angle does it
 * spawn the per-motion effect. */
void fn_8015111C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u16 angle;
    u16 motion;
    u32 kind;

    VEC3_ctor(&pos);
    if (fn_8012EC60(self) == 1) {
        angle = calcVecAngX(&self->vec_0x76C);
        if ((u16)(angle + 0x8000) > 0x671B) {
            motion = em_get_mot_no(self);
            if ((u32)(motion - 0x17) > 2U && motion != 0x33 && motion != 0x40 && motion != 0x72 &&
                motion != 0x7D && motion != 0x82) {
                if (self->team == 1 && em_get_mot_no(self) == 0xD0) {
                    kind = 0xFF;
                } else if ((u16)(angle + 0x8000) > 0x6E38) {
                    kind = 6;
                } else {
                    kind = 0;
                }
            } else {
                kind = 0xFF;
            }
            if (kind != 0xFF && (system_w.field_0x0c % 10) == 0) {
                setVector3(&pos, lbl_80796E1C, lbl_80796E54, lbl_80796E60);
                fn_8010562C(self, kind, 0x1A, &pos, lbl_80796E20);
            }
        }
    }
}

/* 0x80151274 (0x74) - arm the one-shot effect when the action timer (+0x354) is idle. */
void fn_80151274(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (self->field_0x354 == 0) {
        setVector3(&pos, lbl_80796E1C, lbl_80796E1C, lbl_80796F88);
        fn_801369A0(self, arg1, 1, &pos, lbl_80796E20);
    }
}

/* 0x801512E8 (0x114) - the effect-spawn dispatcher: `arg1` selects the joint position, the work
 * position or a joint matrix. */
void fn_801512E8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, s32 arg3, s32 arg4, f32 farg0) {
    char buf_0x18[0x18];
    nw4r::math::VEC3 pos;

    MTX34_ctor(buf_0x18);
    VEC3_ctor(&pos);
    switch (arg1) {
    case 0:
        fn_801048B4(self, arg3, arg2, arg4, farg0);
        return;
    case 1:
        get_joint_wpos_em(self, arg3, &pos);
        pos.y = self->field_0x20C;
        fn_8010D2B0(&pos, self->area_no, arg2, self->field_0x1C0 + arg4,
                    farg0 * get_em_chg_scale(self));
        return;
    case 2:
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        fn_80106694(self, &pos, arg2, farg0);
        return;
    }
}

/* 0x801513FC (0xC0) - the shell-attach step: every fourth frame arm the 2/1 effect and, when the
 * gate is set, hand the work position to the shell callback table. */
void fn_801513FC(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((system_w.field_0x0c & 3) == 0) {
        fn_801512E8(self, 2, 1, 0, 0, lbl_80796E20);
        if ((system_w.field_0x0c & 4) != 0) {
            pos.x = self->pos.x;
            pos.y = self->field_0x20C;
            pos.z = self->pos.z;
            shell_set_func_ptr->field_0x28(self, &pos, 0, 0xFFFF, shell_set_func_ptr, lbl_80796E20);
        }
    }
}

/* 0x80154184 (0x14C) - the part-colour refresh: the two body-part damage levels pick the RGBA pair
 * handed to the MHchar. */
void fn_80154184(struct _ENEMY_WORK* self) {
    u32 rgba[2];
    f32 ratio;

    if (em_parts_damage_level_get(self, 1) < 1) {
        rgba[0] = 0x46;
        rgba[1] = 0xFF;
    } else {
        rgba[0] = 0xA0;
        rgba[1] = 0xFF;
    }
    ratio = self->field_0x1D4;
    fn_800E2F40(self->char_0x024, 1, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
                (u8)(s32)((f32)rgba[1] * ratio), ratio);

    if (em_parts_damage_level_get(self, 2) < 1) {
        rgba[0] = 0x46;
        rgba[1] = 0xFF;
    } else {
        rgba[0] = 0x78;
        rgba[1] = 0xDC;
    }
    ratio = self->field_0x1D4;
    fn_800E2F40(self->char_0x024, 2, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
                (u8)(s32)((f32)rgba[1] * ratio), ratio);
}

/* 0x801542D0 (0x220) - the attack-eligibility test: the enemy's kind/area pair picks the damage
 * interval and the action-record request. */
s32 fn_801542D0(struct _ENEMY_WORK* self, u16 arg1) {
    u8 kind;
    u8 temp;
    u8 temp2;
    u32 range;
    s32 armed;
    s32 arg3;

    kind = fn_802B0668(self->field_0x1E0);
    if ((u32)(kind - 1) > 2U && kind != 5) {
        return 0;
    }
    if (kind == 1 && self->field_0x357 == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 4;
        return 1;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    armed = 0;
    if (kind == 1) {
        range = 0xA;
    } else if (kind == 2) {
        range = 0xA;
    } else if (kind == 3) {
        range = 0xA;
    } else if (kind == 5) {
        range = 5;
    } else {
        range = 0xFF;
    }
    if (range != 0xFF) {
        temp = fn_80129DB8(self);
        if (temp == 2) {
            return 1;
        }
        if (temp == 1) {
            armed = 1;
        }
    }
    if (armed == 0) {
        temp2 = fn_80129E48(self);
        if (temp2 == 1) {
            return 0;
        }
        if (temp2 == 2) {
            return 1;
        }
    }
    if (armed == 0) {
        if ((u32)(kind - 2) <= 1U) {
            arg3 = 3;
        } else if (kind == 1) {
            arg3 = 0xA;
        } else if (kind == 5) {
            arg3 = 3;
        } else {
            arg3 = 0xFF;
        }
        if (fn_8012A014(self, 0x1B, arg3, arg1, lbl_805A20C8, lbl_805A20D4) == 1) {
            return 1;
        }
    }
    if (fn_80129A70(self, arg1) == 1) {
        return 1;
    }
    return (fn_8012A204(self) - 1) == 0;
}

#pragma peephole on

/* 0x801545B8 (0x80) - fill the 0x18-byte spawn record and hand it to the effect queue. */
void fn_801545B8(EmSpawnRec* rec, u8 arg1, s16 arg2, s16 arg3) {
    nw4r::math::VEC3 pos;

    setVec3(&pos, lbl_80796E1C, lbl_80797098, lbl_80796E3C);
    rec->id = 0x1A;
    copyVec3(&rec->pos, &pos);
    rec->field_0x10 = arg1;
    rec->field_0x12 = arg2;
    rec->field_0x14 = arg3;
}

#pragma peephole off

/* 0x80154638 (0x14C) - the range/height test: measure the target's position against the work
 * position through the joint helper. */
s32 fn_80154638(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 w;
    struct _ENEMY_WORK* target;
    f32 dist;
    f32 limit;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    target = fn_80131034(self, 0x1B, 0);
    if (target == 0) {
        return 0;
    }
    if (fn_8012E5A8(target) != 1) {
        return 0;
    }
    if (arg1 == 0) {
        return 1;
    }
    copyVec3(&b, setVec3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80797074 * get_em_chg_scale(self)));
    rotVecY(&b, self->field_0x1C0);
    fn_80051378(&w, &self->pos, &b);
    copyVec3(&a, &w);
    dist = calcDistanceSqXZ(&a, &target->pos);
    limit = lbl_8079709C * get_em_chg_scale(self);
    if (dist < (lbl_8079709C * get_em_chg_scale(self)) * limit) {
        return 1;
    }
    return 0;
}

/* 0x80154784 (0x28) - is the action one of the glide steps?  `u32`: its call site compares the
 * answer against 1 unsigned (`enemy/fn_801B0010.cpp`). */
u32 fn_80154784(struct _ENEMY_WORK* self) {
    if (self->action == 0xD && self->state_sub <= 4) {
        return 1;
    }
    return 0;
}

/* 0x801547AC (0x17C) - the per-kind attack test (`arg1` selects the kind). */
u8 fn_801547AC(struct _ENEMY_WORK* self, u8 arg1) {
    u8 kind;
    u8 area;

    switch (arg1) {
    case 0:
        return fn_80131034(self, 0x1B, 1) != 0;
    case 1:
        return self->field_0x356 != 0;
    case 2:
        kind = fn_802B0668(self->field_0x1E0);
        switch (kind) {
        case 0:
            return 1;
        case 1:
            area = self->area_no;
            if (area == 3 || area == 5) {
                return 1;
            }
            return 0;
        case 5:
            area = self->area_no;
            if (area == 3 || area == 6 || area == 8) {
                return 1;
            }
            return 0;
        case 8:
            if (self->area_no == 1) {
                return 1;
            }
            return 0;
        case 9:
            if (self->area_no == 0) {
                return 1;
            }
            return 0;
        default:
            return 0;
        }
    case 3:
        return (u8)((em_magma_check(self) - 1) == 0);
    case 4:
        if (self->field_0x1E2 == 1 && self->field_0x358 == 1) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/* 0x80154928 (0x54) - step the work position's y and re-sync the model. */
void fn_80154928(struct _ENEMY_WORK* self, u8* arg1, u8* arg2) {
    *arg1 = 0xC;
    *arg2 = 0;
    self->pos.y = self->pos.y + lbl_80796E18;
    fn_80130248(self);
    fn_801305C4(self);
}

/* 0x8015497C (0xC) - clear the action's case-6 flag. */
void fn_8015497C(struct _ENEMY_WORK* self) {
    self->field_0x357 = 0;
}

/* 0x80154988 (0x17C) - the two-way rotation/scale step: the enemy's part state drives the four
 * animation angles toward or away from their limits. */
void fn_80154988(struct _ENEMY_WORK* self) {
    f32 v;

    if (fn_8012EC3C(self) == 1) {
        self->rot_0x328.field_0x330 = self->rot_0x328.field_0x330 - 0x444;
        if ((s16)self->rot_0x328.field_0x330 < -0x2AAA) {
            self->rot_0x328.field_0x330 = 0xD556;
        }
        v = self->rot_0x328.field_0x33C - lbl_807970A0;
        self->rot_0x328.field_0x33C = v;
        if (v > lbl_807970A4) {
            self->rot_0x328.field_0x33C = lbl_807970A4;
        }
        v = self->rot_0x328.field_0x340 - lbl_80796F40;
        self->rot_0x328.field_0x340 = v;
        if (v < lbl_80796E1C) {
            self->rot_0x328.field_0x340 = lbl_80796E1C;
        }
        v = self->rot_0x328.field_0x348 - lbl_80797044;
        self->rot_0x328.field_0x348 = v;
        if (v < lbl_807970A8) {
            self->rot_0x328.field_0x348 = lbl_807970A8;
        }
        v = self->rot_0x328.field_0x34C - lbl_80796F40;
        self->rot_0x328.field_0x34C = v;
        if (v < lbl_80796E1C) {
            self->rot_0x328.field_0x34C = lbl_80796E1C;
        }
    } else {
        self->rot_0x328.field_0x330 = self->rot_0x328.field_0x330 + 0x444;
        if ((s16)self->rot_0x328.field_0x330 > 0) {
            self->rot_0x328.field_0x330 = 0;
        }
        v = self->rot_0x328.field_0x33C + lbl_807970A0;
        self->rot_0x328.field_0x33C = v;
        if (v < lbl_80796E08) {
            self->rot_0x328.field_0x33C = lbl_80796E08;
        }
        v = self->rot_0x328.field_0x340 + lbl_80796F40;
        self->rot_0x328.field_0x340 = v;
        if (v > lbl_80796E0C) {
            self->rot_0x328.field_0x340 = lbl_80796E0C;
        }
        v = self->rot_0x328.field_0x348 + lbl_80797044;
        self->rot_0x328.field_0x348 = v;
        if (v > lbl_80796E14) {
            self->rot_0x328.field_0x348 = lbl_80796E14;
        }
        v = self->rot_0x328.field_0x34C + lbl_80796F40;
        self->rot_0x328.field_0x34C = v;
        if (v > lbl_80796E0C) {
            self->rot_0x328.field_0x34C = lbl_80796E0C;
        }
    }
}

/* 0x80154B04 (0x170) - the aim/rotation step: derive the angle to the work position's stored
 * target and pick the nearest rotation the effect can take. */
void fn_80154B04(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 farg0) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 w;
    u16 angle;
    u16 delta;
    f32 scale;
    u16 base;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    copyVec3(&a, &self->vec_0x36C);
    angle = (u16)calcVecAng2(&self->pos, &a);
    *angleOut = angle;
    delta = (u16)(angle - self->field_0x1C0);
    scale = farg0 * get_em_chg_scale(self);
    if ((u16)(delta + 0xBFFF) > 0x7FFE) {
        setVector3(&b, lbl_80796E1C, lbl_80796E1C, fn_80050EF4(&a, &self->pos) - scale);
        rotVecY(&b, *angleOut);
        fn_80051378(&v, &self->pos, &b);
        copyVec3(out, &v);
        return;
    }
    base = 0xC000;
    if (delta < (u16)-0x8000) {
        base = 0x4000;
    }
    *angleOut = (u16)(base + self->field_0x1C0);
    setVector3(&b, lbl_80796E1C, lbl_80796E1C, -scale);
    rotVecY(&b, *angleOut);
    fn_80051378(&w, &a, &b);
    copyVec3(out, &w);
}

/* 0x80154C74 (0x30) - the team/area sound selector. */
void fn_80154C74(struct _ENEMY_WORK* self) {
    if (self->team == 2) {
        if (self->field_0x1E2 == 3) {
            fn_80130CDC(-7);
            return;
        }
        fn_80130CDC(-0xE);
        return;
    }
    fn_80130CDC(-0xE);
}

/* 0x80154CA4 (0x44) - clamp the work position's height. */
void fn_80154CA4(struct _ENEMY_WORK* self) {
    fn_801303FC(lbl_807970AC);
    if (self->field_0x1AC < lbl_807970B0) {
        self->field_0x1AC = lbl_807970B0;
    }
}

/* 0x80154CE8 (0x5C) - release the 0x0C-byte helper when the action closes. */
s32 fn_80154CE8(s32 self, s16 flag) {
    if (self != 0) {
        fn_8013918C(0, 0);
        if (flag > 0) {
            operator delete((void*)self);
        }
    }
    return self;
}

/* 0x80154D44 (0xFC) - seed the six 0xC-byte animation vectors of the three global records. */
void fn_80154D44(void) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;
    nw4r::math::VEC3 e;
    nw4r::math::VEC3 f;

    fn_80051490(lbl_806A77D8, (s32)setVec3(&a, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    fn_80051490(lbl_806A77D8 + 0xC, (s32)setVec3(&b, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    fn_80051490(lbl_806A77F0, (s32)setVec3(&c, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    fn_80051490(lbl_806A77F0 + 0xC, (s32)setVec3(&d, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    fn_80051490(lbl_806A7808, (s32)setVec3(&e, lbl_80796E1C, lbl_80796E58, lbl_80796E1C));
    fn_80051490(lbl_806A7808 + 0xC, (s32)setVec3(&f, lbl_80796E1C, lbl_807970B8, lbl_80796E1C));
}

/* 0x80154E40 (0x50) - reset the effect slot set. */
void fn_80154E40(struct _ENEMY_WORK* self) {
    u8 i;

    for (i = 0; i < 6; i++) {
        self->init_0x328.slots_0x328[i] = 0xFF;
    }
    for (i = 6; i < 0x0F; i++) {
        self->init_0x328.slots_0x328[i] = 0;
    }
    self->init_0x328.field_0x338 = 0;
    self->init_0x328.field_0x33C = 0;
}

/* 0x80154E90 (0xE0) - attach the 0x0C-byte helper and arm the 0x1C72 effect at the work position. */
void fn_80154E90(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;
    s32 helper;

    VEC3_ctor(&pos);
    if (arg1 != 0) {
        fn_8012FCC4(self, 0, lbl_807970C0);
        fn_8012FCC4(self, 0xA, lbl_807970C4);
    }
    if (fn_801391E8(self) == 0) {
        helper = (s32)operator new(0xC);
        if (helper != 0) {
            fn_80154F70((void**)helper);
        }
        fn_801390FC(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, lbl_807970C0, lbl_807970C8, lbl_807970CC);
        fn_801057A4(self, 0x1A, &pos, 0x1C72, lbl_807970C4);
    }
    self->flags_0x836 = (u16)(self->flags_0x836 | 0x8000);
}

/* 0x80154F70 (0x3C) - the 0x0C-byte helper's constructor (installs the `lbl_805A5D18` table). */
void fn_80154F70(void** self) {
    fn_80147E2C(self);
    *self = (void*)lbl_805A5D18;
}

/* 0x80154FAC (0x150) - remap the effect id when the action is past its first step. */
void fn_80154FAC(struct _ENEMY_WORK* self, u8* arg1, u8* arg2) {
    switch (*arg1) {
    case 1:
        if (fn_8012ECF0(self) == 1 || self->field_0x7C8 >= 0x29) {
            switch (*arg2) {
            case 8:
                *arg2 = 0x19;
                return;
            case 13:
                *arg2 = 0x1A;
                return;
            case 14:
                *arg2 = 0x1B;
                return;
            case 15:
                *arg2 = 0x1C;
                return;
            case 16:
                *arg2 = 0x1D;
                return;
            case 17:
                *arg2 = 0x1E;
                return;
            case 33:
                *arg2 = 0x23;
                return;
            case 34:
                *arg2 = 0x24;
                return;
            }
        }
        return;
    case 2:
        switch (*arg2) {
        case 0:
            if (fn_8012ECF0(self) == 1) {
                *arg2 = 3;
            }
            return;
        case 8:
            if (fn_8012ECF0(self) == 1) {
                *arg2 = 9;
            }
            return;
        case 10:
            if (fn_8012ECF0(self) == 1) {
                *arg2 = 0xB;
            }
            return;
        }
        return;
    }
}

#ifdef __cplusplus
}
#endif
