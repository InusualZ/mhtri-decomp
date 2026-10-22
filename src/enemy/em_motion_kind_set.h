/* enemy/em_motion_kind_set.h - the declaration of `em_motion_kind_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOTION_KIND_SET_H
#define MHTRI_ENEMY_EM_MOTION_KIND_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_motion_kind_set(struct _ENEMY_WORK* self, u32 mode, u32 value);
#ifdef __cplusplus
}
#endif

#endif
