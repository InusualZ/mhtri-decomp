/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `vec3_float_copy`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_VEC3_FLOAT_COPY_H
#define MHTRI_VEC3_FLOAT_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80045330 - copies the three floats at `src` to `dst` (GUESS name). */
void vec3_float_copy(f32* dst, const f32* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_FLOAT_COPY_H */
