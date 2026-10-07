/*
 * MSL/e_log.cpp - the MSL `__ieee754_log` (fdlibm shape): the natural logarithm with `errno` set for negative input.
 *
 * RANGE. .text 0x80464B38..0x80464DEC (1 function in the map, 0x2B4 B); .sbss 0x80794E28..0x80794E30 (the
 *    static zero); .sdata2 0x8079CB98..0x8079CC18 (-2^54, 2^54, 1.0, ln2 high/low, 0.5, 1/3, 2.0, the seven Lg
 *    coefficients and the int-to-double bias).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_log` is the map's name; `e_log_zero` (the static zero the divisions use) is a GUESS.
 * EVIDENCE. the `.sdata2` pool is in order of use (the t1 coefficients Lg2, Lg4, Lg6 precede Lg1, Lg3, Lg5, Lg7); the
 *    negative-input path stores 33 (EDOM) into `errno` before the NaN division.
 * RESIDUALS. one `fadd` pair in the last two returns takes its operands in the other order (register allocation, 2 words).
 * SHAPES. the scalar constants are literals so the compiler pools them itself; `zero` is a zero-initialised static
 *    so the divisions by zero stay run-time.
 */
#pragma fp_contract off
#include "MSL/e_log.h"
#include "MSL/fdlibm.h"
#include "MSL_C/errno.h"

static f64 e_log_zero = 0.0;

extern "C" f64 __ieee754_log(f64 x)
{
    f64 hfsq;
    f64 f;
    f64 s;
    f64 z;
    f64 R;
    f64 w;
    f64 t1;
    f64 t2;
    f64 dk;
    s32 k;
    s32 hx;
    s32 i;
    s32 j;
    u32 lx;

    hx = F64_HI(x);
    lx = F64_LO(x);
    k = 0;
    if (hx < 0x00100000) {
        if (((hx & 0x7FFFFFFF) | lx) == 0) {
            return -18014398509481984.0 / e_log_zero;
        }
        if (hx < 0) {
            errno = 33;
            return (x - x) / e_log_zero;
        }
        k -= 54;
        x *= 18014398509481984.0;
        hx = F64_HI(x);
    }
    if (hx >= 0x7FF00000) {
        return x + x;
    }
    k += (hx >> 20) - 1023;
    hx &= 0x000FFFFF;
    i = (hx + 0x95F64) & 0x100000;
    F64_HI(x) = hx | (i ^ 0x3FF00000);
    k += (i >> 20);
    f = x - 1.0;
    if ((0x000FFFFF & (2 + hx)) < 3) {
        if (f == e_log_zero) {
            if (k == 0) {
                return e_log_zero;
            }
            dk = (f64)k;
            return dk * 6.93147180369123816490e-01 + dk * 1.90821492927058770002e-10;
        }
        R = f * f * (0.5 - 0.33333333333333333 * f);
        if (k == 0) {
            return f - R;
        }
        dk = (f64)k;
        return dk * 6.93147180369123816490e-01 - ((R - dk * 1.90821492927058770002e-10) - f);
    }
    s = f / (2.0 + f);
    dk = (f64)k;
    z = s * s;
    i = hx - 0x6147A;
    w = z * z;
    j = 0x6B851 - hx;
    t1 = w * (3.999999999940941908e-01 + w * (2.222219843214978396e-01 + w * 1.531383769920937332e-01));
    t2 = z * (6.666666666666735130e-01 + w * (2.857142874366239149e-01 + w * (1.818357216161805012e-01 + w * 1.479819860511658591e-01)));
    i |= j;
    R = t2 + t1;
    if (i > 0) {
        hfsq = 0.5 * f * f;
        if (k == 0) {
            return f - (hfsq - s * (hfsq + R));
        }
        return dk * 6.93147180369123816490e-01 - ((hfsq - (s * (hfsq + R) + dk * 1.90821492927058770002e-10)) - f);
    }
    if (k == 0) {
        return f - s * (f - R);
    }
    return dk * 6.93147180369123816490e-01 - ((s * (f - R) - dk * 1.90821492927058770002e-10) - f);
}
