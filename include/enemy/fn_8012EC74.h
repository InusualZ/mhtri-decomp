/* The enemy unit `enemy/fn_8012EC74.cpp` (0x8012EC74..0x80137604): its band's arming helpers, which
 * the neighbouring action units call.
 *
 * Declarations moved here from `include/unsplit/enemy.h` (docs/plan.md 6.5 rule 2: an extern lives
 * with the TU that owns the symbol).  The bodies are still to be written - the signatures are the
 * call sites', with the argument registers and return register recorded per function.
 */
#ifndef MHTRI_ENEMY_FN_8012EC74_H
#define MHTRI_ENEMY_FN_8012EC74_H

#include "types.h"
#ifdef __cplusplus
#include "nw4r/math.h"
#endif

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* r3 (`self`) and f1; its callers `fadds` the return into a value they build. */
f32 fn_8013026C(struct _ENEMY_WORK* self);
/* r3 (`self`), f1; no return (`enemy/fn_80182D5C.cpp`'s `fn_801850F8` passes the sum it just built). */
void fn_8012FE3C(struct _ENEMY_WORK* self, f32 a);
/* r3 (`self`); returns a word compared against 1 (`cmplwi`) - the teardown step of
 * `enemy/fn_80182D5C.cpp` runs `fn_8012E664` only when it answers 1. */
u32 fn_801337FC(struct _ENEMY_WORK* self);
/* r3 (`self`) and r4/r5; the arming helper the action band's functions call. */
void fn_80136B50(struct _ENEMY_WORK* self, u32 a, u32 b);
/* r3 (`self`) and f1, the fade duration it stores and passes on.  `enemy/fn_8014A1BC.c` calls
 * it with two arguments too, but this is the form its own C++ consumer needs. */
#ifdef __cplusplus
void fn_80136D4C(struct _ENEMY_WORK* self, f32 a);
#else
void fn_80136D4C();
#endif
/* `UpdateValue` is this unit's own definition (0x8012FDA0) and its map name is unmangled, so it is
 * declared at C linkage.  Added by `enemy/fn_801B0010.cpp` (rule 2); the answer is in r3. */
u32 UpdateValue(struct _ENEMY_WORK* self);
/* 0x80132154 - this unit's own definition, added by `enemy/fn_801B0010.cpp` (rule 2): r3 the work
 * record and nothing else. */
void fn_80132154(struct _ENEMY_WORK* self);
/* 0x801339AC - this unit's own definition, added by `enemy/fn_801B0010.cpp` (rule 2): r3 the work
 * record, the answer in r3 (compared against 1 by every call site). */
u32 fn_801339AC(struct _ENEMY_WORK* self);
/* 0x80133DB0 - the angle stepper this unit owns.  MOVED here from `include/unsplit/enemy.h` (rule 2:
 * the owner is this unit, and the band header's `u16 fn_80133DB0()` was the no-prototype form).  The
 * signature is the owner's consumers': `enemy/fn_80137604.cpp` declares `(u16, u16, u16)` and
 * `enemy/fn_8014A1BC.c` calls it with three `(u16)`-cast arguments; the answer is a 16-bit angle. */
u16 fn_80133DB0(u16 a, u16 b, u16 c);

/* 0x80130350 - r3 (`self`) and r4 (the `VEC3*` the caller builds); the body writes through r4 only, so
 * no caller reads a return value.  Added with `enemy/fn_801B7020.cpp`, which calls it the same way. */
void fn_80130350(struct _ENEMY_WORK* self, void* vec);
/* 0x8013072C - r3 (`self`), r4 (narrowed with `clrlwi r4,r4,24`) and r5; the action-mode/latch setter
 * the enemy program functions call.  Names of the two scalars are this unit's call sites' (2/0 and
 * 0/0); the body compares r4 against `self->+0x43B`. */
void fn_8013072C(struct _ENEMY_WORK* self, u32 mode, u32 value);
/* 0x80131FA0 - r3 (`self`) and r4 (the scalar the motion modes pass: 80). */
void fn_80131FA0(struct _ENEMY_WORK* self, u32 a);
/* The effect-slot step band this unit owns, declared here for `enemy/fn_801A9540.cpp` (rule 2):
 * that range's per-action state machines drive the motion through them, and the addresses sit in
 * this unit's own `.text` range, so its header is their home.  Signatures are the call sites' -
 * each is a leaf this unit never re-enters. */
/* `fn_8012F8C8` is deliberately NOT declared here: it is one of the 0x8012F symbols whose C
 * consumer (`enemy/fn_8014A1BC.c`) calls it with `self` only and relies on the old-style
 * declaration, so `include/unsplit/enemy.h` carries its `#ifdef __cplusplus` / `#else void
 * fn_8012F8C8();` split form - the shape `fn_80130008`, `fn_801303FC`, `fn_80133F4C` and
 * `fn_80135600` keep there too, and the form the C++ consumers that do not include this header
 * (`enemy/fn_801550FC.cpp`, `enemy/fn_8015E854.cpp`) reach it through.  A prototype here is an
 * MWCC 10563 redeclaration against that declaration as soon as a C consumer includes both
 * headers, which is what `enemy/fn_8014A1BC.c` does. */
void fn_80134004(struct _ENEMY_WORK* self, u32 a, f32 b);
/* r3 the work record, r4/r5 two scalars; returns 1 while the running motion has not finished. */
u32 fn_80134114(struct _ENEMY_WORK* self, s32 a, s32 b);
/* r3 the work record, r4 the effect table, r5/r6/r7 the three scalars the spawn helper takes. */
void fn_80134964(struct _ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
/* r3 the work record, r4 the effect table; returns 1 once the effect has finished. */
u32 fn_80134B0C(struct _ENEMY_WORK* self, void* tbl);
/* r3 the work record; the normalized motion-frame ratio. */
f32 fn_8012F8EC(struct _ENEMY_WORK* self);
/* r3 the work record; the per-frame motion tick the escaping actions run at their head. */
void fn_80131D84(struct _ENEMY_WORK* self);
/* r3 the work record; plays the armed motion's end reaction. */
void fn_80132160(struct _ENEMY_WORK* self);

/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
void fn_801354AC(struct _ENEMY_WORK* self);
void fn_801354B4(struct _ENEMY_WORK* self);
void fn_8013581C(VEC3* out, VEC3* a, VEC3* b, u32 c, u16 d, f32 e);

/* 0x80136DF4 - r3 the work record; the per-frame refresh the em035 program's angle-reset steps tail
 * with (`enemy/em035_prog.cpp`'s `fn_8035F39C`/`fn_8035F5A8`/`fn_8035F644` call it once at the
 * sub-state entry and once per wait frame, and none of them reads a result).  Added with that unit's
 * registration (rule 2: the address is in this unit's own range). */
void fn_80136DF4(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The C++ spellings of this unit's mangled callees, so a call site never spells the mangling
 * (docs/plan.md 6.5 rule 9); each mangles back to its map name.  Added with the registration of
 * `enemy/fn_801993E0.cpp`, which calls all three. */
f32 em_water_check(struct _ENEMY_WORK* self);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
/* `fn_80131034` is deliberately NOT declared here: its map name is the plain `fn_80131034`, so its
 * consumers reach the `extern "C"` form `include/unsplit/enemy.h` already carries.  A C++-linkage
 * copy here is an MWCC 10505 "illegal overloading" against that one. */
/* 0x80135940 - the model scale.  The target's symbol is the C++ mangling
 * `get_em_scale__FP11_ENEMY_WORK` (mangle.py: `f32 get_em_scale(_ENEMY_WORK*)`), so it is declared at
 * C++ scope (docs/plan.md 6.5 rule 9: a caller never spells the mangling). */
f32 get_em_scale(struct _ENEMY_WORK* self);
/* This unit owns three C++-mangled callees the enemy action units reach: the map names
 * `em_get_mot_no__FP11_ENEMY_WORK`, `em_after_frame_check__FP11_ENEMY_WORKUsff` and
 * `get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34`.  Declared at C++ scope so a caller
 * never spells the mangling (docs/plan.md 6.5 rule 9), added with `enemy/fn_801A4504.cpp` (its first
 * consumer to need all three). */
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
/* 0x8013011C - the `em_magma_check__FP11_ENEMY_WORK` predicate: 1 when the work sits on lava.  The
 * map name is a mangling, so it is declared at C++ scope (rule 9); added with
 * `enemy/fn_80387844.cpp`, whose action band branches on it before spawning an effect. */
u32 em_magma_check(struct _ENEMY_WORK* self);
#endif

#endif
