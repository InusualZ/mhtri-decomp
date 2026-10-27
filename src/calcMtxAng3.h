/* calcMtxAng3.h - the declarations of `calcMtxAng3`, `vec3_polar_offset`, `vec3_polar_offset2`, `vec3_segment_offset`, `vec3_segment_offset2`, which `fn_8004CAD8.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * owner's signatures. */
#ifndef MHTRI_CALCMTXANG3_H
#define MHTRI_CALCMTXANG3_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 calcMtxAng3(nw4r::math::MTX34* mtx);
void vec3_polar_offset(VEC3* out, const VEC3* center, f32 dist, u16 yaw);
void vec3_polar_offset2(VEC3* out, const VEC3* center, f32 dist, u16 pitch, u16 yaw);
void vec3_segment_offset(VEC3* out, const VEC3* a, const VEC3* b, f32 dist, f32 t, u16 yaw);
void vec3_segment_offset2(VEC3* out, const VEC3* a, const VEC3* b, f32 dist, f32 t, u16 pitch, u16 yaw);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CALCMTXANG3_H */
