/*
 * MSL/s_modf.cpp - the MSL `modf` (fdlibm shape): splits a double into integral and fractional parts.
 *
 * RANGE. .text 0x80467D24..0x80467E20 (1 function in the map, 0xFC B); no data.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `modf` names the unnamed row (GUESS: it returns `x - *iptr` and stores the integral part).
 * EVIDENCE. no data; the three integral cases store the sign-only word as the fraction.
 * RESIDUALS. none measured.
 * SHAPES. the integral part is built word by word through `iptr`.
 */
#pragma fp_contract off
#include "MSL/s_modf.h"
#include "MSL/fdlibm.h"

extern "C" f64 modf(f64 x, f64* iptr)
{
    s32 i0;
    u32 i1;
    s32 j0;
    u32 i;

    i0 = F64_HI(x);
    i1 = F64_LO(x);
    j0 = ((i0 >> 20) & 0x7FF) - 0x3FF;
    if (j0 < 20) {
        if (j0 < 0) {
            F64_HI(*iptr) = i0 & 0x80000000;
            F64_LO(*iptr) = 0;
            return x;
        }
        i = 0x000FFFFF >> j0;
        if (((i0 & i) | i1) == 0) {
            *iptr = x;
            F64_HI(x) &= 0x80000000;
            F64_LO(x) = 0;
            return x;
        }
        F64_HI(*iptr) = i0 & ~i;
        F64_LO(*iptr) = 0;
        return x - *iptr;
    } else if (j0 > 51) {
        *iptr = x;
        F64_HI(x) &= 0x80000000;
        F64_LO(x) = 0;
        return x;
    } else {
        i = 0xFFFFFFFF >> (j0 - 20);
        if ((i1 & i) == 0) {
            *iptr = x;
            F64_HI(x) &= 0x80000000;
            F64_LO(x) = 0;
            return x;
        }
        F64_HI(*iptr) = i0;
        F64_LO(*iptr) = i1 & ~i;
        return x - *iptr;
    }
}
