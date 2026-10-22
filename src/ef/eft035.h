/* ef/eft035.h - the declarations `ef/eft035.cpp` owns that other units call (docs/plan.md 6.5 rule 2); C++ linkage, so the
 * front-end emits the map's manglings. */
#ifndef MHTRI_EF_EFT035_H
#define MHTRI_EF_EFT035_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

struct _ENEMY_WORK;

/* 0x802F2238 */
void eft035_set(struct _ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale);

#endif /* MHTRI_EF_EFT035_H */
