/*
 * MSL/e_fmod.cpp - the MSL `__ieee754_fmod` (fdlibm shape): the remainder by shift-and-subtract on the mantissa words.
 *
 * RANGE. .text 0x804647B8..0x80464B38 (1 function in the map, 0x380 B); .rodata 0x80573190..0x805731A0 (the
 *    `{0.0, -0.0}` sign table).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `__ieee754_fmod` is the map's name; `fmod_zero` (the signed-zero table) is a GUESS. `__ieee754_fmod` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 * EVIDENCE. the 16-byte `.rodata` run is two doubles, +0.0 and -0.0, indexed by the sign of `x`.
 * RESIDUALS. register allocation only (the shift count and the shifted `ly` live in r10/r11 in the target; same opcodes).
 *    The early-out `hz == 0 && (lx >> k) == (ly >> k)` before each subtract is read from the target's instructions.
 * SHAPES. the scalar constant 1.0 is a literal so the compiler pools it itself.
 */
#pragma fp_contract off
#include "MSL/e_fmod.h"
#include "MSL/fdlibm.h"

static const f64 fmod_zero[2] = {0.0, -0.0};

extern "C" f64 __ieee754_fmod(f64 x, f64 y)
{
    s32 n;
    s32 hx;
    s32 hy;
    s32 hz;
    s32 ix;
    s32 iy;
    s32 sx;
    s32 i;
    s32 k;
    u32 lyk;
    u32 lx;
    u32 ly;
    u32 lz;

    hx = F64_HI(x);
    lx = F64_LO(x);
    hy = F64_HI(y);
    ly = F64_LO(y);
    sx = hx & 0x80000000;
    hx ^= sx;
    hy &= 0x7FFFFFFF;
    if ((hy | ly) == 0 || (hx >= 0x7FF00000) || ((hy | ((ly | -ly) >> 31)) > 0x7FF00000)) {
        return (x * y) / (x * y);
    }
    if (hx <= hy) {
        if ((hx < hy) || (lx < ly)) {
            return x;
        }
        if (lx == ly) {
            return fmod_zero[(u32)sx >> 31];
        }
    }
    if (hx < 0x00100000) {
        if (hx == 0) {
            for (ix = -1043, i = lx; i > 0; i <<= 1) {
                ix -= 1;
            }
        } else {
            for (ix = -1022, i = (hx << 11); i > 0; i <<= 1) {
                ix -= 1;
            }
        }
    } else {
        ix = (hx >> 20) - 1023;
    }
    if (hy < 0x00100000) {
        if (hy == 0) {
            for (iy = -1043, i = ly; i > 0; i <<= 1) {
                iy -= 1;
            }
        } else {
            for (iy = -1022, i = (hy << 11); i > 0; i <<= 1) {
                iy -= 1;
            }
        }
    } else {
        iy = (hy >> 20) - 1023;
    }
    if (ix >= -1022) {
        hx = 0x00100000 | (0x000FFFFF & hx);
    } else {
        n = -1022 - ix;
        if (n <= 31) {
            hx = (hx << n) | (lx >> (32 - n));
            lx <<= n;
        } else {
            hx = lx << (n - 32);
            lx = 0;
        }
    }
    if (iy >= -1022) {
        hy = 0x00100000 | (0x000FFFFF & hy);
    } else {
        n = -1022 - iy;
        if (n <= 31) {
            hy = (hy << n) | (ly >> (32 - n));
            ly <<= n;
        } else {
            hy = ly << (n - 32);
            ly = 0;
        }
    }
    n = ix - iy;
    k = n + 2;
    lyk = ly >> k;
    while (n--) {
        hz = hx - hy;
        lz = lx - ly;
        if (hz == 0 && (lx >> k) == lyk) {
            return fmod_zero[(u32)sx >> 31];
        }
        if (lx < ly) {
            hz -= 1;
        }
        if (hz < 0) {
            hx = hx + hx + (lx >> 31);
            lx = lx + lx;
        } else {
            if ((hz | lz) == 0) {
                return fmod_zero[(u32)sx >> 31];
            }
            hx = hz + hz + (lz >> 31);
            lx = lz + lz;
        }
    }
    hz = hx - hy;
    lz = lx - ly;
    if (hz == 0 && (lx >> k) == lyk) {
        return fmod_zero[(u32)sx >> 31];
    }
    if (lx < ly) {
        hz -= 1;
    }
    if (hz >= 0) {
        hx = hz;
        lx = lz;
    }
    if ((hx | lx) == 0) {
        return fmod_zero[(u32)sx >> 31];
    }
    while (hx < 0x00100000) {
        hx = hx + hx + (lx >> 31);
        lx = lx + lx;
        iy -= 1;
    }
    if (iy >= -1022) {
        hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
        F64_HI(x) = hx | sx;
        F64_LO(x) = lx;
    } else {
        n = -1022 - iy;
        if (n <= 20) {
            lx = (lx >> n) | ((u32)hx << (32 - n));
            hx >>= n;
        } else if (n <= 31) {
            lx = (hx << (32 - n)) | (lx >> n);
            hx = sx;
        } else {
            lx = hx >> (n - 32);
            hx = sx;
        }
        F64_HI(x) = hx | sx;
        F64_LO(x) = lx;
        x *= 1.0;
    }
    return x;
}
