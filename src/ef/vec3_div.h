/* ef/vec3_div.h - the declarations of `vec3_div`, which `ef/ef_particlemanager.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_EF_VEC3_DIV_H
#define MHTRI_EF_VEC3_DIV_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void vec3_div(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 divisor);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_VEC3_DIV_H */
