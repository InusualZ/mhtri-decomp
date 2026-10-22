/* enemy/em_wave_segment.h - the declaration of `em_wave_segment`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_WAVE_SEGMENT_H
#define MHTRI_ENEMY_EM_WAVE_SEGMENT_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_wave_segment(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u32 c, u16 d,
                     f32 e);
#ifdef __cplusplus
}
#endif

#endif
