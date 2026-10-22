/* enemy/em015_act13_early_ck.h - leaf header (docs/plan.md 6.5 rule 2): `enemy/em015_prog.cpp`'s
 *   `em015_act13_early_ck`, for `lobby/fn_802FA9A0.cpp`.  C linkage (the owner defines it inside its `extern "C"`
 *   block). */
#ifndef MHTRI_ENEMY_EM015_ACT13_EARLY_CK_H
#define MHTRI_ENEMY_EM015_ACT13_EARLY_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8018257C: whether the em015 is in the opening sub-states of its action 13. */
u32 em015_act13_early_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM015_ACT13_EARLY_CK_H */
