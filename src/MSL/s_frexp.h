/*
 * MSL/s_frexp.h - the declaration of `frexp`, owned by `MSL/s_frexp.cpp`.
 */
#ifndef MSL_S_FREXP_H
#define MSL_S_FREXP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467B30 - splits `x` into a fraction in [0.5, 1) and a power of two stored through `eptr`. */
f64 frexp(f64 x, s32* eptr);

#ifdef __cplusplus
}
#endif

#endif
