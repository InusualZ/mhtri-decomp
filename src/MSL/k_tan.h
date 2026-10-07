/*
 * MSL/k_tan.h - the declaration of `__kernel_tan`, owned by `MSL/k_tan.cpp`.
 */
#ifndef MSL_K_TAN_H
#define MSL_K_TAN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467320 - the tangent of `x` (with tail `y`) on [-pi/4, pi/4]; `iy` is 1 for tan and -1 for -1/tan. */
f64 __kernel_tan(f64 x, f64 y, int iy);

#ifdef __cplusplus
}
#endif

#endif
