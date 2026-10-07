/*
 * MSL/w_math.h - the libm wrapper entry points `MSL/w_math.cpp` owns (each a 4-byte tail call into the fdlibm
 *    kernel) and the software square root they sit beside.
 */
#ifndef MSL_W_MATH_H
#define MSL_W_MATH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467F64 - the arc cosine of `x`. */
f64 acos(f64 x);

/* 0x80467F68 - the arc sine of `x`. */
f64 asin(f64 x);

/* 0x80467F6C - the arc tangent of `y / x` in the quadrant of (x, y). */
f64 atan2(f64 y, f64 x);

/* 0x80467F70 - the floating-point remainder of `x / y`. */
f64 fmod(f64 x, f64 y);

/* 0x80467F74 - the base-10 logarithm of `x`. */
f64 log10(f64 x);

/* 0x80467F78 - `x` raised to the power `y`. */
f64 pow(f64 x, f64 y);

/* 0x80467F7C - the square root of `x` (Newton iteration), with `errno` set for negative input. */
f64 __ieee754_sqrt(f64 x);

/* 0x804681C4 - the quiet NaN selected by the tag string (the body is a bare return). */
f64 nan(const char* tag);

/* 0x804681C8 - the square root of `x`. */
f64 sqrt(f64 x);

#ifdef __cplusplus
}
#endif

#endif
