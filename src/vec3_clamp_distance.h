/* Leaf header (docs/plan.md 6.5 rule 2): `vec3_clamp_distance` (0x800512C0), owned by `fn_8004CAD8.cpp` (the nw4r math
 * band at the root of src/).  C linkage (the map row is a plain name). */
#ifndef MHTRI_VEC3_CLAMP_DISTANCE_H
#define MHTRI_VEC3_CLAMP_DISTANCE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800512C0 - pulls `pos` to within `limit` of `center`; returns how far it was beyond the limit. */
f32 vec3_clamp_distance(VEC3* center, VEC3* pos, f32 limit);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_CLAMP_DISTANCE_H */
