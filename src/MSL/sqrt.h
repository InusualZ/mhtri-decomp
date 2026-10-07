/*
 * MSL/sqrt.h - the declaration of `sqrt`, the 4-byte thunk at 0x804681C8 owned by `MSL/w_math.cpp`.
 */
#ifndef MSL_SQRT_H
#define MSL_SQRT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804681C8 - the square root of `x`. */
f64 sqrt(f64 x);

#ifdef __cplusplus
}
#endif

#endif
