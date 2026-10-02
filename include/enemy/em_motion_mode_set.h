/* enemy/em_motion_mode_set.h - the declaration of `em_motion_mode_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOTION_MODE_SET_H
#define MHTRI_ENEMY_EM_MOTION_MODE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_motion_mode_set(struct _ENEMY_WORK* self, u8 mode);
#ifdef __cplusplus
}
#endif

#endif
