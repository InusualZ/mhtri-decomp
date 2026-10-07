/*
 * MSL/scalbn.h - the declaration of `scalbn`, owned by `MSL/mathf.cpp`.
 */
#ifndef MSL_SCALBN_H
#define MSL_SCALBN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463FBC - `x` times two to the power `n`. */
f64 scalbn(f64 x, s32 n);

#ifdef __cplusplus
}
#endif

#endif
