/*
 * TRK/mem_TRK.c - MetroTRK's own `TRK_memcpy` and `TRK_memset`.
 *
 * RANGE. .text 0x804689C8..0x80468C2C (2 functions in the map, 0x264 B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `mem_TRK`); both function names are the dump's.
 * EVIDENCE. two dump names; no data; called throughout the nub. Both bodies read and write one byte per iteration
 *    through the containing word (mask, shift, merge), as the target does.
 * RESIDUALS. none known.
 * SHAPES. both return their destination and loop on the remaining count; `TRK_memcpy` moves each byte through the inline
 *    word read/merge helpers, `TRK_memset` merges in place. The local declaration order (shift, word, mask) is the register
 *    order the target allocates.
 */
#include "TRK/mem_TRK.h"

/* Reads the byte at `p` through its containing word. */
static inline u8 trk_read_byte(const u8* p)
{
    u32 base = (u32)p & ~3;
    return *(u32*)base >> ((3 - ((u32)p - base)) * 8);
}

/* Writes `v` to the byte at `p` by merging it into the containing word. */
static inline void trk_write_byte(u8* p, u8 v)
{
    u32 sh;
    u32 base;
    u32 mask;
    base = (u32)p & ~3;
    sh = (3 - ((u32)p - base)) * 8;
    mask = 0xFF << sh;
    *(u32*)base = (*(u32*)base & ~mask) | (mask & (v << sh));
}

/* untyped: memcpy-shaped byte range */
void* TRK_memcpy(void* dst, const void* src, u32 n)
{
    const u8* s = (const u8*)src;
    u8* d = (u8*)dst;
    while (n != 0) {
        trk_write_byte(d, trk_read_byte(s));
        s++;
        d++;
        n--;
    }
    return dst;
}

/* untyped: memset-shaped byte range */
void* TRK_memset(void* dst, int val, u32 n)
{
    u8* d = (u8*)dst;
    u8 v = val;
    while (n != 0) {
        u32 sh;
        u32 mask;
        u32 db;
        db = (u32)d & ~3;
        sh = (3 - ((u32)d - db)) * 8;
        mask = 0xFF << sh;
        *(u32*)db = (*(u32*)db & ~mask) | (mask & (v << sh));
        d++;
        n--;
    }
    return dst;
}
