/*
 * MSL_C/string.h - the narrow string routines the multibyte conversions call, owned by `MSL_C/string.c`.
 */
#ifndef MSL_C_STRING_H
#define MSL_C_STRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F554 (0xC0): copies the string `src` including its terminator to `dst`. */
char* strcpy(char* dst, const char* src);

/* 0x8045F614 (0x44): copies at most `n` bytes of `src` to `dst`, padding with zeros. */
char* strncpy(char* dst, const char* src, u32 n);

/* 0x8045F7E0 (0x30): returns the first occurrence of `c` in `s`, or NULL. */
char* strchr(const char* s, int c);

#ifdef __cplusplus
}
#endif

#endif
