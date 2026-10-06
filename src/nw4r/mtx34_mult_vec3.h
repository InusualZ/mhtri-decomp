/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `mtx34_mult_vec3` (0x800514FC), the matrix-times-vector helper
 * `fn_8004CAD8.cpp`'s range owns (`out = mtx * in`, the vector taken as a point).  GUESS name: from its callers (the
 * ef draw strategies transform quad corners and ribbon edges through it).
 */
#ifndef MHTRI_NW4R_MTX34_MULT_VEC3_H
#define MHTRI_NW4R_MTX34_MULT_VEC3_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void mtx34_mult_vec3(VEC3* out, const MTX34* mtx, const VEC3* in);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW4R_MTX34_MULT_VEC3_H */
