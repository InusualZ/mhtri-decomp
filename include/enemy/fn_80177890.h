/* The `enemy/fn_80177890.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `enemy/fn_80178378.cpp`'s master dispatcher tail-calls `fn_80177BA4`, which the owner defines as
 * `void fn_80177BA4(_ENEMY_WORK* self)` (its `state_sub` dispatcher).
 */
#ifndef MHTRI_ENEMY_FN_80177890_H
#define MHTRI_ENEMY_FN_80177890_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_80177BA4(struct _ENEMY_WORK* self); /* 0x80177BA4 - the mid sub-state dispatcher */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80177890_H */
