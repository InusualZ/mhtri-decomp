/* enemy/em018_act13_early_ck.h - leaf header (docs/plan.md 6.5 rule 2): `enemy/em018_prog.cpp`'s
 *   `em018_act13_early_ck` (0x8019E580), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_ENEMY_EM018_ACT13_EARLY_CK_H
#define MHTRI_ENEMY_EM018_ACT13_EARLY_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8019E580: whether the em018 is in action 13 at sub-state 5 or below. */
u32 em018_act13_early_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM018_ACT13_EARLY_CK_H */
