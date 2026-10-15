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

#ifdef __cplusplus
}
#endif

#endif
