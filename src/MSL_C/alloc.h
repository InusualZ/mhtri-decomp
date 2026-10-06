/*
 * Declarations of `src/MSL_C/alloc.cpp` - the MSL C library block (`.text` 0x804578FC..0x804642C8): the string, stdio and
 * conversion helpers the SDK links instead of a `stdlib` (`-nosyspath` leaves the prototypes to the unit that calls them).
 * Moved here from `unsplit/Runtime.PPCEABI.H.h` when the phase 4 split registered the unit; that band header
 * includes this one.  Other units that still declare them locally (`src/g3d/g3d_resanmtexsrt.cpp`,
 * `src/homebutton/keyboard_ui.cpp`, `src/light/light.cpp`, `src/font/flfnt.cpp`) can adopt it when next touched.
 */
#ifndef MHTRI_MSL_C_ALLOC_H
#define MHTRI_MSL_C_ALLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

int strcmp(const char* a, const char* b);      /* 0x8045F684, consumed by `Network/PatInterface.cpp` */
int sprintf(char* dst, const char* fmt, ...); /* 0x8045DECC */
char* strchr(const char* s, int c);           /* 0x8045F7E0 */
int atoi(const char* s);                      /* 0x804616B4 */
char* strcpy(char* dst, const char* src);     /* 0x8045F554 */
char* strncpy(char* dst, const char* src, u32 n); /* 0x8045F614 */
char* strcat(char* dst, const char* src);     /* 0x8045F658 */
int printf(const char* fmt, ...);             /* 0x8045EDBC */
void* memmove(void* dst, const void* src, u32 n);        /* 0x8045B598; untyped: memcpy-shaped byte range */
int memcmp(const void* a, const void* b, u32 n);         /* 0x8045B6BC; untyped: byte range */
int snprintf(char* dst, u32 size, const char* fmt, ...); /* 0x8045DDD8 - bounded formatter */
/* 0x80463B58 - the wide-string copy; the elements this image's wide strings use are 2 bytes. */
u16* wcsncpy(u16* dst, const u16* src, u32 n);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/DWCi.h, NetworkStream.h, unknown.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct DWCiCType;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8060EDB0 - the character-class record `DWCi_parseAddress` validates port digits against (a header
 * whose +0x38 pointer reaches the per-character u16 flags, bit 3 marking a decimal digit). */
extern struct DWCiCType DWCi_digitClassTable;

/* 0x8045DFA0 - the C library's `rand` (the 0x41C64E6D linear congruential step, top 15 bits). */
s32 rand(void);

/* 0x8045DFC0 - seed `rand` (the 8-byte setter right after it). */
void srand(u32 seed);

/* 0x80463E08 - the arctangent of `y / x` in radians (the libm `atan2` entry); `Pl/pl_yure.cpp` turns it into
 * a 16-bit angle word.  0x805015C8 - `out = mtx * v` (the paired-single matrix/vector multiply); the two
 * vectors may alias.  Added with that unit. */
f32 atan2f(f32 y, f32 x);

/* 0x8045F37C */
int sscanf(const char* src, const char* fmt, ...);

/* 0x8045F858 */
char* strtok(char* string, const char* delimiters);

/* 0x8045AB38 / 0x8045AB48 - the absolute value (`srawi`/`xor`/`subf`).  The two bodies are the same; MSL's
 * arith.c defines `abs` before `labs`, so the first is `abs` and the second `labs` (a GUESS by that order). */
int abs(int n);
long labs(long n);

/* 0x80463E74 - the global frame thunk `g3d/g3d_resanmlight.cpp`'s `fn_8008FFFC` tail-calls. */
f32 fn_80463E74(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_ALLOC_H */
