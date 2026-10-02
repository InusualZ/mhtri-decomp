/* enemy/em_target_pos_set.h - the declaration of `em_target_pos_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_TARGET_POS_SET_H
#define MHTRI_ENEMY_EM_TARGET_POS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_target_pos_set(struct _ENEMY_WORK *self, u32 a);
#ifdef __cplusplus
}
#endif

#endif
