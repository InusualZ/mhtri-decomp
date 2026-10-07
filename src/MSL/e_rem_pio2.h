/*
 * MSL/e_rem_pio2.h - the declaration of `__ieee754_rem_pio2`, owned by `MSL/e_rem_pio2.cpp`.
 */
#ifndef MSL_E_REM_PIO2_H
#define MSL_E_REM_PIO2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80465714 - reduces `x` modulo pi/2 into `y[0] + y[1]` and returns the quadrant count. */
s32 __ieee754_rem_pio2(f64 x, f64* y);

#ifdef __cplusplus
}
#endif

#endif
