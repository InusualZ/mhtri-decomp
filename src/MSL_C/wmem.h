/*
 * MSL_C/wmem.h - the wide-character memory routines owned by `MSL_C/wmem.c`.
 */
#ifndef MSL_C_WMEM_H
#define MSL_C_WMEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80461778 (0x8): copies `n` wide characters. */
u16* wmemcpy(u16* dst, const u16* src, u32 n);

/* 0x804617E0 (0x28): returns the first `value` in the first `n` wide characters of `s`, or NULL. */
u16* wmemchr(const u16* s, u16 value, u32 n);

#ifdef __cplusplus
}
#endif

#endif
