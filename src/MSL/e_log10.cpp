/*
 * MSL/e_log10.cpp - the MSL `__ieee754_log10` (fdlibm shape): the base-10 logarithm via `__ieee754_log`, with `errno`
 *    set for non-positive input.
 *
 * RANGE. .text 0x80464DEC..0x80464F00 (1 function in the map, 0x114 B); .sbss 0x80794E30..0x80794E38 (the static
 *    zero); .sdata2 0x8079CC18..0x8079CC48 (-2^54, 2^54, log10(2) low and high, 1/ln(10) and the int-to-double bias).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_log10` is the dump's name; `e_log10_zero` is a GUESS. `__ieee754_log10` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 * EVIDENCE. both non-positive paths store 33 (EDOM) into `errno`; the one call is `__ieee754_log`.
 * RESIDUALS. none measured.
 * SHAPES. the scalar constants are literals so the compiler pools them itself; `zero` is a zero-initialised static.
 */
#pragma fp_contract off
#include "MSL/e_log10.h"
#include "MSL/e_log.h"
#include "MSL/fdlibm.h"
#include "MSL_C/errno.h"

static f64 e_log10_zero = 0.0;

extern "C" f64 __ieee754_log10(f64 x)
{
    f64 y;
    f64 z;
    s32 i;
    s32 k;
    s32 hx;
    u32 lx;

    hx = F64_HI(x);
    lx = F64_LO(x);
    k = 0;
    if (hx < 0x00100000) {
        if (((hx & 0x7FFFFFFF) | lx) == 0) {
            errno = 33;
            return -18014398509481984.0 / e_log10_zero;
        }
        if (hx < 0) {
            errno = 33;
            return (x - x) / e_log10_zero;
        }
        k -= 54;
        x *= 18014398509481984.0;
        hx = F64_HI(x);
    }
    if (hx >= 0x7FF00000) {
        return x + x;
    }
    k += (hx >> 20) - 1023;
    i = ((u32)k & 0x80000000) >> 31;
    hx = (hx & 0x000FFFFF) | ((0x3FF - i) << 20);
    y = (f64)(k + i);
    F64_HI(x) = hx;
    z = y * 3.69423907715893078616e-13 + 4.34294481903251816668e-01 * __ieee754_log(x);
    return z + y * 3.01029995663611771306e-01;
}
