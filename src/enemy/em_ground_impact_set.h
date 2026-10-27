/* enemy/em_ground_impact_set.h - the declaration of `em_ground_impact_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the owner's definition. */
#ifndef MHTRI_ENEMY_EM_GROUND_IMPACT_SET_H
#define MHTRI_ENEMY_EM_GROUND_IMPACT_SET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_ground_impact_set(struct _ENEMY_WORK* self, u8 kind, u8 fx, f32 scale, VEC3* offset);
#ifdef __cplusplus
}
#endif

#endif
