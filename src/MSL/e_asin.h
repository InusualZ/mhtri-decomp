/*
 * MSL/e_asin.h - the declaration of `__ieee754_asin`, owned by `MSL/e_asin.cpp`.
 */
#ifndef MSL_E_ASIN_H
#define MSL_E_ASIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804642C8 - the arc sine of `x` in radians. */
f64 __ieee754_asin(f64 x);

#ifdef __cplusplus
}
#endif

#endif
