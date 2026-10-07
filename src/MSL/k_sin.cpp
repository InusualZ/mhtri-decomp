/*
 * MSL/k_sin.cpp - the MSL `__kernel_sin` (fdlibm shape): the sine of `x` on [-pi/4, pi/4] with a tail `y`.
 *
 * RANGE. .text 0x80467260..0x80467320 (1 function in the map, 0xC0 B); .sdata2 0x8079CE38..0x8079CE70 (the seven
 *    pooled coefficients).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__kernel_sin` is the map's name.
 * EVIDENCE. the seven doubles in `.sdata2` are the sine polynomial coefficients S2..S6, S1 and 0.5, in order of use.
 * RESIDUALS. none measured.
 * SHAPES. the coefficients are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/k_sin.h"
#include "MSL/fdlibm.h"

extern "C" f64 __kernel_sin(f64 x, f64 y, int iy)
{
    f64 z;
    f64 v;
    f64 r;
    s32 ix = F64_HI(x) & 0x7FFFFFFF;

    if (ix < 0x3E400000) {
        if ((int)x == 0) {
            return x;
        }
    }
    z = x * x;
    v = z * x;
    r = 0.00833333333332249 + z * (-0.0001984126982985795 + z * (2.7557313707070068e-06 + z * (-2.5050760253406863e-08 + z * 1.58969099521155e-10)));
    if (iy == 0) {
        return x + v * (-0.16666666666666632 + z * r);
    }
    return x - ((z * (0.5 * y - v * r) - y) - v * -0.16666666666666632);
}
