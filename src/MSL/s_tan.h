/*
 * MSL/s_tan.h - the declaration of `tan`, owned by `MSL/s_tan.cpp`.
 */
#ifndef MSL_S_TAN_H
#define MSL_S_TAN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467EEC - the tangent of `x` in radians. */
f64 tan(f64 x);

#ifdef __cplusplus
}
#endif

#endif
