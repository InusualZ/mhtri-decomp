/*
 * MSL/e_acos.cpp - the MSL `__ieee754_acos` (fdlibm shape): the arc cosine with `errno` set outside [-1, 1].
 *
 * RANGE. .text 0x80463FFC..0x804642C8 (1 function in the map, 0x2CC B); .sdata2 0x8079CA30..0x8079CAB8 (the
 *    pooled constants: 0.0, pi, pi/2, the pS0..pS5 and qS1..qS4 coefficients, 1.0, pio2_lo, 0.5, 2.0).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_acos` is the map's name (the unit's registered one). `__ieee754_acos` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 * EVIDENCE. the domain-error path stores 33 (EDOM) into `errno` and returns `__float_nan`; the two `sqrt` calls go
 *    through the 4-byte thunk at 0x804681C8.
 * RESIDUALS. none measured.
 * SHAPES. the constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/e_acos.h"
#include "MSL/sqrt.h"
#include "MSL/fdlibm.h"
#include "MSL_C/errno.h"
#include "MSL_C/float.h"

extern "C" f64 __ieee754_acos(f64 x)
{
    f64 z;
    f64 p;
    f64 q;
    f64 r;
    f64 w;
    f64 s;
    f64 c;
    f64 df;
    s32 hx;
    s32 ix;

    hx = F64_HI(x);
    ix = hx & 0x7FFFFFFF;
    if (ix >= 0x3FF00000) {
        if (((ix - 0x3FF00000) | F64_LO(x)) == 0) {
            if (hx > 0) {
                return 0.0;
            }
            return 3.14159265358979311600e+00;
        }
        errno = 33;
        return *(f32*)__float_nan;
    }
    if (ix < 0x3FE00000) {
        if (ix <= 0x3C600000) {
            return 1.57079632679489655800e+00 + 6.12323399573676603587e-17;
        }
        z = x * x;
        p = z * (1.66666666666666657415e-01 + z * (-3.25565818622400915405e-01 + z * (2.01212532134862925881e-01 + z * (-4.00555345006794114027e-02 + z * (7.91534994289814532176e-04 + z * 3.47933107596021167570e-05)))));
        q = 1.0 + z * (-2.40339491173441421878e+00 + z * (2.02094576023350569471e+00 + z * (-6.88283971605453293030e-01 + z * 7.70381505559019352791e-02)));
        r = p / q;
        return 1.57079632679489655800e+00 - (x - (6.12323399573676603587e-17 - x * r));
    } else if (hx < 0) {
        z = (1.0 + x) * 0.5;
        p = z * (1.66666666666666657415e-01 + z * (-3.25565818622400915405e-01 + z * (2.01212532134862925881e-01 + z * (-4.00555345006794114027e-02 + z * (7.91534994289814532176e-04 + z * 3.47933107596021167570e-05)))));
        q = 1.0 + z * (-2.40339491173441421878e+00 + z * (2.02094576023350569471e+00 + z * (-6.88283971605453293030e-01 + z * 7.70381505559019352791e-02)));
        s = sqrt(z);
        r = p / q;
        w = r * s - 6.12323399573676603587e-17;
        return 3.14159265358979311600e+00 - 2.0 * (s + w);
    } else {
        z = (1.0 - x) * 0.5;
        s = sqrt(z);
        df = s;
        F64_LO(df) = 0;
        c = (z - df * df) / (s + df);
        p = z * (1.66666666666666657415e-01 + z * (-3.25565818622400915405e-01 + z * (2.01212532134862925881e-01 + z * (-4.00555345006794114027e-02 + z * (7.91534994289814532176e-04 + z * 3.47933107596021167570e-05)))));
        q = 1.0 + z * (-2.40339491173441421878e+00 + z * (2.02094576023350569471e+00 + z * (-6.88283971605453293030e-01 + z * 7.70381505559019352791e-02)));
        r = p / q;
        w = r * s + c;
        return 2.0 * (df + w);
    }
}
