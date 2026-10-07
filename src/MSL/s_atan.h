/*
 * MSL/s_atan.h - the declaration of `atan`, owned by `MSL/s_atan.cpp`.
 */
#ifndef MSL_S_ATAN_H
#define MSL_S_ATAN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467570 - the arctangent of `x` in radians. */
f64 atan(f64 x);

#ifdef __cplusplus
}
#endif

#endif
