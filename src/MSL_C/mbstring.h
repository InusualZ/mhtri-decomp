/*
 * MSL_C/mbstring.h - the multibyte conversion entry points, owned by `MSL_C/mbstring.c`.
 */
#ifndef MSL_C_MBSTRING_H
#define MSL_C_MBSTRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045B3A0 (0x18): decodes one multibyte character through the active locale's decoder. */
s32 mbtowc(u16* wide, const char* bytes, u32 count);

/* 0x8045B3B8 (0x4C): the one-byte-per-character decoder of the C locale. */
s32 __mbtowc_noconv(u16* wide, const char* bytes, u32 count);

/* 0x8045B404 (0x1C): the one-byte-per-character encoder of the C locale. */
s32 __wctomb_noconv(char* bytes, char wide);

/* 0x8045B420 (0xC0): converts a multibyte string to at most `count` wide characters; returns the number written or -1. */
u32 mbstowcs(u16* wide, const char* bytes, u32 count);

/* 0x8045B4E0 (0xB8): converts a wide string to at most `count` bytes; returns the number written. */
u32 wcstombs(char* bytes, const u16* wide, u32 count);

#ifdef __cplusplus
}
#endif

#endif
