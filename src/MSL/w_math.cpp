/*
 * MSL/w_math.cpp - the libm wrapper entries: four 4-byte tail thunks (`acos`, `asin`, `atan2`, `fmod`), two more
 *    (`log10`, `pow`), the software `__ieee754_sqrt` with the EDOM errno path, a bare return and the 4-byte `sqrt`
 *    thunk.
 *
 * RANGE. .text 0x80467F64..0x804681CC (9 functions in the map, 0x268 B); .sdata2 0x8079CF38..0x8079CF40.
 * FLAGS. the `OS` lib's `cflags_os` (these functions scored as written with it); compiled as C++ with `extern "C"`
 *    linkage; the body is the bit-by-bit square root.
 * NAMES. file name GUESS (MSL `w_*` wrappers merged); `acos`, `asin`, `atan2`, `fmod` name the unnamed thunks by their
 *    target, `__ieee754_sqrt` names the 0x248-byte body (GUESS: the fdlibm bit-by-bit root) and `__libm_unused_stub`
 *    the bare return (GUESS).
 * EVIDENCE. each thunk is a single `b` to the matching fdlibm unit (e_acos, e_asin, e_atan2, e_fmod, e_log10,
 *    e_pow); the 0x248-byte function reads `errno`, `__float_nan` and `.sdata2` 0x8079CF38 (`1.0`).
 * RESIDUALS. COARSE: the thunks may be several files; no data separates them.
 * SHAPES. the root is the shift-and-subtract loop over the two mantissa words; the rounding probe uses 1.0 and 1e-300
 *    literals the compiler folds.
 */
#pragma fp_contract off
#include "MSL/w_math.h"
#include "MSL/e_acos.h"
#include "MSL/e_asin.h"
#include "MSL/e_atan2.h"
#include "MSL/e_fmod.h"
#include "MSL/e_log10.h"
#include "MSL/e_pow.h"
#include "MSL/fdlibm.h"
#include "MSL_C/errno.h"
#include "MSL_C/float.h"

extern "C" f64 acos(f64 x)
{
    return __ieee754_acos(x);
}

extern "C" f64 asin(f64 x)
{
    return __ieee754_asin(x);
}

extern "C" f64 atan2(f64 y, f64 x)
{
    return __ieee754_atan2(y, x);
}

extern "C" f64 fmod(f64 x, f64 y)
{
    return __ieee754_fmod(x, y);
}

extern "C" f64 log10(f64 x)
{
    return __ieee754_log10(x);
}

extern "C" f64 pow(f64 x, f64 y)
{
    return __ieee754_pow(x, y);
}

extern "C" f64 __ieee754_sqrt(f64 x)
{
    f64 z;
    s32 sign = (s32)0x80000000;
    u32 r;
    u32 t1;
    u32 s1;
    u32 ix1;
    u32 q1;
    s32 ix0;
    s32 s0;
    s32 q;
    s32 m;
    s32 t;
    s32 i;

    ix0 = F64_HI(x);
    ix1 = F64_LO(x);
    if ((ix0 & 0x7FF00000) == 0x7FF00000) {
        errno = 33;
        return x * x + x;
    }
    if (ix0 <= 0) {
        if (((ix0 & (~sign)) | ix1) == 0) {
            return x;
        } else if (ix0 < 0) {
            errno = 33;
            return *(f32*)__float_nan;
        }
    }
    m = (ix0 >> 20);
    if (m == 0) {
        while (ix0 == 0) {
            m -= 21;
            ix0 |= (ix1 >> 11);
            ix1 <<= 21;
        }
        for (i = 0; (ix0 & 0x00100000) == 0; i++) {
            ix0 <<= 1;
        }
        m -= i - 1;
        ix0 |= (ix1 >> (32 - i));
        ix1 <<= i;
    }
    m -= 1023;
    ix0 = (ix0 & 0x000FFFFF) | 0x00100000;
    if (m & 1) {
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
    }
    m >>= 1;
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;
    r = 0x00200000;
    while (r != 0) {
        t = s0 + r;
        if (t <= ix0) {
            s0 = t + r;
            ix0 -= t;
            q += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }
    r = sign;
    while (r != 0) {
        t1 = s1 + r;
        t = s0;
        if ((t < ix0) || ((t == ix0) && (t1 <= ix1))) {
            s1 = t1 + r;
            if (((t1 & sign) == sign) && (s1 & sign) == 0) {
                s0 += 1;
            }
            ix0 -= t;
            if (ix1 < t1) {
                ix0 -= 1;
            }
            ix1 -= t1;
            q1 += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }
    if ((ix0 | ix1) != 0) {
        z = 1.0 - 1.0e-300;
        if (z >= 1.0) {
            z = 1.0 + 1.0e-300;
            if (q1 == (u32)0xFFFFFFFF) {
                q1 = 0;
                q += 1;
            } else if (z > 1.0) {
                if (q1 == (u32)0xFFFFFFFE) {
                    q += 1;
                }
                q1 += 2;
            } else {
                q1 += (q1 & 1);
            }
        }
    }
    ix0 = (q >> 1) + 0x3FE00000;
    ix1 = q1 >> 1;
    if ((q & 1) == 1) {
        ix1 |= sign;
    }
    ix0 += (m << 20);
    F64_HI(z) = ix0;
    F64_LO(z) = ix1;
    return z;
}

extern "C" void __libm_unused_stub(void)
{
}

extern "C" f64 sqrt(f64 x)
{
    return __ieee754_sqrt(x);
}
