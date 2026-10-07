/*
 * MSL/e_atan2.cpp - the MSL `__ieee754_atan2` (fdlibm shape): the two-argument arc tangent.
 *
 * RANGE. .text 0x80464560..0x804647B8 (1 function in the map, 0x258 B); .sdata2 0x8079CB40..0x8079CB98 (pi, -pi,
 *    -pi/2, pi/2, pi/4, -pi/4, 3pi/4, -3pi/4, 0.0, -0.0 and pi_lo).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_atan2` is the map's name. `__ieee754_atan2` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 * EVIDENCE. the pool holds the folded `pi + tiny` forms; the single call is `atan`.
 * RESIDUALS. none measured.
 * SHAPES. the constants are literals so the compiler pools them itself; the quadrant code `m` selects the sign cases.
 */
#pragma fp_contract off
#include "MSL/e_atan2.h"
#include "MSL/s_atan.h"
#include "MSL/fdlibm.h"

extern "C" f64 __ieee754_atan2(f64 y, f64 x)
{
    f64 z;
    s32 k;
    s32 m;
    s32 hx;
    s32 hy;
    s32 ix;
    s32 iy;
    u32 lx;
    u32 ly;

    hx = F64_HI(x);
    ix = hx & 0x7FFFFFFF;
    lx = F64_LO(x);
    hy = F64_HI(y);
    iy = hy & 0x7FFFFFFF;
    ly = F64_LO(y);
    if (((ix | ((lx | -lx) >> 31)) > 0x7FF00000) || ((iy | ((ly | -ly) >> 31)) > 0x7FF00000)) {
        return x + y;
    }
    if ((hx - 0x3FF00000 | lx) == 0) {
        return atan(y);
    }
    m = ((hy >> 31) & 1) | ((hx >> 30) & 2);
    if ((iy | ly) == 0) {
        switch (m) {
        case 0:
        case 1:
            return y;
        case 2:
            return 3.14159265358979311600e+00 + 1.0e-300;
        case 3:
            return -3.14159265358979311600e+00 - 1.0e-300;
        }
    }
    if ((ix | lx) == 0) {
        return (hy < 0) ? -1.57079632679489655800e+00 - 1.0e-300 : 1.57079632679489655800e+00 + 1.0e-300;
    }
    if (ix == 0x7FF00000) {
        if (iy == 0x7FF00000) {
            switch (m) {
            case 0:
                return 7.85398163397448278999e-01 + 1.0e-300;
            case 1:
                return -7.85398163397448278999e-01 - 1.0e-300;
            case 2:
                return 3.0 * 7.85398163397448278999e-01 + 1.0e-300;
            case 3:
                return -3.0 * 7.85398163397448278999e-01 - 1.0e-300;
            }
        } else {
            switch (m) {
            case 0:
                return 0.0;
            case 1:
                return -0.0;
            case 2:
                return 3.14159265358979311600e+00 + 1.0e-300;
            case 3:
                return -3.14159265358979311600e+00 - 1.0e-300;
            }
        }
    }
    if (iy == 0x7FF00000) {
        return (hy < 0) ? -1.57079632679489655800e+00 - 1.0e-300 : 1.57079632679489655800e+00 + 1.0e-300;
    }
    k = (iy - ix) >> 20;
    if (k > 60) {
        z = 1.57079632679489655800e+00 + 0.5 * 1.2246467991473531772e-16;
    } else if (hx < 0 && k < -60) {
        z = 0.0;
    } else {
        z = atan(__fabs(y / x));
    }
    switch (m) {
    case 0:
        return z;
    case 1:
        F64_HI(z) ^= 0x80000000;
        return z;
    case 2:
        return 3.14159265358979311600e+00 - (z - 1.2246467991473531772e-16);
    default:
        return (z - 1.2246467991473531772e-16) - 3.14159265358979311600e+00;
    }
}
