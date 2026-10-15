/*
 * MSL_C/wprintf.h - declarations of the symbols owned by `MSL_C/wprintf.c` that other units call.
 */
#ifndef MSL_C_WPRINTF_H
#define MSL_C_WPRINTF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Formats into the wide buffer `s` of `n` characters; returns the length or a negative value. */
int swprintf(u16* s, u32 n, const u16* format, ...);

#ifdef __cplusplus
}
#endif

#endif
