/* ef/eft_stun_set.h - the declarations of `eft_stun_set`, which `ef/fn_8010BDE4.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_EF_EFT_STUN_SET_H
#define MHTRI_EF_EFT_STUN_SET_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
void eft_stun_set(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* offset, u32 kind, f32 scale);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_STUN_SET_H */
