/*
 * MSL/k_rem_pio2.cpp - the MSL `__kernel_rem_pio2` (fdlibm shape): reduces a 24-bit-chunk vector modulo pi/2.
 *
 * RANGE. .text 0x80465BA8..0x80467260 (1 function in the map, 0x16B8 B); .rodata 0x80573358..0x805733A8 (the
 *    `init_jk` table and the eight `PIo2` chunks); .sdata2 0x8079CDF8..0x8079CE38 (0.0, 2^-24, 2^24, 8.0, 0.125,
 *    0.5, 1.0 and the int-to-double bias).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__kernel_rem_pio2` is the map's name; `init_jk` and `PIo2` are the algorithm's own table names.
 * EVIDENCE. `.rodata` 0x80573358 holds the words 2, 3, 4, 6 and 0x80573368 the doubles 1.5707962512969971 ...
 * RESIDUALS. none known (the `recompute` jump is a `for (;;)` with `continue`).
 * SHAPES. the scalar constants are literals so the compiler pools them itself; the two tables are static arrays.
 */
#pragma fp_contract off
#include "MSL/k_rem_pio2.h"
#include "MSL/s_floor.h"
#include "MSL/scalbn.h"

static const s32 init_jk[4] = { 2, 3, 4, 6 };

static const f64 PIo2[8] = {
    1.57079625129699707031e+00, 7.54978941586159635335e-08, 5.39030252995776476554e-15, 3.28200341580791294123e-22,
    1.27065575308067607349e-29, 1.22933308981111328932e-36, 2.73370053816464559624e-44, 2.16741683877804819444e-51,
};

extern "C" s32 __kernel_rem_pio2(f64* x, f64* y, s32 e0, s32 nx, s32 prec, const s32* ipio2)
{
    s32 jz;
    s32 jx;
    s32 jv;
    s32 jp;
    s32 jk;
    s32 carry;
    s32 n;
    s32 iq[20];
    s32 i;
    s32 j;
    s32 k;
    s32 m;
    s32 q0;
    s32 ih;
    f64 z;
    f64 fw;
    f64 f[20];
    f64 fq[20];
    f64 q[20];

    jk = init_jk[prec];
    jp = jk;

    jx = nx - 1;
    jv = (e0 - 3) / 24;
    if (jv < 0) {
        jv = 0;
    }
    q0 = e0 - 24 * (jv + 1);

    j = jv - jx;
    m = jx + jk;
    for (i = 0; i <= m; i++, j++) {
        f[i] = (j < 0) ? 0.0 : (f64)ipio2[j];
    }

    for (i = 0; i <= jk; i++) {
        for (j = 0, fw = 0.0; j <= jx; j++) {
            fw += x[j] * f[jx + i - j];
        }
        q[i] = fw;
    }

    jz = jk;
    for (;;) {
        for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--) {
            fw = (f64)((s32)(5.96046447753906250000e-08 * z));
            iq[i] = (s32)(z - 16777216.0 * fw);
            z = q[j - 1] + fw;
        }

        z = scalbn(z, q0);
        z -= 8.0 * floor(z * 0.125);
        n = (s32)z;
        z -= (f64)n;
        ih = 0;
        if (q0 > 0) {
            i = iq[jz - 1] >> (24 - q0);
            n += i;
            iq[jz - 1] -= i << (24 - q0);
            ih = iq[jz - 1] >> (23 - q0);
        } else if (q0 == 0) {
            ih = iq[jz - 1] >> 23;
        } else if (z >= 0.5) {
            ih = 2;
        }

        if (ih > 0) {
            n += 1;
            carry = 0;
            for (i = 0; i < jz; i++) {
                j = iq[i];
                if (carry == 0) {
                    if (j != 0) {
                        carry = 1;
                        iq[i] = 0x1000000 - j;
                    }
                } else {
                    iq[i] = 0xFFFFFF - j;
                }
            }
            if (q0 > 0) {
                switch (q0) {
                case 1:
                    iq[jz - 1] &= 0x7FFFFF;
                    break;
                case 2:
                    iq[jz - 1] &= 0x3FFFFF;
                    break;
                }
            }
            if (ih == 2) {
                z = 1.0 - z;
                if (carry != 0) {
                    z -= scalbn(1.0, q0);
                }
            }
        }

        if (z == 0.0) {
            j = 0;
            for (i = jz - 1; i >= jk; i--) {
                j |= iq[i];
            }
            if (j == 0) {
                for (k = 1; iq[jk - k] == 0; k++) {
                }
                for (i = jz + 1; i <= jz + k; i++) {
                    f[jx + i] = (f64)ipio2[jv + i];
                    for (j = 0, fw = 0.0; j <= jx; j++) {
                        fw += x[j] * f[jx + i - j];
                    }
                    q[i] = fw;
                }
                jz += k;
                continue;
            }
        }
        break;
    }

    if (z == 0.0) {
        jz -= 1;
        q0 -= 24;
        while (iq[jz] == 0) {
            jz--;
            q0 -= 24;
        }
    } else {
        z = scalbn(z, -q0);
        if (z >= 16777216.0) {
            fw = (f64)((s32)(5.96046447753906250000e-08 * z));
            iq[jz] = (s32)(z - 16777216.0 * fw);
            jz += 1;
            q0 += 24;
            iq[jz] = (s32)fw;
        } else {
            iq[jz] = (s32)z;
        }
    }

    fw = scalbn(1.0, q0);
    for (i = jz; i >= 0; i--) {
        q[i] = fw * (f64)iq[i];
        fw *= 5.96046447753906250000e-08;
    }

    for (i = jz; i >= 0; i--) {
        for (fw = 0.0, k = 0; k <= jp && k <= jz - i; k++) {
            fw += PIo2[k] * q[i + k];
        }
        fq[jz - i] = fw;
    }

    switch (prec) {
    case 0:
        fw = 0.0;
        for (i = jz; i >= 0; i--) {
            fw += fq[i];
        }
        y[0] = (ih == 0) ? fw : -fw;
        break;
    case 1:
    case 2:
        fw = 0.0;
        for (i = jz; i >= 0; i--) {
            fw += fq[i];
        }
        y[0] = (ih == 0) ? fw : -fw;
        fw = fq[0] - fw;
        for (i = 1; i <= jz; i++) {
            fw += fq[i];
        }
        y[1] = (ih == 0) ? fw : -fw;
        break;
    case 3:
        for (i = jz; i > 0; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (i = jz; i > 1; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (fw = 0.0, i = jz; i >= 2; i--) {
            fw += fq[i];
        }
        if (ih == 0) {
            y[0] = fq[0];
            y[1] = fq[1];
            y[2] = fw;
        } else {
            y[0] = -fq[0];
            y[1] = -fq[1];
            y[2] = -fw;
        }
        break;
    }
    return n & 7;
}
