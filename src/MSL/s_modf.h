/*
 * MSL/s_modf.h - the declaration of `modf`, owned by `MSL/s_modf.cpp`.
 */
#ifndef MSL_S_MODF_H
#define MSL_S_MODF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467D24 - returns the fractional part of `x` and stores the integral part through `iptr`. */
f64 modf(f64 x, f64* iptr);

#ifdef __cplusplus
}
#endif

#endif
