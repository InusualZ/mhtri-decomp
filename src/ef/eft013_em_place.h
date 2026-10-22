/* ef/eft013_em_place.h - the declarations of `eft013_em_place`, `eft013_em_spawn_head` and `eft013_spark_set`, which `ef/eft013_fx.cpp` owns (docs/plan.md 6.5
 * rule 2); the signatures are the owner's definitions. */
#ifndef MHTRI_EF_EFT013_EM_PLACE_H
#define MHTRI_EF_EFT013_EM_PLACE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80107518 */
void eft013_em_place(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, f32 scale, u32 b);
/* 0x8010A8CC */
void eft013_em_spawn_head(struct _ENEMY_WORK* self, u32 a, f32 scale);
/* 0x80107804 */
void eft013_spark_set(struct _ENEMY_WORK* self, u32 a);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT013_EM_PLACE_H */
