/* enemy/em_mot_set.h - the declaration of `em_mot_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOT_SET_H
#define MHTRI_ENEMY_EM_MOT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_mot_set(struct _ENEMY_WORK* self, s32 motion, s32 arg2, s32 arg3);
#ifdef __cplusplus
}
#endif

#endif
