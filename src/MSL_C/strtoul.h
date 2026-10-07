/*
 * MSL_C/strtoul.h - the string-to-integer scanners, owned by `MSL_C/strtoul.c`.
 */
#ifndef MSL_C_STRTOUL_H
#define MSL_C_STRTOUL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The character source of the scanners: `read(arg, 0, 0)` reads, `read(arg, ch, 1)` ungets, `read(arg, 0, 2)` tests the end. */
typedef struct ReadArg ReadArg; /* opaque state of the character source */
typedef int (*ReadProc)(ReadArg*, int, int);

/* 0x80460D0C (0x414): scans an unsigned integer through `read` and returns its magnitude. */
u32 __strtoul(int base, int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* negative, int* overflow);

/* 0x80461120 (0x4A8): scans an unsigned 64-bit integer through `read` and returns its magnitude. */
u64 __strtoull(int base, int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* negative, int* overflow);

/* 0x804615C8 (0xEC): converts a decimal, octal or hexadecimal string to a signed long. */
long strtol(const char* str, char** end, int base);

/* 0x804616B4 (0xC4): converts a decimal string to an int. */
int atoi(const char* str);

#ifdef __cplusplus
}
#endif

#endif
