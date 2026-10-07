/*
 * MSL/e_pow.h - the declaration of `__ieee754_pow`, owned by `MSL/e_pow.cpp`.
 */
#ifndef MSL_E_POW_H
#define MSL_E_POW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80464F00 - `x` raised to the power `y`. */
f64 __ieee754_pow(f64 x, f64 y);

#ifdef __cplusplus
}
#endif

#endif
