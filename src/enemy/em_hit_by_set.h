/* enemy/em_hit_by_set.h - the declaration of `em_hit_by_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HIT_BY_SET_H
#define MHTRI_ENEMY_EM_HIT_BY_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_hit_by_set(struct _ENEMY_WORK* enemy, s16 timer, u8 index);
#ifdef __cplusplus
}
#endif

#endif
