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

#ifdef __cplusplus
}
#endif

#endif
