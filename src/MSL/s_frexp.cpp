/*
 * MSL/s_frexp.cpp - the MSL `frexp` (fdlibm shape): the fraction and binary exponent of a double.
 *
 * RANGE. .text 0x80467B30..0x80467BB8 (1 function in the map, 0x88 B); .sdata2 0x8079CEF8..0x8079CF00 (the
 *    pooled 2^54).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `frexp` is the dump's name.
 * EVIDENCE. the one pooled double is 2^54 (the subnormal rescale).
 * RESIDUALS. none measured.
 * SHAPES. the constant is a literal so the compiler pools it itself.
 */
#pragma fp_contract off
#include "MSL/s_frexp.h"
#include "MSL/fdlibm.h"

extern "C" f64 frexp(f64 x, s32* eptr)
{
    s32 hx;
    s32 ix;
    u32 lx;

    hx = F64_HI(x);
    ix = 0x7FFFFFFF & hx;
    lx = F64_LO(x);
    *eptr = 0;
    if (ix >= 0x7FF00000 || ((ix | lx) == 0)) {
        return x;
    }
    if (ix < 0x00100000) {
        x *= 18014398509481984.0;
        hx = F64_HI(x);
        ix = hx & 0x7FFFFFFF;
        *eptr = -54;
    }
    *eptr += (ix >> 20) - 1022;
    hx = (hx & 0x800FFFFF) | 0x3FE00000;
    F64_HI(x) = hx;
    return x;
}
