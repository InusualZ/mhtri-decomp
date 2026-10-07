/*
 * MSL/mathf.cpp - the single-precision libm wrappers: each narrows the result of the matching double routine.
 *
 * RANGE. .text 0x80463DE4..0x80463FFC (14 functions in the map, 0x218 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; compiled as C++ with `extern "C"` linkage.
 * NAMES. `atan2f`, `cosf`, `fabsf`, `fmodf`, `tanf`, `scalbn` are the map's (`cosf`, `fabsf`, `fmodf` GUESS: the dump labels
 *    those addresses `tanf` or a placeholder); `asinf`, `ceilf`, `floorf`, `ldexpf`,
 *    `powf`, `sinf`, `modff`, `sqrtf` name the unnamed rows by the one double routine each calls (GUESS).
 * EVIDENCE. every body is one call and an `frsp`; `scalbn` is the one double-precision row (frexp, add, ldexp).
 * RESIDUALS. none measured.
 * SHAPES. float arguments reach the double callee unconverted (hardware FP keeps them in double registers).
 */
#include "MSL/w_math.h"
#include "MSL/s_ceil.h"
#include "MSL/s_cos.h"
#include "MSL/s_floor.h"
#include "MSL/s_frexp.h"
#include "MSL/s_ldexp.h"
#include "MSL/s_modf.h"
#include "MSL/s_sin.h"
#include "MSL/s_tan.h"

extern "C" f32 asinf(f32 x)
{
    return (f32)asin(x);
}

extern "C" f32 atan2f(f32 y, f32 x)
{
    return (f32)atan2(y, x);
}

extern "C" f32 ceilf(f32 x)
{
    return (f32)ceil(x);
}

extern "C" f32 cosf(f32 x)
{
    return (f32)cos(x);
}

extern "C" f32 floorf(f32 x)
{
    return (f32)floor(x);
}

extern "C" f32 ldexpf(f32 x, s32 n)
{
    return (f32)ldexp(x, n);
}

extern "C" f32 powf(f32 x, f32 y)
{
    return (f32)pow(x, y);
}

extern "C" f32 sinf(f32 x)
{
    return (f32)sin(x);
}

extern "C" f32 fabsf(f32 x)
{
    f64 a = __fabs(x);

    return (f32)a;
}

extern "C" f32 fmodf(f32 x, f32 y)
{
    return (f32)fmod(x, y);
}

extern "C" f32 modff(f32 x, f32* iptr)
{
    f64 ip;
    f32 frac = (f32)modf(x, &ip);

    *iptr = (f32)ip;
    return frac;
}

extern "C" f32 sqrtf(f32 x)
{
    return (f32)sqrt(x);
}

extern "C" f32 tanf(f32 x)
{
    return (f32)tan(x);
}

extern "C" f64 scalbn(f64 x, s32 n)
{
    s32 e;

    x = frexp(x, &e);
    e += n;
    return ldexp(x, e);
}
