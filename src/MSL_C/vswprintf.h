/* Leaf header (docs/plan.md 6.5 rule 2): `MSL_C/wprintf.c`'s `vswprintf` (0x80463A94), for `mh3_pad.cpp`.
 * C linkage (the map row is a plain name). */
#ifndef MHTRI_MSL_C_VSWPRINTF_H
#define MHTRI_MSL_C_VSWPRINTF_H

#include "types.h"
#include "stdarg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463A94: formats `format` with `args` into the wide buffer `s` of `n` characters. */
int vswprintf(u16* s, u32 n, const u16* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_VSWPRINTF_H */
