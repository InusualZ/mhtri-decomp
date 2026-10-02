/* enemy/em_wave_amp.h - the declaration of `em_wave_amp`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_WAVE_AMP_H
#define MHTRI_ENEMY_EM_WAVE_AMP_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_wave_amp(nw4r::math::VEC3* out, nw4r::math::VEC3* a, f32 e, u32 c, u16 d);
#ifdef __cplusplus
}
#endif

#endif
