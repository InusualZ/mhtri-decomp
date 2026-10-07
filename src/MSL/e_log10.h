/*
 * MSL/e_log10.h - the declaration of `__ieee754_log10`, owned by `MSL/e_log10.cpp`.
 */
#ifndef MSL_E_LOG10_H
#define MSL_E_LOG10_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80464DEC - the base-10 logarithm of `x`. */
f64 __ieee754_log10(f64 x);

#ifdef __cplusplus
}
#endif

#endif
