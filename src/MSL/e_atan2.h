/*
 * MSL/e_atan2.h - the declaration of `__ieee754_atan2`, owned by `MSL/e_atan2.cpp`.
 */
#ifndef MSL_E_ATAN2_H
#define MSL_E_ATAN2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80464560 - the arc tangent of `y / x` in radians, in the quadrant of (x, y). */
f64 __ieee754_atan2(f64 y, f64 x);

#ifdef __cplusplus
}
#endif

#endif
