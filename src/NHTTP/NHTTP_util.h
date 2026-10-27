/*
 * NHTTP/NHTTP_util.h - the calls `src/NHTTP/NHTTP_util.c` defines (.text 0x80516CA0..0x80517614): the C-library
 * shims and the string, URL, number and Base64 helpers.
 */
#ifndef MHTRI_NHTTP_NHTTP_UTIL_H
#define MHTRI_NHTTP_NHTTP_UTIL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: byte range */
void* NHTTPi_memcpy(void* dst, const void* src, u32 n);
u32 NHTTPi_strlen(const char* s);
int NHTTPi_strcmp(const char* a, const char* b);
/* untyped: byte range */
void* NHTTPi_memclr(void* dst, u32 n);
s32 NHTTPi_strnicmp(const char* s1, const char* s2, s32 n);
s32 NHTTPi_urlEncodedLength(const char* s);
s32 NHTTPi_urlEncodedLengthN(const char* s, s32 n);
s32 NHTTPi_encodeUrlChar(u8* dst, char c);
s32 NHTTPi_strToHex(const char* s, s32 n);
s32 NHTTPi_strToDec(const char* s, s32 n);
s32 NHTTPi_uintToStr(char* buf, u32 value);
s32 NHTTPi_compareToken(const char* a, const char* b);
s32 NHTTPi_strtonum(const char* s, s32 n);
s32 NHTTPi_containsString(const char* haystack, s32 haystackLen, const char* needle, s32 needleLen);
s32 NHTTPi_Base64Encode(char* dst, const char* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_UTIL_H */
