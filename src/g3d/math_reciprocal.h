/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `math_reciprocal`, owned by `g3d/g3d_anmchr.cpp`; for consumers whose
 * own `Vec3` cannot sit beside `nw4r/math.h`, which `g3d/g3d_anmchr.h` pulls in.
 */
#ifndef MHTRI_G3D_MATH_RECIPROCAL_H
#define MHTRI_G3D_MATH_RECIPROCAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 math_reciprocal(f32 value); /* 0x800610AC - the reciprocal helper */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_MATH_RECIPROCAL_H */
