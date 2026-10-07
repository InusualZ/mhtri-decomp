/*
 * OS/OSUtf.h - declarations of the symbols owned by `OS/OSUtf.c` that other units call.
 */
#ifndef OS_OSUTF_H
#define OS_OSUTF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D5420 - decodes one UTF-8 sequence at `src` into `*out`; returns the next position, NULL when malformed. */
u8* OSUTF8to32(u8* src, u32* out);

/* 0x804D5540 - decodes one UTF-16 unit or surrogate pair at `src` into `*out`; returns the next position, NULL when malformed. */
u16* OSUTF16to32(u16* src, u32* out);

/* 0x804D55B0 - maps a code point to its Windows-1252 byte in the 0x80..0x9F block; 0 when it has none. */
u8 OSUTF32toANSI(u32 code);

/* 0x804D5630 - maps a code point to its Shift-JIS code through the two-level table; 0 when it has none. */
u16 OSUTF32toSJIS(u32 code);

#ifdef __cplusplus
}
#endif

#endif
