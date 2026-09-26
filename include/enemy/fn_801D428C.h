/* The enemy unit `enemy/fn_801D428C.cpp` (0x801D428C..0x801D80EC): the enemy action band's
 * per-motion dispatchers and helpers.
 *
 * `fn_801D6694` moved here from `enemy/fn_801B0010.cpp` when this unit landed (docs/plan.md 6.5
 * rule 2: an extern lives with the TU that owns the symbol).  Its previous home was the band header
 * `include/unsplit/enemy.h`, whose comment said the bracketing registered units named different
 * modules - true until this unit was registered.
 */
#ifndef MHTRI_ENEMY_FN_801D428C_H
#define MHTRI_ENEMY_FN_801D428C_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* r3 the work record; answers in r3 (its callers compare the result against 1). */
u32 fn_801D6694(_ENEMY_WORK* work);

#ifdef __cplusplus
}
#endif

#endif
