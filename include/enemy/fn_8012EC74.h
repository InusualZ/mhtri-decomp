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

#ifdef __cplusplus
}
#endif

#endif
