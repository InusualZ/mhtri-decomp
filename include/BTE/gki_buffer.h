/*
 * BTE/gki_buffer.h - declarations of the symbols owned by `BTE/gki_buffer.cpp` (the 0x804772F0..0x804AFED0 band)
 * that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef BTE_GKI_BUFFER_H
#define BTE_GKI_BUFFER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The RVL SDK's ENC library (`<< RVL_SDK - ENC ... >>`, .data 0x8061A4A8, registered by 0x804AE880 on first use).
 * 0x804AEAC0 / 0x804AEAD0 are two-instruction wrappers that pass a zero fifth argument to the converters 0x804AEAE0
 * (UTF-16 in: honours a 0xFEFF/0xFFFE byte-order mark, joins surrogate pairs, emits 1..4 bytes per code point) and
 * 0x804AEDF0 (the reverse).  The lengths are in/out: the space on entry, the units used on return.  NAMES (GUESSES
 * in the ENC scheme, from the conversion each one performs). */
s32 ENCConvertStringUtf16ToUtf8(u8* dst, s32* dstLength, const u16* src, s32* srcLength);
s32 ENCConvertStringUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength);

#ifdef __cplusplus
}
#endif

#endif /* BTE_GKI_BUFFER_H */
