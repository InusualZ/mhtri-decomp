/* Leaf header (docs/plan.md 6.5 rule 2): `rotVecZXY`, which `fn_8004CAD8.cpp` owns (C++ linkage: the map row is
 * `rotVecZXY__FPQ34nw4r4math4VEC3P10_CP_VECTOR`). */
#ifndef MHTRI_ROTVECZXY_H
#define MHTRI_ROTVECZXY_H

#include "types.h"

#ifdef __cplusplus
#include "nw4r/math.h"    /* VEC3 */
#include "ef/cp_vector.h" /* _CP_VECTOR */

/* 0x80051214 (0x54): rotates `v` about Z, then X, then Y by the three angles of `ang`. */
void rotVecZXY(VEC3* v, _CP_VECTOR* ang);
#endif

#endif /* MHTRI_ROTVECZXY_H */
