/*
 * MSL/s_cos.cpp - the MSL `cos` (fdlibm shape): argument reduction around `__kernel_cos` / `__kernel_sin`.
 *
 * RANGE. .text 0x80467918..0x804679E0 (1 function in the map, 0xC8 B); .sdata2 0x8079CEE0..0x8079CEE8 (the
 *    pooled 0.0 passed as the tail).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `cos` is the dump's name.
 * EVIDENCE. the pooled double is zero; the quadrant switch calls the kernels with sign flips.
 * RESIDUALS. none measured.
 * SHAPES. the reduction result lives in a two-element stack array.
 */
#include "MSL/s_cos.h"
#include "MSL/k_cos.h"
#include "MSL/k_sin.h"
#include "MSL/e_rem_pio2.h"
#include "MSL/fdlibm.h"

extern "C" f64 cos(f64 x)
{
    f64 y[2];
    f64 z = 0.0;
    s32 n;
    s32 ix = F64_HI(x) & 0x7FFFFFFF;

    if (ix <= 0x3FE921FB) {
        return __kernel_cos(x, z);
    }
    if (ix >= 0x7FF00000) {
        return x - x;
    }
    n = __ieee754_rem_pio2(x, y);
    switch (n & 3) {
    case 0:
        return __kernel_cos(y[0], y[1]);
    case 1:
        return -__kernel_sin(y[0], y[1], 1);
    case 2:
        return -__kernel_cos(y[0], y[1]);
    default:
        return __kernel_sin(y[0], y[1], 1);
    }
}
