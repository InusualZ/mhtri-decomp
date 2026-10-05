/* enemy/em_act_step_arm.h - the declaration of `em_act_step_arm`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ACT_STEP_ARM_H
#define MHTRI_ENEMY_EM_ACT_STEP_ARM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_act_step_arm(struct _ENEMY_WORK *self, u32 a, u32 b, u32 c);
#ifdef __cplusplus
}
#endif

#endif
