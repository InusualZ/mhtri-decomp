/* enemy/em_approach_step.h - the declaration of `em_approach_step`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_APPROACH_STEP_H
#define MHTRI_ENEMY_EM_APPROACH_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_approach_step(struct _ENEMY_WORK* self, s32 a, s32 b);
#ifdef __cplusplus
}
#endif

#endif
