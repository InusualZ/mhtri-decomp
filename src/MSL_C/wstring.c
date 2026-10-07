/*
 * MSL_C/wstring.c - the wide-string routines: `wcslen`, `wcsncpy`, `wcscmp` and their siblings.
 *
 * RANGE. .text 0x80463B20..0x80463C48 (6 functions in the map, 0x128 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `wstring`); `wcscpy`, `wcsncat`, `wcschr` name the three unnamed rows by their bodies
 *    (GUESS for the dump has none; the `wcs*` scheme of the neighbours).
 * EVIDENCE. name family only (`wcs*`); no data.
 * RESIDUALS. COARSE: boundary by names and call-graph (the wide printf unit calls `wcslen`); `wcscmp` allocates the
 *    two characters in r0/r5 where the target uses r3/r0 (same opcodes).
 * SHAPES. wide characters are `u16`; every walk is a pre-incremented pointer.
 */
#include "types.h"

u32 wcslen(const u16* s)
{
    const u16* p = s - 1;
    u32 n = -1;

    do {
        n++;
    } while (*++p != 0);
    return n;
}

u16* wcscpy(u16* dst, const u16* src)
{
    const u16* s = src - 1;
    u16* d = dst - 1;
    u32 c;

    do {
        c = *++s;
        *++d = c;
    } while (c != 0);
    return dst;
}

u16* wcsncpy(u16* dst, const u16* src, u32 n)
{
    const u16* s = src - 1;
    u16* d = dst - 1;

    n++;
    while (--n != 0) {
        u32 c = *++s;
        *++d = c;
        if (c == 0) {
            while (--n != 0) {
                *++d = 0;
            }
            return dst;
        }
    }
    return dst;
}

u16* wcsncat(u16* dst, const u16* src, u32 n)
{
    const u16* s = src - 1;
    u16* d = dst - 1;

    while (*++d != 0) {
    }
    d--;
    n++;
    while (--n != 0) {
        u32 c = *++s;
        *++d = c;
        if (c == 0) {
            d--;
            break;
        }
    }
    *++d = 0;
    return dst;
}

int wcscmp(const u16* a, const u16* b)
{
    u32 d;
    u32 c;
    const u16* p = a - 1;
    const u16* q = b - 1;

    while ((c = *++p) == (d = *++q)) {
        if (c == 0) {
            return 0;
        }
    }
    return c - d;
}

u16* wcschr(const u16* s, u16 ch)
{
    const u16* p = s - 1;
    u32 c;

    while ((c = *++p) != 0) {
        if (c == ch) {
            return (u16*)p;
        }
    }
    if (ch != 0) {
        return 0;
    }
    return (u16*)p;
}
