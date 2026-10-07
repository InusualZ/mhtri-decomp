/*
 * MSL/s_copysign.h - the declaration of `copysign`, owned by `MSL/s_copysign.cpp`.
 */
#ifndef MSL_S_COPYSIGN_H
#define MSL_S_COPYSIGN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804678EC - the magnitude of `x` with the sign of `y`. */
f64 copysign(f64 x, f64 y);

#ifdef __cplusplus
}
#endif

#endif
