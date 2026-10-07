/*
 * MSL/s_copysign.cpp - the MSL `copysign` (fdlibm shape): the magnitude of `x` with the sign of `y`.
 *
 * RANGE. .text 0x804678EC..0x80467918 (1 function in the map, 0x2C B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `copysign` is the map's name.
 * EVIDENCE. the high words of both arguments are read through their stack slots.
 * RESIDUALS. none measured.
 * SHAPES. both arguments live in memory; only the high word of `x` is rewritten.
 */
#include "MSL/s_copysign.h"

extern "C" f64 copysign(f64 x, f64 y)
{
    u32* hx = (u32*)&x;
    u32* hy = (u32*)&y;

    *hx = (*hx & 0x7FFFFFFF) | (*hy & 0x80000000);
    return x;
}
