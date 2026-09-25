/* The `enemy/fn_80178128.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `enemy/fn_80178378.cpp`'s master dispatcher (`fn_8017F138`) tail-calls `fn_8017827C`, the mid
 * sub-state dispatcher that unit defines as `extern "C" void fn_8017827C(_ENEMY_WORK* self)`.
 */
#ifndef MHTRI_ENEMY_FN_80178128_H
#define MHTRI_ENEMY_FN_80178128_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_8017827C(struct _ENEMY_WORK* self); /* 0x8017827C - the mid sub-state dispatcher */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80178128_H */
