/*
 * MSL_C/printf.h - declarations of the symbols owned by `MSL_C/printf.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_MSL_C_PRINTF_H
#define MHTRI_MSL_C_PRINTF_H

#include "types.h"
#include "stdarg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045DB84 (0xCC): prints formatted output on the console. */
int printf(const char* format, ...);

/* 0x8045DC50 - prints `format` with the arguments in `args` through the console. */
int vprintf(const char* format, va_list args);

/* 0x8045DCCC (0x88): formats into a buffer of at most `n` characters from a variable argument list. */
int vsnprintf(char* s, u32 n, const char* format, va_list args);

/* 0x8045DD54 (0x84): formats into an unbounded buffer from a variable argument list. */
int vsprintf(char* s, const char* format, va_list args);

/* 0x8045DDD8 (0xF4): formats into a buffer of at most `n` characters. */
int snprintf(char* s, u32 n, const char* format, ...);

/* 0x8045DECC (0xD4): formats into an unbounded buffer. */
int sprintf(char* s, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_PRINTF_H */
