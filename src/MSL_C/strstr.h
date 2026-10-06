/* MSL_C/strstr.h - the MSL string and conversion routines `MSL_C/alloc.cpp` owns that the DWC units call (docs/plan.md
 *   6.5 rule 2, leaf header): `strstr`, `strncmp`, `wcslen`, `strtol`. */
#ifndef MHTRI_MSL_C_STRSTR_H
#define MHTRI_MSL_C_STRSTR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F97C - the first occurrence of `needle` in `haystack`, or NULL. */
char* strstr(const char* haystack, const char* needle);

/* 0x8045F7A0 - compares at most `n` characters. */
int strncmp(const char* a, const char* b, u32 n);

/* 0x80463B20 - the length of a wide string in characters. */
u32 wcslen(const u16* string);

/* 0x804615C8 - converts the leading integer of `string` in `base`. */
s32 strtol(const char* string, char** end, s32 base);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_STRSTR_H */
