/* enemy/em_motion_param_set.h - the declaration of `em_motion_param_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOTION_PARAM_SET_H
#define MHTRI_ENEMY_EM_MOTION_PARAM_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* The value comes first: every caller loads it before the index, and the body stores it (+0x1D8) before the halfword
 * index (+0x1DC). */
void em_motion_param_set(struct _ENEMY_WORK* self, f32 value, s16 index);
#ifdef __cplusplus
}
#endif

#endif
