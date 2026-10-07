/*
 * MSL/k_tan.cpp - the MSL `__kernel_tan` (fdlibm shape): the tangent of `x` on [-pi/4, pi/4] with a tail `y`.
 *
 * RANGE. .text 0x80467320..0x80467570 (1 function in the map, 0x250 B); .rodata 0x805733A8..0x80573410 (the 13
 *    polynomial coefficients); .sdata2 0x8079CE70..0x8079CEA8 (1.0, -1.0, pi/4 and its low part, 0.0, 2.0 and the
 *    int-to-double bias).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__kernel_tan` is the map's name; the `.rodata` table `tan_coefficients` is a GUESS.
 * EVIDENCE. the coefficients start at 1/3 and follow the tangent series; the `.sdata2` doubles appear in order of use.
 * RESIDUALS. none measured.
 * SHAPES. the scalar constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/k_tan.h"
#include "MSL/fdlibm.h"

static const f64 tan_coefficients[13] = {
    3.33333333333334091986e-01, 1.33333333333201242699e-01, 5.39682539762260521377e-02, 2.18694882948595424599e-02,
    8.86323982359930005737e-03, 3.59207910759131235356e-03, 1.45620945432529025516e-03, 5.88041240820264096874e-04,
    2.46463134818469906812e-04, 7.81794442939557092300e-05, 7.14072491382608190305e-05, -1.85586374855275456654e-05,
    2.59073051863633712884e-05,
};

extern "C" f64 __kernel_tan(f64 x, f64 y, int iy)
{
    f64 z;
    f64 r;
    f64 v;
    f64 w;
    f64 s;
    s32 ix;
    s32 hx;

    hx = F64_HI(x);
    ix = hx & 0x7FFFFFFF;
    if (ix < 0x3E300000) {
        if ((int)x == 0) {
            if (((ix | F64_LO(x)) | (iy + 1)) == 0) {
                return 1.0 / __fabs(x);
            }
            if (iy == 1) {
                return x;
            }
            return -1.0 / x;
        }
    }
    if (ix >= 0x3FE59428) {
        if (hx < 0) {
            x = -x;
            y = -y;
        }
        z = 7.853981633974483e-01 - x;
        w = 3.061616997868383e-17 - y;
        x = z + w;
        y = 0.0;
    }
    z = x * x;
    w = z * z;
    r = tan_coefficients[1] + w * (tan_coefficients[3] + w * (tan_coefficients[5] + w * (tan_coefficients[7] + w * (tan_coefficients[9] + w * tan_coefficients[11]))));
    v = z * (tan_coefficients[2] + w * (tan_coefficients[4] + w * (tan_coefficients[6] + w * (tan_coefficients[8] + w * (tan_coefficients[10] + w * tan_coefficients[12])))));
    s = z * x;
    r = y + z * (s * (r + v) + y);
    r += tan_coefficients[0] * s;
    w = x + r;
    if (ix >= 0x3FE59428) {
        v = (f64)iy;
        return (f64)(1 - ((hx >> 30) & 2)) * (v - 2.0 * (x - (w * w / (w + v) - r)));
    }
    if (iy == 1) {
        return w;
    }
    {
        f64 a;
        f64 t;

        z = w;
        F64_LO(z) = 0;
        v = r - (z - x);
        t = a = -1.0 / w;
        F64_LO(t) = 0;
        s = 1.0 + t * z;
        return t + a * (s + t * v);
    }
}
