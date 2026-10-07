/*
 * MSL/k_sin.h - the declaration of `__kernel_sin`, owned by `MSL/k_sin.cpp`.
 */
#ifndef MSL_K_SIN_H
#define MSL_K_SIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467260 - the sine of `x` (with tail `y`, used when `iy` is nonzero) on [-pi/4, pi/4]. */
f64 __kernel_sin(f64 x, f64 y, int iy);

#ifdef __cplusplus
}
#endif

#endif
