/*
 * MSL/s_ceil.cpp - the MSL `ceil` (fdlibm shape): clears the fraction bits of the two words, rounding up.
 *
 * RANGE. .text 0x804677A0..0x804678EC (1 function in the map, 0x14C B); .sdata2 0x8079CED0..0x8079CEE0 (the
 *    pooled 1e300 and 0.0).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `ceil` names the unnamed row (GUESS: the sibling of `floor` with the sign tests reversed).
 * EVIDENCE. `1e300 + x > 0.0` raises inexact before the truncation, as in every fdlibm rounding routine.
 * RESIDUALS. none measured.
 * SHAPES. the constants are literals so the compiler pools them itself.
 */
#pragma fp_contract off
#include "MSL/s_ceil.h"
#include "MSL/fdlibm.h"

extern "C" f64 ceil(f64 x)
{
    s32 i0;
    u32 i1;
    s32 j0;
    u32 i;
    u32 j;

    i0 = F64_HI(x);
    i1 = F64_LO(x);
    j0 = ((i0 >> 20) & 0x7FF) - 0x3FF;
    if (j0 < 20) {
        if (j0 < 0) {
            if (1.0e300 + x > 0.0) {
                if (i0 < 0) {
                    i0 = 0x80000000;
                    i1 = 0;
                } else if ((i0 | i1) != 0) {
                    i0 = 0x3FF00000;
                    i1 = 0;
                }
            }
        } else {
            i = 0x000FFFFF >> j0;
            if (((i0 & i) | i1) == 0) {
                return x;
            }
            if (1.0e300 + x > 0.0) {
                if (i0 > 0) {
                    i0 += 0x00100000 >> j0;
                }
                i0 &= ~i;
                i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400) {
            return x + x;
        }
        return x;
    } else {
        i = 0xFFFFFFFF >> (j0 - 20);
        if ((i1 & i) == 0) {
            return x;
        }
        if (1.0e300 + x > 0.0) {
            if (i0 > 0) {
                if (j0 == 20) {
                    i0 += 1;
                } else {
                    j = i1 + (1 << (52 - j0));
                    if (j < i1) {
                        i0 += 1;
                    }
                    i1 = j;
                }
            }
            i1 &= ~i;
        }
    }
    F64_HI(x) = i0;
    F64_LO(x) = i1;
    return x;
}
