/* enemy/em_act_arm_unless_down.h - the declaration of `em_act_arm_unless_down`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ACT_ARM_UNLESS_DOWN_H
#define MHTRI_ENEMY_EM_ACT_ARM_UNLESS_DOWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_act_arm_unless_down(struct _ENEMY_WORK* self, u8 a, u8 b);
#ifdef __cplusplus
}
#endif

#endif
