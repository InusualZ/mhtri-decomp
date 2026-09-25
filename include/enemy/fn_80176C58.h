/* The `enemy/fn_80176C58.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `enemy/fn_80178378.cpp`'s master dispatcher tail-calls `fn_801775C0`, which the owner defines as
 * `extern "C" void fn_801775C0(_ENEMY_WORK* self)` (its sub-state dispatcher).
 */
#ifndef MHTRI_ENEMY_FN_80176C58_H
#define MHTRI_ENEMY_FN_80176C58_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_801775C0(struct _ENEMY_WORK* self); /* 0x801775C0 - the low sub-state dispatcher */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80176C58_H */
