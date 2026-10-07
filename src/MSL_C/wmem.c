/*
 * MSL_C/wmem.c - the wide-character memory routines (`wmemcpy` and two siblings).
 *
 * RANGE. .text 0x80461778..0x80461808 (3 functions in the map, 0x90 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `wmem`); `wmemset` and `wmemchr` name the two unnamed rows by their bodies (GUESS).
 * EVIDENCE. family only: `wmemcpy` is called by the wide-string writer of the wide printf unit; no data.
 * RESIDUALS. COARSE: three small functions, boundary to the neighbours by names.
 * SHAPES. `wmemcpy` is a tail call of `memcpy` with the count doubled.
 */
#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"

u16* wmemcpy(u16* dst, const u16* src, u32 n)
{
    return memcpy(dst, src, n * 2);
}

u16* wmemset(u16* dst, u16 value, u32 n)
{
    u16* start = dst;

    while (n != 0) {
        *dst++ = value;
        n--;
    }
    return start;
}

u16* wmemchr(const u16* s, u16 value, u32 n)
{
    while (n != 0) {
        if (*s == value) {
            return (u16*)s;
        }
        s++;
        n--;
    }
    return 0;
}
