/*
 * MSL/s_ldexp.h - the declaration of `ldexp`, owned by `MSL/s_ldexp.cpp`.
 */
#ifndef MSL_S_LDEXP_H
#define MSL_S_LDEXP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80467BB8 - `value` times two to the power `exp`, with overflow and underflow handled. */
f64 ldexp(f64 value, s32 exp);

#ifdef __cplusplus
}
#endif

#endif
