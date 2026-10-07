/*
 * MSL/s_cos.h - the declaration of `cos`, owned by `MSL/s_cos.cpp`.
 */
#ifndef MSL_S_COS_H
#define MSL_S_COS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467918 - the cosine of `x` in radians. */
f64 cos(f64 x);

#ifdef __cplusplus
}
#endif

#endif
