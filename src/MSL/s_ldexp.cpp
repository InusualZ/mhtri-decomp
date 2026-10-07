/*
 * MSL/s_ldexp.cpp - the MSL `ldexp` (fdlibm `scalbn` shape): scales a double by a power of two.
 *
 * RANGE. .text 0x80467BB8..0x80467D24 (1 function in the map, 0x16C B); .sdata2 0x8079CF00..0x8079CF28 (the
 *    pooled 0.0, 2^54, 1e-300, 1e300 and 2^-54).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `ldexp` is the dump's name.
 * EVIDENCE. the five pooled doubles appear in the order of use; the finite test is `__fpclassifyd`.
 * RESIDUALS. none measured.
 * SHAPES. the constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/s_ldexp.h"
#include "MSL/s_copysign.h"
#include "MSL_C/__fpclassifyd.h"
#include "MSL/fdlibm.h"


extern "C" f64 ldexp(f64 value, s32 exp)
{
    s32 k;
    s32 hx;
    u32 lx;

    if (__fpclassifyd(value) <= 2 || value == 0.0) {
        return value;
    }
    hx = F64_HI(value);
    lx = F64_LO(value);
    k = (hx & 0x7FF00000) >> 20;
    if (k == 0) {
        if ((lx | (hx & 0x7FFFFFFF)) == 0) {
            return value;
        }
        value *= 18014398509481984.0;
        hx = F64_HI(value);
        k = ((hx & 0x7FF00000) >> 20) - 54;
        if (exp < -50000) {
            return 1.0e-300 * value;
        }
    }
    if (k == 0x7FF) {
        return value + value;
    }
    k = k + exp;
    if (k > 0x7FE) {
        return 1.0e300 * copysign(1.0e300, value);
    }
    if (k > 0) {
        F64_HI(value) = (hx & 0x800FFFFF) | (k << 20);
        return value;
    }
    if (k <= -54) {
        if (exp > 50000) {
            return 1.0e300 * copysign(1.0e300, value);
        }
        return 1.0e-300 * copysign(1.0e-300, value);
    }
    k += 54;
    F64_HI(value) = (hx & 0x800FFFFF) | (k << 20);
    return 5.551115123125783e-17 * value;
}
