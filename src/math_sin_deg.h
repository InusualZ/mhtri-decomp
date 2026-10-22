/* Leaf header (docs/plan.md 6.5 rule 2): `fn_80056F24.cpp`'s math_sin_deg */
#ifndef MHTRI_MATH_SIN_DEG_H
#define MHTRI_MATH_SIN_DEG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800591AC - the sine of `deg` degrees (`SinFIdx(deg * 256 / 360)`, nw4r's SinDeg shape; C linkage in the owner)
 * (GUESS name). */
f32 math_sin_deg(f32 deg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MATH_SIN_DEG_H */
