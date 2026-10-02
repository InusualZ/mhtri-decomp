/* enemy/em_fall_start.h - the declaration of `em_fall_start`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_FALL_START_H
#define MHTRI_ENEMY_EM_FALL_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_fall_start(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
