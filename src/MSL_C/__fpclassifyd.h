/*
 * __fpclassifyd.h - the declaration of `__fpclassifyd`, owned by `MSL_C/misc_io.c`.
 */
#ifndef MSL_C_FPCLASSIFYD_H
#define MSL_C_FPCLASSIFYD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045BA50 - classifies a double (NaN, infinity, zero, normal, subnormal). */
int __fpclassifyd(f64 x);

#ifdef __cplusplus
}
#endif

#endif
