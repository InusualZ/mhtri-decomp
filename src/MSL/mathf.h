/*
 * MSL/mathf.h - declarations of the symbols owned by `MSL/mathf.cpp` that other units call.
 */
#ifndef MSL_MATHF_H
#define MSL_MATHF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns `x` times two to the `exp`. */
f32 ldexpf(f32 x, s32 exp);
/* 0x80463E50 / 0x80463EE0 - the single-precision cosine and sine. */
f32 cosf(f32 x);
f32 sinf(f32 x);
/* 0x80463EBC - `x` to the power `y`. */
f32 powf(f32 x, f32 y);
/* 0x80463E08 / 0x80463F74 - the single-precision arc tangent of `y / x` and square root. */
f32 atan2f(f32 y, f32 x);
f32 sqrtf(f32 x);

#ifdef __cplusplus
}
#endif

#endif
