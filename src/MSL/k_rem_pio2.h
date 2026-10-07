/*
 * MSL/k_rem_pio2.h - the declaration of `__kernel_rem_pio2`, owned by `MSL/k_rem_pio2.cpp`.
 */
#ifndef MSL_K_REM_PIO2_H
#define MSL_K_REM_PIO2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80465BA8 - reduces the `nx`-term 24-bit-chunk vector `x` (exponent `e0`) modulo pi/2 into `y[0..prec]` using the
 * 2/pi table `ipio2`, and returns the quadrant count. */
s32 __kernel_rem_pio2(f64* x, f64* y, s32 e0, s32 nx, s32 prec, const s32* ipio2);

#ifdef __cplusplus
}
#endif

#endif
