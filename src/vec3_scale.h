/*
 * Leaf header (docs/plan.md 6.5 rule 2): `vec3_scale` (0x80051EE0, `out = in * scale`), owned by
 * `src/fn_8004CAD8.cpp`.  `fn_8004CAD8.h` includes this one, so there is one declaration; it is separate so
 * `ef/ef_particlemanager.cpp`, whose local declarations disagree with several of that header's, can reach it.
 */
#ifndef MHTRI_VEC3_SCALE_H
#define MHTRI_VEC3_SCALE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void vec3_scale(VEC3* out, VEC3* in, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_SCALE_H */
