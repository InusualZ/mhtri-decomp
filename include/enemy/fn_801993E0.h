/* The enemy unit `enemy/fn_801993E0.cpp` (0x801993E0..0x8019ED34): the enemy action band's
 * effect-slot helpers, which `enemy/fn_801A4504.cpp` calls.
 *
 * Declarations moved here from `include/unsplit/enemy.h` (docs/plan.md 6.5 rule 2: an extern lives
 * with the TU that owns the symbol) when the unit landed and made them owned.  The signatures are
 * the call sites' registers; `fn_8019E9AC` is `s32` because the landed caller tests its result with
 * a signed compare.
 */
#ifndef MHTRI_ENEMY_FN_801993E0_H
#define MHTRI_ENEMY_FN_801993E0_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* r3 the work record, r4 the slot id; arms one action slot. */
void fn_8019E960(struct _ENEMY_WORK* self, s32 slot);
/* r3 the work record; returns the slot word the caller compares against 0. */
s32 fn_8019E9AC(struct _ENEMY_WORK* self, s32 slot);
/* r3 the work record; the per-joint slot release `fn_801A94C0` tail-calls. */
void fn_8019EA04(struct _ENEMY_WORK* self);
/* 0x8019E840 - r3 the work record; returns the 1/0 flag `enemy/fn_8019ED34.cpp`'s `fn_8019F07C`
 * tests (declared by that unit; rule 2: this unit owns the address). */
u32 fn_8019E840(struct _ENEMY_WORK* self);
/* r3 the work record and three scalars. */
void fn_8019EC38(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);

#ifdef __cplusplus
}
#endif

#endif
