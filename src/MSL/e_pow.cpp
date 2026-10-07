/*
 * MSL/e_pow.cpp - the MSL `__ieee754_pow` (fdlibm shape): `x` raised to `y` through a split log2 and a polynomial exp2.
 *
 * RANGE. .text 0x80464F00..0x80465714 (1 function in the map, 0x814 B); .rodata 0x805731A0..0x805731D0 (the `bp`, `dp_h`
 *    and `dp_l` tables); .sdata2 0x8079CC48..0x8079CD58 (the scalar constants in order of use).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_pow` is the map's name; the table names are the algorithm's own.
 * EVIDENCE. the three 2-entry tables are {1.0, 1.5}, {0, log2(1.5) high} and {0, log2(1.5) low}; the pooled infinity is
 *    the folded `huge * huge`; the calls are `sqrt` (the thunk) and `scalbn`.
 * RESIDUALS. same 541 instructions as the target; the target keeps six doubles in f26..f31 across the body where ours uses seven
 *    (f25..f31), so the frame is 0x10 larger and the FP registers differ; the cause is not isolated.
 * SHAPES. the scalar constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/e_pow.h"
#include "MSL/sqrt.h"
#include "MSL/scalbn.h"
#include "MSL/fdlibm.h"

static const f64 bp[2] = {1.0, 1.5};
static const f64 dp_h[2] = {0.0, 5.84962487220764160156e-01};
static const f64 dp_l[2] = {0.0, 1.35003920212974897128e-08};

extern "C" f64 __ieee754_pow(f64 x, f64 y)
{
    f64 z;
    f64 ax;
    f64 z_h;
    f64 z_l;
    f64 p_h;
    f64 p_l;
    f64 y1;
    f64 t1;
    f64 t2;
    f64 r;
    f64 s;
    f64 t;
    f64 u;
    f64 v;
    f64 w;
    s32 i;
    s32 j;
    s32 k;
    s32 yisint;
    s32 n;
    s32 hx;
    s32 hy;
    s32 ix;
    s32 iy;
    u32 lx;
    u32 ly;

    hx = F64_HI(x);
    lx = F64_LO(x);
    hy = F64_HI(y);
    ly = F64_LO(y);
    ix = hx & 0x7FFFFFFF;
    iy = hy & 0x7FFFFFFF;
    if ((iy | ly) == 0) {
        return 1.0;
    }
    if (ix > 0x7FF00000 || ((ix == 0x7FF00000) && (lx != 0)) || iy > 0x7FF00000 || ((iy == 0x7FF00000) && (ly != 0))) {
        return x + y;
    }
    yisint = 0;
    if (hx < 0) {
        if (iy >= 0x43400000) {
            yisint = 2;
        } else if (iy >= 0x3FF00000) {
            k = (iy >> 20) - 0x3FF;
            if (k > 20) {
                j = ly >> (52 - k);
                if ((u32)(j << (52 - k)) == ly) {
                    yisint = 2 - (j & 1);
                }
            } else if (ly == 0) {
                j = iy >> (20 - k);
                if ((j << (20 - k)) == iy) {
                    yisint = 2 - (j & 1);
                }
            }
        }
    }
    if (ly == 0) {
        if (iy == 0x7FF00000) {
            if (((ix - 0x3FF00000) | lx) == 0) {
                return y - y;
            } else if (ix >= 0x3FF00000) {
                return (hy >= 0) ? y : 0.0;
            } else {
                return (hy < 0) ? -y : 0.0;
            }
        }
        if (iy == 0x3FF00000) {
            if (hy < 0) {
                return 1.0 / x;
            }
            return x;
        }
        if (hy == 0x40000000) {
            return x * x;
        }
        if (hy == 0x3FE00000) {
            if (hx >= 0) {
                return sqrt(x);
            }
        }
    }
    ax = __fabs(x);
    if (lx == 0) {
        if (ix == 0x7FF00000 || ix == 0 || ix == 0x3FF00000) {
            z = ax;
            if (hy < 0) {
                z = 1.0 / z;
            }
            if (hx < 0) {
                if (((ix - 0x3FF00000) | yisint) == 0) {
                    z = (z - z) / (z - z);
                } else if (yisint == 1) {
                    z = -z;
                }
            }
            return z;
        }
    }
    n = ((u32)hx >> 31) - 1;
    if ((n | yisint) == 0) {
        return (x - x) / (x - x);
    }
    s = 1.0;
    if ((n | (yisint - 1)) == 0) {
        s = -1.0;
    }
    if (iy > 0x41E00000) {
        if (iy > 0x43F00000) {
            if (ix <= 0x3FEFFFFF) {
                return (hy < 0) ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
            }
            if (ix >= 0x3FF00000) {
                return (hy > 0) ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
            }
        }
        if (ix < 0x3FEFFFFF) {
            return (hy < 0) ? s * 1.0e300 * 1.0e300 : s * 1.0e-300 * 1.0e-300;
        }
        if (ix > 0x3FF00000) {
            return (hy > 0) ? s * 1.0e300 * 1.0e300 : s * 1.0e-300 * 1.0e-300;
        }
        t = ax - 1.0;
        w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25));
        u = 1.44269502162933349609e+00 * t;
        v = t * 1.92596299112661746887e-08 - w * 1.44269504088896338700e+00;
        t1 = u + v;
        F64_LO(t1) = 0;
        t2 = v - (t1 - u);
    } else {
        f64 ss;
        f64 s2;
        f64 s_h;
        f64 s_l;
        f64 t_h;
        f64 t_l;

        n = 0;
        if (ix < 0x00100000) {
            ax *= 9007199254740992.0;
            n -= 53;
            ix = F64_HI(ax);
        }
        n += ((ix) >> 20) - 0x3FF;
        j = ix & 0x000FFFFF;
        ix = j | 0x3FF00000;
        if (j <= 0x3988E) {
            k = 0;
        } else if (j < 0xBB67A) {
            k = 1;
        } else {
            k = 0;
            n += 1;
            ix -= 0x00100000;
        }
        F64_HI(ax) = ix;
        u = ax - bp[k];
        v = 1.0 / (ax + bp[k]);
        ss = u * v;
        s_h = ss;
        F64_LO(s_h) = 0;
        t_h = 0.0;
        F64_HI(t_h) = ((ix >> 1) | 0x20000000) + 0x00080000 + (k << 18);
        t_l = ax - (t_h - bp[k]);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);
        s2 = ss * ss;
        r = s2 * s2 * (5.99999999999994648725e-01 + s2 * (4.28571428578550184252e-01 + s2 * (3.33333329818377432918e-01 + s2 * (2.72728123808534006489e-01 + s2 * (2.30660745775561754067e-01 + s2 * 2.06975017800338417784e-01)))));
        r += s_l * (s_h + ss);
        s2 = s_h * s_h;
        t_h = 3.0 + s2 + r;
        F64_LO(t_h) = 0;
        t_l = r - ((t_h - 3.0) - s2);
        u = s_h * t_h;
        v = s_l * t_h + t_l * ss;
        p_h = u + v;
        F64_LO(p_h) = 0;
        p_l = v - (p_h - u);
        z_h = 9.61796700954437255859e-01 * p_h;
        z_l = -7.02846165095275826516e-09 * p_h + p_l * 9.61796693925975554329e-01 + dp_l[k];
        t = (f64)n;
        t1 = (((z_h + z_l) + dp_h[k]) + t);
        F64_LO(t1) = 0;
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }
    y1 = y;
    F64_LO(y1) = 0;
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    j = F64_HI(z);
    i = F64_LO(z);
    if (j >= 0x40900000) {
        if (((j - 0x40900000) | i) != 0) {
            return s * 1.0e300 * 1.0e300;
        } else {
            if (p_l + 8.0085662595372944372e-17 > z - p_h) {
                return s * 1.0e300 * 1.0e300;
            }
        }
    } else if ((j & 0x7FFFFFFF) >= 0x4090CC00) {
        if (((j - 0xC090CC00) | i) != 0) {
            return s * 1.0e-300 * 1.0e-300;
        } else {
            if (p_l <= z - p_h) {
                return s * 1.0e-300 * 1.0e-300;
            }
        }
    }
    i = j & 0x7FFFFFFF;
    k = (i >> 20) - 0x3FF;
    n = 0;
    if (i > 0x3FE00000) {
        n = j + (0x00100000 >> (k + 1));
        k = ((n & 0x7FFFFFFF) >> 20) - 0x3FF;
        t = 0.0;
        F64_HI(t) = (n & ~(0x000FFFFF >> k));
        n = ((n & 0x000FFFFF) | 0x00100000) >> (20 - k);
        if (j < 0) {
            n = -n;
        }
        p_h -= t;
    }
    t = p_l + p_h;
    F64_LO(t) = 0;
    u = t * 6.93147182464599609375e-01;
    v = (p_l - (t - p_h)) * 6.93147180559945286227e-01 + t * -1.90465429995776804525e-09;
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (1.66666666666666019037e-01 + t * (-2.77777777770155933842e-03 + t * (6.61375632143793436117e-05 + t * (-1.65339022054652515390e-06 + t * 4.13813679705723846039e-08))));
    r = (z * t1) / (t1 - 2.0) - (w + z * w);
    z = 1.0 - (r - z);
    j = F64_HI(z);
    j += (n << 20);
    if ((j >> 20) <= 0) {
        z = scalbn(z, n);
    } else {
        F64_HI(z) += (n << 20);
    }
    return s * z;
}
