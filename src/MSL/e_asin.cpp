/*
 * MSL/e_asin.cpp - the MSL `__ieee754_asin` (fdlibm shape): the arc sine with `errno` set outside [-1, 1].
 *
 * RANGE. .text 0x804642C8..0x80464560 (1 function in the map, 0x298 B); .sdata2 0x8079CAB8..0x8079CB40 (pio2_hi,
 *    pio2_lo, 1e300, 1.0, the pS0..pS5 and qS1..qS4 coefficients, 0.5, 2.0 and pi/4).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_asin` is the map's name (the unit's registered one). `__ieee754_asin` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 * EVIDENCE. the domain-error path stores 33 (EDOM) into `errno` and returns `__float_nan`; `sqrt` is the 4-byte thunk
 *    at 0x804681C8.
 * RESIDUALS. none measured.
 * SHAPES. the constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/e_asin.h"
#include "MSL/sqrt.h"
#include "MSL/fdlibm.h"
#include "MSL_C/errno.h"
#include "MSL_C/float.h"

extern "C" f64 __ieee754_asin(f64 x)
{
    f64 t;
    f64 w;
    f64 p;
    f64 q;
    f64 c;
    f64 r;
    f64 s;
    s32 hx;
    s32 ix;

    hx = F64_HI(x);
    ix = hx & 0x7FFFFFFF;
    if (ix >= 0x3FF00000) {
        if (((ix - 0x3FF00000) | F64_LO(x)) == 0) {
            return x * 1.57079632679489655800e+00 + x * 6.12323399573676603587e-17;
        }
        errno = 33;
        return *(f32*)__float_nan;
    } else if (ix < 0x3FE00000) {
        if (ix < 0x3E400000) {
            if (1.0e300 + x > 1.0) {
                return x;
            }
        } else {
            t = x * x;
        }
        p = t * (1.66666666666666657415e-01 + t * (-3.25565818622400915405e-01 + t * (2.01212532134862925881e-01 + t * (-4.00555345006794114027e-02 + t * (7.91534994289814532176e-04 + t * 3.47933107596021167570e-05)))));
        q = 1.0 + t * (-2.40339491173441421878e+00 + t * (2.02094576023350569471e+00 + t * (-6.88283971605453293030e-01 + t * 7.70381505559019352791e-02)));
        w = p / q;
        return x + x * w;
    }
    w = 1.0 - __fabs(x);
    t = w * 0.5;
    p = t * (1.66666666666666657415e-01 + t * (-3.25565818622400915405e-01 + t * (2.01212532134862925881e-01 + t * (-4.00555345006794114027e-02 + t * (7.91534994289814532176e-04 + t * 3.47933107596021167570e-05)))));
    q = 1.0 + t * (-2.40339491173441421878e+00 + t * (2.02094576023350569471e+00 + t * (-6.88283971605453293030e-01 + t * 7.70381505559019352791e-02)));
    s = sqrt(t);
    if (ix >= 0x3FEF3333) {
        w = p / q;
        t = 1.57079632679489655800e+00 - (2.0 * (s + s * w) - 6.12323399573676603587e-17);
    } else {
        w = s;
        F64_LO(w) = 0;
        c = (t - w * w) / (s + w);
        r = p / q;
        p = 2.0 * s * r - (6.12323399573676603587e-17 - 2.0 * c);
        q = 7.85398163397448278999e-01 - 2.0 * w;
        t = 7.85398163397448278999e-01 - (p - q);
    }
    if (hx > 0) {
        return t;
    }
    return -t;
}
