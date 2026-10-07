/*
 * MSL/e_fmod.h - the declaration of `__ieee754_fmod`, owned by `MSL/e_fmod.cpp`.
 */
#ifndef MSL_E_FMOD_H
#define MSL_E_FMOD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804647B8 - the floating-point remainder of `x / y`. */
f64 __ieee754_fmod(f64 x, f64 y);

#ifdef __cplusplus
}
#endif

#endif
