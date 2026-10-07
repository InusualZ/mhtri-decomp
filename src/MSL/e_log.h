/*
 * MSL/e_log.h - the declaration of `__ieee754_log`, owned by `MSL/e_log.cpp`.
 */
#ifndef MSL_E_LOG_H
#define MSL_E_LOG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80464B38 - the natural logarithm of `x`. */
f64 __ieee754_log(f64 x);

#ifdef __cplusplus
}
#endif

#endif
