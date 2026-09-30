/* enemy/fn_801502C8.cpp - the enemy em00x action band, `.text` 0x801502C8..0x80154E40 (26 functions).
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
 *      `3FC0`, and the `.bss` vector records - none is a source-file name.  (Discovery notes a
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
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
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
 * seam.  The left edge at 0x801502C8 is where the registered `enemy/fn_8014A1BC.c` ends and its
 * extab/extabindex runs end exactly where this unit's begin (extab 0x8000DAD4..0x8000DC54,
 * extabindex 0x80028728..0x80028968).  The right edge at 0x80154E40 is proven three ways: `fn_80154D44`
 * is this unit's static-initializer (the `.ctors` word below points at it, and MWCC places a TU's
 * `__sinit` last in its `.text`); the value 0.0 is pooled at `lbl_80796E1C` (read up to `fn_80154D44`) and
 * again at `lbl_807970C0` (first read by `fn_80154E90`), and one TU pools a value once; the next `.data`
 * claim, `lbl_805A5D18` (the vtable `fn_80154F70` installs) and the `jumptable_805A4848` of `fn_80154FAC`
 * tile `enemy/fn_801550FC.cpp`'s `.data`.  The four functions 0x80154E40..0x801550FC moved there (the old
 * edge 0x801550FC was the start of the next registered unit, not a seam).
 *
 * sections.  `.text` 0x801502C8..0x80154E40, `extab` 0x8000DC54..0x8000DCFC, `extabindex`
 * 0x80028968..0x80028A64, `.ctors` 0x8056F320..0x8056F324 (its entry is `fn_80154D44`, the six-vector
 * seeder), `.data` 0x805A2BD8..0x805A4478 (the parameter tables and `jumptable_*` this range reads) and `.bss` 0x806A77D8..0x806A7820 (the
 * three 0x18-byte vector records `fn_80154D44` fills).  The `.data` and `.bss` claims are not emitted by
 * the source yet.
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
 * shape and residuals, measured (`recompile.py --measure`, the official report metric).  22 of the 24
 * written functions are at or above the 80 % bar, 12 of them byte-identical (`fn_801502C8`,
 * `fn_80150728`, `fn_80150FCC`, `fn_80151074`, `fn_801510C4`, `fn_801545B8`, `fn_80154784`,
 * `fn_80154928`, `fn_8015497C`, `fn_80154988`, `fn_80154CA4`, `fn_80154D44`).  The file default is
 * `#pragma peephole off` - like `enemy/fn_801D80EC.cpp`, most of the band keeps the unfused
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
#include "enemy/fn_80147CE0.h" /* EmSpawnRec + em_res_user_data_ctor (the owner's header) */
#include "enemy/fn_801502C8.h" /* this unit's own declarations (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * the band's callees, declared with the signatures their call sites set (rule 2: the plain prototypes
 * are not `extern` declarations of another unit's symbol; the owners' headers carry the canonical
 * spellings and are a follow-up).
 * -------------------------------------------------------------------------------------------------- */

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) - the action/motion arming helpers. */
void em_busy_set(struct _ENEMY_WORK* self);
void em_mot_set_blend(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c, s32 d);
void em_mot_set(struct _ENEMY_WORK* self, s32 motion, s32 arg2, s32 arg3);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
void em_fall_height_get(struct _ENEMY_WORK* self);
void fn_801303FC(f32 a);
void em_fall_start(struct _ENEMY_WORK* self);
void fn_80130CDC(s32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_801369A0(struct _ENEMY_WORK* self, u8 a, s32 b, void* c, f32 d);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);
s16 em_demo_frame_get(void);
u32 em_demo_time_ck(s32 label);
void em_demo_pos_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_reset(struct _ENEMY_WORK* self, s32 a);
void em_demo_enable(struct _ENEMY_WORK* self);
void em_demo_key3_apply(struct _ENEMY_WORK* self, s16 a, void* b, void* c);
void em_demo_key_apply(struct _ENEMY_WORK* self, s16 a, void* b, void* c, s32 d, s32 e);
void em_hit_window_set_default(struct _ENEMY_WORK* self, s32 a, s32 b);
u32 fn_8012A014(struct _ENEMY_WORK* self, s32 a, s32 b, u16 c, void* d, void* e);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);u32 fn_80129D3C(struct _ENEMY_WORK* self);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u8 fn_80129E48(struct _ENEMY_WORK* self);
s32 fn_8012A204(struct _ENEMY_WORK* self);
u8 stage_map_kind_get(u8 a);
void em_action_finish_fall(struct _ENEMY_WORK* self);

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
void eft009_spawn_at_joint(struct _ENEMY_WORK* self, s32 a, u8 b, s32 c, f32 d);
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, void* pos, f32 c);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);

/* the runtime helpers the range reaches. */
f32 fn_80050EF4(const void* a, const void* b);
void addVec3(void* out, const void* a, const void* b);
void assignVec3(void* dst, s32 src);
void fn_800516F0(void* out);
void draw_shape_arm(struct _ENEMY_WORK* self, s32 a, s32 b);
void mhchar_mat_tev_set(void* self, s32 a, s32 b, u8 c, s32 d, s32 e, u8 f, f32 g);
void fn_8013918C(void* self, s32 a);
void eft_em_spawn(struct _ENEMY_WORK* self, s32 a, s32 b, void* c, f32 d);
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
void operator delete(void* ptr) throw();

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

/* this range's own `.data` parameter tables (claimed in splits.txt, not emitted by the source yet, so
 * they stay the original bytes; only the ones the code loads are declared). */
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
extern VEC3 vec_pair_801502C8_0[2];
extern VEC3 vec_pair_801502C8_1[2];
extern VEC3 vec_pair_801502C8_2[2];

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * functions, in address order
 * -------------------------------------------------------------------------------------------------- */

#pragma peephole on

/* 0x801502C8 (0x460) - the 11-state action step (state at +0x05): arm the motion, wait out each
 * effect window through `em_demo_time_ck`/`em_frame_check`, and re-arm the next motion at each
 * transition. */
void fn_801502C8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x1D6) == 1) {
            self->state = self->state + 1;
            em_demo_pos_set(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
            em_demo_enable(self);
        }
        return;
    case 2:
        if (em_demo_time_ck(0x2DA) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x3D, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        }
        return;
    case 3:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
        if (em_after_frame_check(self, 0, lbl_80796F18, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_demo_reset(self, 0);
        }
        return;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
        if (em_demo_time_ck(0x3A2) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xC8, 0, 0x1E);
            em_demo_enable(self);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2D68, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796FC4, lbl_80796E1C);
        }
        return;
    case 5:
        if (em_frame_check(self, 0, lbl_80796F44, lbl_80796E1C) == 1) {
            em_hit_window_set_default(self, 0, 0x17);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2D68, 0);
        if (em_demo_time_ck(0x41A) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x13, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 6:
        if (em_demo_time_ck(0x578) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x12, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 7:
        if (em_demo_time_ck(0x6A4) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x29, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
        }
        return;
    case 8:
        if (em_frame_check(self, 0, lbl_80796FD0, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x43, 6, 0);
            em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        }
        return;
    case 9:
        em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        if (em_frame_check(self, 0, lbl_80796FD4, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 6, 0);
        }
        return;
    case 10:
        em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        return;
    }
}

/* 0x80150728 (0xA4) - the two-state step: arm the 0x1A motion, then close the action. */
void fn_80150728(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_pos_set(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        return;
    case 1:
        em_action_finish_fall(self);
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
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796FD8, lbl_80796E1C);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x15A) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 0, 0);
            em_demo_pos_set(self, lbl_80796FDC, lbl_80796FE0, lbl_80796FE4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
            em_demo_enable(self);
        }
        return;
    case 2:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 0, 0);
        }
        return;
    case 3:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_after_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set_blend(self, 0x3D, 0x1E, 0, 1);
        }
        return;
    case 4:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_demo_time_ck(0x280) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1F, 0, 0x16);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        }
        return;
    case 5:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x20, 0, 0);
        }
        return;
    case 6:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1F, 0, 0);
        }
        return;
    case 7:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_after_frame_check(self, 0, lbl_80796FE8, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x3D, 0xA, 0);
        }
        return;
    case 8:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_demo_time_ck(0x3EE) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xDB, 0, 0x28);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3AE0, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        }
        return;
    case 9:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3AE0, 0);
        if (em_demo_time_ck(0x434) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        }
        return;
    case 10:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_demo_time_ck(0x468) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xCE, 0, 2);
        }
        return;
    case 11:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xCF, 0, 2);
        }
        return;
    case 12:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_demo_time_ck(0x506) == 1) {
            self->state = self->state + 1;
            em_mot_set_blend(self, 0xCF, 0x14, 0x22, 1);
            em_demo_pos_set(self, lbl_80796FEC, lbl_80796FF0, lbl_80796FF4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        }
        return;
    case 13:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 0);
        }
        return;
    case 14:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 0);
        }
        return;
    case 15:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1C, 0, 0);
            self->timer_0x020 = 0;
        }
        return;
    case 16:
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            draw_shape_arm(self, 0x1B, 0xA);
        }
        setVector3(&pos, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            eft_em_spawn(self, 0, 0x1A, &pos, lbl_80796E20);
        }
        if (em_frame_check(self, 3, lbl_80796FF8, lbl_80796F2C) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x1A, &pos, lbl_80796E20);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        return;
    }
}

/* 0x80150FCC (0xA8) - the closing two-state step. */
void fn_80150FCC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_pos_set(self, lbl_80796FFC, lbl_80797000, lbl_80797004);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
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
    if (em_alt_mode_ck(self) == 1) {
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
                eft_spawn_type10(self, kind, 0x1A, &pos, lbl_80796E20);
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

    /* `buf_0x18` is the unit's own 0x18-byte scratch buffer, whose first 0xC bytes are the record
     * the helper constructs; the buffer's size is part of the matched frame (see the unit header). */
    MTX34_ctor((MTX34*)buf_0x18);
    VEC3_ctor(&pos);
    switch (arg1) {
    case 0:
        eft009_spawn_at_joint(self, arg3, arg2, arg4, farg0);
        return;
    case 1:
        get_joint_wpos_em(self, arg3, &pos);
        pos.y = self->field_0x20C;
        eft_spawn_pos_in_area(&pos, self->area_no, arg2, self->field_0x1C0 + arg4,
                    farg0 * get_em_chg_scale(self));
        return;
    case 2:
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        eft_spawn_type11(self, &pos, arg2, farg0);
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
            shell_set_func_ptr->method_0x28(self, &pos, 0, 0xFFFF, shell_set_func_ptr, lbl_80796E20);
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
    mhchar_mat_tev_set(self->char_0x024, 1, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
                (u8)(s32)((f32)rgba[1] * ratio), ratio);

    if (em_parts_damage_level_get(self, 2) < 1) {
        rgba[0] = 0x46;
        rgba[1] = 0xFF;
    } else {
        rgba[0] = 0x78;
        rgba[1] = 0xDC;
    }
    ratio = self->field_0x1D4;
    mhchar_mat_tev_set(self->char_0x024, 2, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
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

    kind = stage_map_kind_get(self->field_0x1E0);
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
    addVec3(&w, &self->pos, &b);
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
        kind = stage_map_kind_get(self->field_0x1E0);
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
    em_fall_height_get(self);
    em_fall_start(self);
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
        addVec3(&v, &self->pos, &b);
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
    addVec3(&w, &a, &b);
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

    assignVec3(vec_pair_801502C8_0, (s32)setVec3(&a, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    assignVec3(&vec_pair_801502C8_0[1], (s32)setVec3(&b, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    assignVec3(vec_pair_801502C8_1, (s32)setVec3(&c, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    assignVec3(&vec_pair_801502C8_1[1], (s32)setVec3(&d, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    assignVec3(vec_pair_801502C8_2, (s32)setVec3(&e, lbl_80796E1C, lbl_80796E58, lbl_80796E1C));
    assignVec3(&vec_pair_801502C8_2[1], (s32)setVec3(&f, lbl_80796E1C, lbl_807970B8, lbl_80796E1C));
}

#ifdef __cplusplus
}
#endif

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A77D8..0x806A7820`), in address order: the 3 two-vector record(s)
 * its static constructor `fn_80154D44` builds (`.data` tables point at them).  Names are GUESSes: each record is a
 * pair of model-space points. */
VEC3 vec_pair_801502C8_0[2];  /* +0x806A77D8 */
VEC3 vec_pair_801502C8_1[2];  /* +0x806A77F0 */
VEC3 vec_pair_801502C8_2[2];  /* +0x806A7808 */
