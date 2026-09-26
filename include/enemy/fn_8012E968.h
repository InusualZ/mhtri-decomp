/* The enemy control/predicate unit `enemy/fn_8012E968.cpp` (0x8012E968..0x8012EC74): the two one-byte
 * predicates the action band's handlers gate on, owned by that unit.
 *
 * Declarations moved here from `include/unsplit/enemy.h` (docs/plan.md 6.5 rule 2: an extern lives
 * with the TU that owns the symbol; the band header is a fallback, not the owner).
 */
#ifndef MHTRI_ENEMY_FN_8012E968_H
#define MHTRI_ENEMY_FN_8012E968_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8012EC3C - whether the record's `field_0x89F` is 2 or 3. */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);

/* 0x8012EC60 - whether the record's latched mode is 1 (`fn_80130A10` latches it, 0/1).  Its body
 * reads the work record from r3, and the landed call sites spell it with **no argument**
 * (`enemy/fn_801993E0.cpp`, `enemy/fn_801CA004.cpp`, `enemy/fn_8015E854.cpp`, `enemy/fn_8013BE60.c`
 * and this band's own `fn_80181E24`): they leave the record in r3 and the call must not materialise
 * one.  MWCC rejects the two spellings in one TU (`(10197) illegal function overloading`), so a unit
 * that has to pass `self` in r3 declares that overload locally instead
 * (`enemy/fn_801502C8.cpp`, `enemy/fn_8018B3B8.cpp`, `enemy/fn_80191598.cpp`,
 * `enemy/fn_801D428C.cpp`, `enemy/fn_801D80EC.cpp`, `enemy/fn_801DB8E0.cpp`).  The `(void)` spelling
 * is therefore the one this header owns - `(arg)` or a variadic `(...)` here changes every consumer's
 * code (measured: `enemy/fn_801CA004.cpp`'s `fn_801CA004` 87.77 -> 86.67, `enemy/fn_801993E0.cpp`'s
 * `fn_8019DB9C` 100 -> 93.75). */
u32 fn_8012EC60(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8012E968_H */
