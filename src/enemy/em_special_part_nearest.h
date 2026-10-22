/* enemy/em_special_part_nearest.h - the declaration of `em_special_part_nearest`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_SPECIAL_PART_NEAREST_H
#define MHTRI_ENEMY_EM_SPECIAL_PART_NEAREST_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_special_part_nearest(struct _ENEMY_WORK *self, Vec3 *pos, u32 a);
#ifdef __cplusplus
}
#endif

#endif
