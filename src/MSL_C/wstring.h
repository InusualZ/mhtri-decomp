/*
 * MSL_C/wstring.h - the wide-string routines owned by `MSL_C/wstring.c`.
 */
#ifndef MSL_C_WSTRING_H
#define MSL_C_WSTRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463B20 (0x1C): returns the length of a wide string in characters. */
u32 wcslen(const u16* s);

/* 0x80463B3C (0x1C): copies the wide string `src` including its terminator to `dst`. */
u16* wcscpy(u16* dst, const u16* src);

/* 0x80463C1C (0x2C): returns the first occurrence of `ch` in `s`, or NULL. */
u16* wcschr(const u16* s, u16 ch);

#ifdef __cplusplus
}
#endif

#endif
