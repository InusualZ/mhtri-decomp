/* enemy/em_hit_mask_get.h - the declaration of `em_hit_mask_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HIT_MASK_GET_H
#define MHTRI_ENEMY_EM_HIT_MASK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u16 em_hit_mask_get(struct _ENEMY_WORK* work);
#ifdef __cplusplus
}
#endif

#endif
