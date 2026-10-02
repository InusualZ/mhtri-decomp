/* enemy/em_approach_start.h - the declaration of `em_approach_start`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_APPROACH_START_H
#define MHTRI_ENEMY_EM_APPROACH_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);
#ifdef __cplusplus
}
#endif

#endif
