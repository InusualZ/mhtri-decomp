/*
 * TRK/mem_TRK.c - MetroTRK's own `TRK_memcpy` and `TRK_memset`.
 *
 * RANGE. .text 0x804689C8..0x80468C2C (2 functions in the map, 0x264 B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `mem_TRK`); both function names are the dump's.
 * EVIDENCE. two dump names; no data; called throughout the nub. Both bodies read and write one byte per iteration
 *    through the containing word (mask, shift, merge), as the target does.
 * RESIDUALS. register allocation only: `TRK_memcpy` keeps the destination cursor in r30 and the shift in r31 where ours
 *    uses r11/r10 (same opcodes, 95 %); `TRK_memset` names the word address r10 and the mask r9 where ours swaps them
 *    (97.6 %).
 * SHAPES. both return their destination and loop on the remaining count.
 */
#include "TRK/mem_TRK.h"

/* untyped: memcpy-shaped byte range */
void* TRK_memcpy(void* dst, const void* src, u32 n)
{
    const u8* s = (const u8*)src;
    u8* d = (u8*)dst;
    while (n != 0) {
        u32 db = (u32)d & ~3;
        u32 sb = (u32)s & ~3;
        u32 mask = 0xFF << ((3 - ((u32)d - db)) * 8);
        u32 b = (*(u32*)sb >> ((3 - ((u32)s - sb)) * 8)) & 0xFF;
        *(u32*)db = (*(u32*)db & ~mask) | (mask & (b << ((3 - ((u32)d - db)) * 8)));
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
        u32 db = (u32)d & ~3;
        u32 sh = (3 - ((u32)d - db)) * 8;
        u32 mask = 0xFF << sh;
        *(u32*)db = (*(u32*)db & ~mask) | (mask & (v << sh));
        d++;
        n--;
    }
    return dst;
}
