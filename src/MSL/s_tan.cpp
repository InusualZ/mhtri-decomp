/*
 * MSL/s_tan.cpp - the MSL `tan` (fdlibm shape): argument reduction around `__kernel_tan`.
 *
 * RANGE. .text 0x80467EEC..0x80467F64 (1 function in the map, 0x78 B); .sdata2 0x8079CF30..0x8079CF38 (the
 *    pooled 0.0 passed as the tail).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `tan` is the dump's name.
 * EVIDENCE. the pooled double is zero; the kernel's last argument is 1 for even and -1 for odd quadrants.
 * RESIDUALS. none measured.
 * SHAPES. the reduction result lives in a two-element stack array.
 */
#include "MSL/s_tan.h"
#include "MSL/k_tan.h"
#include "MSL/e_rem_pio2.h"
#include "MSL/fdlibm.h"

extern "C" f64 tan(f64 x)
{
    f64 y[2];
    f64 z = 0.0;
    s32 n;
    s32 ix = F64_HI(x) & 0x7FFFFFFF;

    if (ix <= 0x3FE921FB) {
        return __kernel_tan(x, z, 1);
    }
    if (ix >= 0x7FF00000) {
        return x - x;
    }
    n = __ieee754_rem_pio2(x, y);
    return __kernel_tan(y[0], y[1], 1 - ((n & 1) << 1));
}
