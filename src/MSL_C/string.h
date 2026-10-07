/*
 * MSL_C/string.h - the narrow string routines the multibyte conversions call, owned by `MSL_C/string.c`.
 */
#ifndef MSL_C_STRING_H
#define MSL_C_STRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F614 (0x44): copies at most `n` bytes of `src` to `dst`, padding with zeros. */
char* strncpy(char* dst, const char* src, u32 n);

#ifdef __cplusplus
}
#endif

#endif
