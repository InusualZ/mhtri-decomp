/* enemy/em_motion_timer_arm.h - the declaration of `em_motion_timer_arm`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOTION_TIMER_ARM_H
#define MHTRI_ENEMY_EM_MOTION_TIMER_ARM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_motion_timer_arm(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
