/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s vec3_random_offset */
#ifndef MHTRI_VEC3_RANDOM_OFFSET_H
#define MHTRI_VEC3_RANDOM_OFFSET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80051B7C - writes `in` moved by a random polar offset of up to `radius` to `out` (GUESS name). */
void vec3_random_offset(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 radius, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_RANDOM_OFFSET_H */
