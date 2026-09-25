/* The declarations `enemy/fn_80191598.cpp` owns (docs/plan.md 6.5 rule 2: an extern lives with the TU
 * that defines the symbol).  Created when `enemy/fn_801993E0.cpp` registered as a consumer: its
 * `fn_8019D8B8` and `fn_8019D9BC` call into the aim/action group this unit holds, and no header for
 * the owner existed yet (the rule-2 backlog's "owners needing a header" shape).
 */
#ifndef MHTRI_ENEMY_FN_80191598_H
#define MHTRI_ENEMY_FN_80191598_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80192370 - r3 (`self`) and r4, a selector; the return is compared against 0 by its caller, so
 * it reports a status word rather than a pointer. */
u32 fn_80192370(struct _ENEMY_WORK* self, u32 a);
/* 0x80192618 - r3 (`self`) only, no return; the tail every action of this band runs. */
void fn_80192618(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80191598_H */
