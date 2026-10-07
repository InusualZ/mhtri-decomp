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

/* 0x8045DC50 - prints `format` with the arguments in `args` through the console. */
int vprintf(const char* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_PRINTF_H */
