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

/* 0x8012EC60 - whether the record's latched mode is 1 (`em_alt_mode_set` latches it, 0/1).  The real
 * signature takes the record: the body loads `+0x8AA` straight out of r3 (`lbz r3,2218(r3)`), and
 * every target call site that must set r3 does so explicitly (`em_act_effect_ck`'s `mr r3,r31`).  This
 * header owns that signature; the old `(void)` spelling was the workaround (a caller had to reach the
 * callee through a pointer string it could not name in the header).  A lander whose target call site
 * leaves the record already in r3 with no setup - the C handler `fn_8013BE60.c`'s `fn_8013D4C4` - is
 * the same spelling, `em_alt_mode_ck(self)`, and the compiler emits no setup because r3 already holds it.
 * If a unit's target genuinely sets nothing (it cannot name `self`), it uses a local function-pointer
 * typedef, never a second declaration - MWCC rejects two declarations of one C symbol
 * (`(10197)`/`(10248)`). */
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8012E968_H */
