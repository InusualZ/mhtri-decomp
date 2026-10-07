/*
 * MSL/s_ceil.h - the declaration of `ceil`, owned by `MSL/s_ceil.cpp`.
 */
#ifndef MSL_S_CEIL_H
#define MSL_S_CEIL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804677A0 - the smallest integral value not less than `x`. */
f64 ceil(f64 x);

#ifdef __cplusplus
}
#endif

#endif
