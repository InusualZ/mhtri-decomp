/*
 * MSL/k_cos.cpp - the MSL `__kernel_cos` (fdlibm shape): the cosine of `x` on [-pi/4, pi/4] with a tail `y`.
 *
 * RANGE. .text 0x80465A98..0x80465BA8 (1 function in the map, 0x110 B); .sdata2 0x8079CDB0..0x8079CDF8 (the nine
 *    pooled constants).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__kernel_cos` is the map's name.
 * EVIDENCE. the constants in `.sdata2` are 1.0, the six cosine polynomial coefficients, 0.5 and 0.28125, in order
 *    of use.
 * RESIDUALS. none measured.
 * SHAPES. the coefficients are literals so the compiler pools them itself; `qx` is built from the high word of `x`.
 */
#pragma fp_contract off
#include "MSL/k_cos.h"
#include "MSL/fdlibm.h"

extern "C" f64 __kernel_cos(f64 x, f64 y)
{
    f64 a;
    f64 hz;
    f64 z;
    f64 r;
    f64 qx;
    s32 ix = F64_HI(x) & 0x7FFFFFFF;

    if (ix < 0x3E400000) {
        if ((int)x == 0) {
            return 1.0;
        }
    }
    z = x * x;
    r = z * (0.0416666666666666 + z * (-0.001388888888887411 + z * (2.480158728947673e-05 + z * (-2.7557314351390663e-07 + z * (2.087572321298175e-09 + z * -1.1359647557788195e-11)))));
    if (ix < 0x3FD33333) {
        return 1.0 - (0.5 * z - (z * r - x * y));
    }
    if (ix > 0x3FE90000) {
        qx = 0.28125;
    } else {
        F64_HI(qx) = ix - 0x00200000;
        F64_LO(qx) = 0;
    }
    hz = 0.5 * z - qx;
    a = 1.0 - qx;
    return a - (hz - (z * r - x * y));
}
