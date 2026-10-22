/* calcMtxAng3.h - the declarations of `calcMtxAng3`, `vec3_polar_offset`, `vec3_polar_offset2`, `vec3_segment_offset`, `vec3_segment_offset2`, which `fn_8004CAD8.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_CALCMTXANG3_H
#define MHTRI_CALCMTXANG3_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 calcMtxAng3(nw4r::math::MTX34* mtx);
void vec3_polar_offset(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u32 angle, f32 reach);
void vec3_polar_offset2(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u32 yaw, u32 pitch, f32 reach);
void vec3_segment_offset(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u32 angle, f32 reach, f32 t);
void vec3_segment_offset2(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u32 yaw, u32 pitch, f32 reach, f32 t);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CALCMTXANG3_H */
