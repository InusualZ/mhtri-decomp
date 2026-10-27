/* enemy/em_eft_fx_spawn_at.h - the declarations of `em_eft_fx_spawn_at` and `em_eft_fx_spawn_free`, which `enemy/em_eft_fx.cpp` owns (docs/plan.md 6.5
 * rule 2); the signatures are the owner's definitions. */
#ifndef MHTRI_ENEMY_EM_EFT_FX_SPAWN_AT_H
#define MHTRI_ENEMY_EM_EFT_FX_SPAWN_AT_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8011D4AC */
void em_eft_fx_spawn_at(struct _ENEMY_WORK* self, u32 type, u32 id, u32 timer, const nw4r::math::VEC3* pos,
                        f32 scale);
/* 0x8011D7B0 */
void em_eft_fx_spawn_free(u32 type, const nw4r::math::VEC3* pos, const _CP_VECTOR* rot, u8 area, u8 demo,
                          f32 scale);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_EFT_FX_SPAWN_AT_H */
