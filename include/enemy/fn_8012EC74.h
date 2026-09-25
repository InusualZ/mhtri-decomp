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
/* 0x8013072C - this unit's own definition, added by `enemy/fn_801B0010.cpp` (rule 2).  Signature is
 * the owner's: r3 the work record and r4/r5 the two scalars every call site sets. */
void fn_8013072C(struct _ENEMY_WORK* self, u32 a, u32 b);
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

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* `em_get_mot_no` is this unit's own definition and the map name is its mangling
 * (`em_get_mot_no__FP11_ENEMY_WORK`), so it is declared at C++ scope (rule 9) and answers the
 * motion id in r3 (its callers mask it to 16 bits).  Added by `enemy/fn_801B0010.cpp` (rule 2). */
u16 em_get_mot_no(struct _ENEMY_WORK* work);
#endif

#endif
