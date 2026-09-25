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

/* 0x80130350 - r3 (`self`) and r4 (the `VEC3*` the caller builds); the body writes through r4 only, so
 * no caller reads a return value.  Added with `enemy/fn_801B7020.cpp`, which calls it the same way. */
void fn_80130350(struct _ENEMY_WORK* self, void* vec);
/* 0x8013072C - r3 (`self`), r4 (narrowed with `clrlwi r4,r4,24`) and r5; the action-mode/latch setter
 * the enemy program functions call.  Names of the two scalars are this unit's call sites' (2/0 and
 * 0/0); the body compares r4 against `self->+0x43B`. */
void fn_8013072C(struct _ENEMY_WORK* self, u32 mode, u32 value);
/* 0x80131FA0 - r3 (`self`) and r4 (the scalar the motion modes pass: 80). */
void fn_80131FA0(struct _ENEMY_WORK* self, u32 a);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
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
#endif

#endif
