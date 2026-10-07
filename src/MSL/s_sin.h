/*
 * MSL/s_sin.h - the declaration of `sin`, owned by `MSL/s_sin.cpp`.
 */
#ifndef MSL_S_SIN_H
#define MSL_S_SIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467E20 - the sine of `x` in radians. */
f64 sin(f64 x);

#ifdef __cplusplus
}
#endif

#endif
