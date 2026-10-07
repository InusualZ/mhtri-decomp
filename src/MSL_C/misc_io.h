/*
 * MSL_C/misc_io.h - float classification and the stdio exit hook setter, owned by `MSL_C/misc_io.c`
 *    (`__fpclassifyd` has its own leaf header, `MSL_C/__fpclassifyd.h`).
 */
#ifndef MSL_C_MISC_IO_H
#define MSL_C_MISC_IO_H

#include "types.h"

#define FP_NAN 1
#define FP_INFINITE 2
#define FP_ZERO 3
#define FP_NORMAL 4
#define FP_SUBNORMAL 5

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045B9D8 (0x60): classifies a float as FP_NAN, FP_INFINITE, FP_ZERO, FP_NORMAL or FP_SUBNORMAL. */
int __fpclassifyf(f32 x);

/* 0x8045BA38 (0x18): returns the sign bit of a double in place (0 or 0x80000000). */
s32 __signbitd(f64 x);

/* 0x8045BACC (0x10): registers the stream-closing routine `exit` runs. */
void __stdio_atexit(void);

#ifdef __cplusplus
}
#endif

#endif
