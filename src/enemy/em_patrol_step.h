/* enemy/em_patrol_step.h - the declaration of `em_patrol_step`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_PATROL_STEP_H
#define MHTRI_ENEMY_EM_PATROL_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_patrol_step(struct _ENEMY_WORK* self, s32 a);
#ifdef __cplusplus
}
#endif

#endif
