/*
 * MSL_C/mem.c - the MSL memory routines: `memmove`, `memchr`, `__memrchr`, `memcmp` and the `__copy_longs_*`
 *    helpers `memmove` calls.
 *
 * RANGE. .text 0x8045B598..0x8045B9D8 (8 functions in the map, 0x440 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `mem`); `memcpy` and `memset` are registered separately
 *    (`Runtime.PPCEABI.H/memcpy.c`, `memset.c`).
 * EVIDENCE. call-graph closure: `memmove` calls the four `__copy_longs` helpers, nothing else calls them; no data.
 * RESIDUALS. COARSE: MSL may split `mem` and `mem_funcs`; no evidence to separate them. Register allocation only in
 *    `memcmp`, `__copy_longs_aligned`, `__copy_longs_unaligned` and `__copy_longs_rev_unaligned` (same opcode
 *    sequences; the target keeps the destination in the parameter register).
 * SHAPES. the copy helpers align the destination bytewise, move 32-byte blocks, then single words, then the
 *    tail bytes; the unaligned ones merge two source words per destination word.
 */
#include "types.h"

static void __copy_longs_aligned(u8* dst, const u8* src, u32 n);
static void __copy_longs_rev_aligned(u8* dst, const u8* src, u32 n);
static void __copy_longs_unaligned(u8* dst, const u8* src, u32 n);
static void __copy_longs_rev_unaligned(u8* dst, const u8* src, u32 n);

/* untyped: memmove-shaped byte range */
void* memmove(void* dst, const void* src, u32 n)
{
    int reverse = (u32)dst > (u32)src;

    if (n >= 32) {
        if ((((u32)dst ^ (u32)src) & 3) != 0) {
            if (!reverse) {
                __copy_longs_unaligned((u8*)dst, (const u8*)src, n);
            } else {
                __copy_longs_rev_unaligned((u8*)dst, (const u8*)src, n);
            }
        } else {
            if (!reverse) {
                __copy_longs_aligned((u8*)dst, (const u8*)src, n);
            } else {
                __copy_longs_rev_aligned((u8*)dst, (const u8*)src, n);
            }
        }
        return dst;
    }
    if (!reverse) {
        const u8* s = src;
        u8* d = dst;

        s--;
        d--;
        n++;
        while (--n != 0) {
            *++d = *++s;
        }
    } else {
        const u8* s = src;
        u8* d = dst;

        s += n;
        d += n;
        n++;
        while (--n != 0) {
            *--d = *--s;
        }
    }
    return dst;
}

/* untyped: byte range */
void* memchr(const void* buf, int ch, u32 n)
{
    u32 want = (u8)ch;
    const u8* p = buf;

    p--;
    n++;
    while (--n != 0) {
        if (*++p == want) {
            return (void*)p;
        }
    }
    return 0;
}

/* untyped: byte range */
void* __memrchr(const void* buf, int ch, u32 n)
{
    const u8* p = buf;
    u32 want = (u8)ch;

    p += n;
    n++;
    while (--n != 0) {
        if (*--p == want) {
            return (void*)p;
        }
    }
    return 0;
}

/* untyped: byte range */
int memcmp(const void* a, const void* b, u32 n)
{
    const u8* q;
    u32 d;
    u32 c;
    const u8* p;

    q = b;
    p = a;
    q--;
    p--;
    for (n++; --n != 0;) {
        c = *++p;
        d = *++q;
        if (c != d) {
            return (*p < *q) ? -1 : 1;
        }
    }
    return 0;
}

static void __copy_longs_aligned(u8* dst, const u8* src, u32 n)
{
    u32 i = (-(u32)dst) & 3;
    u32* wd;
    const u32* ws;
    u8* wb;
    const u8* sb;

    src = src - 1;
    dst = dst - 1;
    if (i != 0) {
        n -= i;
        do {
            *++dst = *++src;
        } while (--i != 0);
    }
    wb = dst - 3;
    sb = src - 3;
    wd = (u32*)wb;
    ws = (const u32*)sb;
    i = n >> 5;
    if (i != 0) {
        do {
            wd[1] = ws[1];
            wd[2] = ws[2];
            wd[3] = ws[3];
            wd[4] = ws[4];
            wd[5] = ws[5];
            wd[6] = ws[6];
            wd[7] = ws[7];
            *(wd += 8) = *(ws += 8);
        } while (--i != 0);
    }
    i = (n >> 2) & 7;
    if (i != 0) {
        do {
            *++wd = *++ws;
        } while (--i != 0);
    }
    dst = (u8*)wd;
    src = (const u8*)ws;
    dst += 3;
    src += 3;
    n &= 3;
    if (n != 0) {
        do {
            *++dst = *++src;
        } while (--n != 0);
    }
}

static void __copy_longs_rev_aligned(u8* dst, const u8* src, u32 n)
{
    u8* d = dst + n;
    const u8* s = src + n;
    u32 i = (u32)d & 3;
    u32* wd;
    const u32* ws;

    if (i != 0) {
        n -= i;
        do {
            *--d = *--s;
        } while (--i != 0);
    }
    wd = (u32*)d;
    ws = (const u32*)s;
    i = n >> 5;
    if (i != 0) {
        do {
            wd[-1] = ws[-1];
            wd[-2] = ws[-2];
            wd[-3] = ws[-3];
            wd[-4] = ws[-4];
            wd[-5] = ws[-5];
            wd[-6] = ws[-6];
            wd[-7] = ws[-7];
            *(wd -= 8) = *(ws -= 8);
        } while (--i != 0);
    }
    i = (n >> 2) & 7;
    if (i != 0) {
        do {
            *--wd = *--ws;
        } while (--i != 0);
    }
    d = (u8*)wd;
    s = (const u8*)ws;
    n &= 3;
    if (n != 0) {
        do {
            *--d = *--s;
        } while (--n != 0);
    }
}

static void __copy_longs_unaligned(u8* dst, const u8* src, u32 n)
{
    u32 i = (-(u32)dst) & 3;
    u32 lsh;
    u32 rsh;
    u32* wd;
    u32 soff;
    const u32* ws;
    u8* wb;
    const u8* sb;
    u32 w0;
    u32 w1;

    src = src - 1;
    dst = dst - 1;
    if (i != 0) {
        n -= i;
        do {
            *++dst = *++src;
        } while (--i != 0);
    }
    soff = (u32)(src + 1) & 3;
    lsh = ((u32)(src + 1) << 3) & 0x18;
    rsh = 32 - lsh;
    sb = src - soff;
    wb = dst - 3;
    ws = (const u32*)sb;
    wd = (u32*)wb;
    i = n >> 3;
    sb = (const u8*)ws;
    sb++;
    ws = (const u32*)sb;
    w0 = *ws;
    do {
        w1 = ws[1];
        wd[1] = (w0 << lsh) | (w1 >> rsh);
        w0 = *(ws += 2);
        *(wd += 2) = (w1 << lsh) | (w0 >> rsh);
    } while (--i != 0);
    if (n & 4) {
        w1 = *++ws;
        *++wd = (w0 << lsh) | (w1 >> rsh);
    }
    n &= 3;
    src = (const u8*)ws;
    dst = (u8*)wd;
    src += 3;
    dst += 3;
    if (n != 0) {
        src -= 4 - soff;
        do {
            *++dst = *++src;
        } while (--n != 0);
    }
}

static void __copy_longs_rev_unaligned(u8* dst, const u8* src, u32 n)
{
    u8* d = dst + n;
    u32 i = (u32)d & 3;
    u32 rsh;
    u32 lsh;
    u32* wd;
    u32 soff;
    const u32* ws;
    u32 w0;
    u32 w1;

    src = src + n;
    if (i != 0) {
        n -= i;
        do {
            *--d = *--src;
        } while (--i != 0);
    }
    lsh = ((u32)src << 3) & 0x18;
    soff = (u32)src & 3;
    rsh = 32 - lsh;
    i = n >> 3;
    src = src + (4 - soff);
    ws = (const u32*)src;
    wd = (u32*)d;
    w0 = *--ws;
    do {
        w1 = *--ws;
        *--wd = (w1 << lsh) | (w0 >> rsh);
        w0 = *--ws;
        *--wd = (w0 << lsh) | (w1 >> rsh);
    } while (--i != 0);
    if (n & 4) {
        w1 = *--ws;
        *--wd = (w1 << lsh) | (w0 >> rsh);
    }
    n &= 3;
    d = (u8*)wd;
    if (n != 0) {
        src = (const u8*)ws;
        src += soff;
        do {
            *--d = *--src;
        } while (--n != 0);
    }
}
