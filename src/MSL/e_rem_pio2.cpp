/*
 * MSL/e_rem_pio2.cpp - the MSL `__ieee754_rem_pio2` (fdlibm shape): reduces a double modulo pi/2.
 *
 * RANGE. .text 0x80465714..0x80465A98 (1 function in the map, 0x384 B); .rodata 0x805731D0..0x80573358 (the
 *    66-word 2/pi table and the 32-word `npio2_hw` table); .sdata2 0x8079CD58..0x8079CDB0 (0.0, pio2_1/1t/2/2t/3/3t,
 *    0.5, 2/pi, 2^24 and the int-to-double bias).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_rem_pio2` is the dump's name; `two_over_pi` and `npio2_hw` are the algorithm's own table names.
 * EVIDENCE. the first `.rodata` words are 0xA2F983, 0x6E4E44, ... (the bits of 2/pi in 24-bit chunks); the second table
 *    is the high words of n*pi/2.
 * RESIDUALS. `npio2_hw[n - 1]` is addressed with `slwi`+`add`+`lwz -4` where the target uses `subi`+`slwi`+`lwzx` (2 words).
 * SHAPES. the scalar constants are literals so the compiler pools them itself; the two tables are static arrays.
 */
#pragma fp_contract off
#include "MSL/e_rem_pio2.h"
#include "MSL/k_rem_pio2.h"
#include "MSL/fdlibm.h"

static const s32 two_over_pi[66] = {
    0x00A2F983, 0x006E4E44, 0x001529FC, 0x002757D1, 0x00F534DD, 0x00C0DB62, 0x0095993C, 0x00439041,
    0x00FE5163, 0x00ABDEBB, 0x00C561B7, 0x00246E3A, 0x00424DD2, 0x00E00649, 0x002EEA09, 0x00D1921C,
    0x00FE1DEB, 0x001CB129, 0x00A73EE8, 0x008235F5, 0x002EBB44, 0x0084E99C, 0x007026B4, 0x005F7E41,
    0x003991D6, 0x00398353, 0x0039F49C, 0x00845F8B, 0x00BDF928, 0x003B1FF8, 0x0097FFDE, 0x0005980F,
    0x00EF2F11, 0x008B5A0A, 0x006D1F6D, 0x00367ECF, 0x0027CB09, 0x00B74F46, 0x003F669E, 0x005FEA2D,
    0x007527BA, 0x00C7EBE5, 0x00F17B3D, 0x000739F7, 0x008A5292, 0x00EA6BFB, 0x005FB11F, 0x008D5D08,
    0x00560330, 0x0046FC7B, 0x006BABF0, 0x00CFBC20, 0x009AF436, 0x001DA9E3, 0x0091615E, 0x00E61B08,
    0x00659985, 0x005F14A0, 0x0068408D, 0x00FFD880, 0x004D7327, 0x00310606, 0x001556CA, 0x0073A8C9,
    0x0060E27B, 0x00C08C6B,
};

static const s32 npio2_hw[32] = {
    0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C, 0x4025FDBB, 0x402921FB,
    0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C, 0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB,
    0x403AB41B, 0x403C463A, 0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C,
    0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB, 0x404858EB, 0x404921FB,
};

extern "C" s32 __ieee754_rem_pio2(f64 x, f64* y)
{
    f64 z;
    f64 w;
    f64 t;
    f64 r;
    f64 fn;
    f64 tx[3];
    s32 e0;
    s32 i;
    s32 j;
    s32 nx;
    s32 n;
    s32 ix;
    s32 hx;

    hx = F64_HI(x);
    ix = hx & 0x7FFFFFFF;
    if (ix <= 0x3FE921FB) {
        y[0] = x;
        y[1] = 0.0;
        return 0;
    }
    if (ix < 0x4002D97C) {
        if (hx > 0) {
            z = x - 1.57079632673412561417e+00;
            if (ix != 0x3FF921FB) {
                y[0] = z - 6.07710050650619224932e-11;
                y[1] = (z - y[0]) - 6.07710050650619224932e-11;
            } else {
                z -= 6.07710050630396597660e-11;
                y[0] = z - 2.02226624879595063154e-21;
                y[1] = (z - y[0]) - 2.02226624879595063154e-21;
            }
            return 1;
        } else {
            z = x + 1.57079632673412561417e+00;
            if (ix != 0x3FF921FB) {
                y[0] = z + 6.07710050650619224932e-11;
                y[1] = (z - y[0]) + 6.07710050650619224932e-11;
            } else {
                z += 6.07710050630396597660e-11;
                y[0] = z + 2.02226624879595063154e-21;
                y[1] = (z - y[0]) + 2.02226624879595063154e-21;
            }
            return -1;
        }
    }
    if (ix <= 0x413921FB) {
        t = __fabs(x);
        n = (s32)(t * 6.36619772367581382433e-01 + 0.5);
        fn = (f64)n;
        r = t - fn * 1.57079632673412561417e+00;
        w = fn * 6.07710050650619224932e-11;
        if (n < 32 && ix != npio2_hw[n - 1]) {
            y[0] = r - w;
        } else {
            j = ix >> 20;
            y[0] = r - w;
            i = j - ((F64_HI(y[0]) >> 20) & 0x7FF);
            if (i > 16) {
                t = r;
                w = fn * 6.07710050630396597660e-11;
                r = t - w;
                w = fn * 2.02226624879595063154e-21 - ((t - r) - w);
                y[0] = r - w;
                i = j - ((F64_HI(y[0]) >> 20) & 0x7FF);
                if (i > 49) {
                    t = r;
                    w = fn * 2.02226624871116645580e-21;
                    r = t - w;
                    w = fn * 8.47842766036889956997e-32 - ((t - r) - w);
                    y[0] = r - w;
                }
            }
        }
        y[1] = (r - y[0]) - w;
        if (hx < 0) {
            y[0] = -y[0];
            y[1] = -y[1];
            return -n;
        }
        return n;
    }
    if (ix >= 0x7FF00000) {
        y[0] = y[1] = x - x;
        return 0;
    }
    F64_LO(z) = F64_LO(x);
    e0 = (ix >> 20) - 1046;
    F64_HI(z) = ix - (e0 << 20);
    for (i = 0; i < 2; i++) {
        tx[i] = (f64)((s32)(z));
        z = (z - tx[i]) * 16777216.0;
    }
    tx[2] = z;
    nx = 3;
    while (tx[nx - 1] == 0.0) {
        nx--;
    }
    n = __kernel_rem_pio2(tx, y, e0, nx, 2, two_over_pi);
    if (hx < 0) {
        y[0] = -y[0];
        y[1] = -y[1];
        return -n;
    }
    return n;
}
