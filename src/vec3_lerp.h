/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8`'s `vec3_lerp`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_VEC3_LERP_H
#define MHTRI_VEC3_LERP_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80052234 (fn_8004CAD8): the point `t` of the way from `from` to `to`. */
void vec3_lerp(nw4r::math::VEC3* out, const nw4r::math::VEC3* from, const nw4r::math::VEC3* to, f32 t);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_LERP_H */
