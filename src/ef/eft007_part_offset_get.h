/* ef/eft007_part_offset_get.h - the declaration of `eft007_part_offset_get`, which `ef/eft007.cpp` owns (docs/plan.md 6.5 rule 2); the
 * signature is the owner's definition. */
#ifndef MHTRI_EF_EFT007_PART_OFFSET_GET_H
#define MHTRI_EF_EFT007_PART_OFFSET_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80103968 */
void eft007_part_offset_get(struct _ENEMY_WORK* enemy, u32 part, nw4r::math::VEC3* offset);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT007_PART_OFFSET_GET_H */
