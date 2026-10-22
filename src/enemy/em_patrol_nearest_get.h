/* enemy/em_patrol_nearest_get.h - the declaration of `em_patrol_nearest_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_PATROL_NEAREST_GET_H
#define MHTRI_ENEMY_EM_PATROL_NEAREST_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_patrol_nearest_get(struct _ENEMY_WORK *self, u32 a);
#ifdef __cplusplus
}
#endif

#endif
