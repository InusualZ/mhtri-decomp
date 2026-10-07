/*
 * MSL/e_acos.h - the declaration of `__ieee754_acos`, owned by `MSL/e_acos.cpp`.
 */
#ifndef MSL_E_ACOS_H
#define MSL_E_ACOS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463FFC - the arc cosine of `x` in radians. */
f64 __ieee754_acos(f64 x);

#ifdef __cplusplus
}
#endif

#endif
