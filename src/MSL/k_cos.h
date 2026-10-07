/*
 * MSL/k_cos.h - the declaration of `__kernel_cos`, owned by `MSL/k_cos.cpp`.
 */
#ifndef MSL_K_COS_H
#define MSL_K_COS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80465A98 - the cosine of `x` (with tail `y`) on [-pi/4, pi/4]. */
f64 __kernel_cos(f64 x, f64 y);

#ifdef __cplusplus
}
#endif

#endif
