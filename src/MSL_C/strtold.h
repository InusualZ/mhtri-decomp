/*
 * MSL_C/strtold.h - the string-to-floating-point scanner, owned by `MSL_C/strtold.c`.
 */
#ifndef MSL_C_STRTOLD_H
#define MSL_C_STRTOLD_H

#include "MSL_C/strtoul.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F9E8 (0x1324): scans a floating-point number through `read` and returns its value. */
f64 __strtold(int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* overflow);

#ifdef __cplusplus
}
#endif

#endif
