/* enemy/em_data_point_get.h - the declaration of `em_data_point_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_DATA_POINT_GET_H
#define MHTRI_ENEMY_EM_DATA_POINT_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_data_point_get(struct _ENEMY_WORK *self, u16 a, Vec3 *out);
#ifdef __cplusplus
}
#endif

#endif
