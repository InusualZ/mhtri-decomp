/*
 * MSL/strlen.cpp - the MSL `strlen`.
 *
 * RANGE. .text 0x804565C0..0x804565DC (1 function in the map, 0x1C B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. `strlen` is the dump's name.
 * EVIDENCE. the first function after the own-slot run of the `menu_sysmsg` band, named in the map.
 * RESIDUALS. none measured.
 * SHAPES. pre-incremented walk with the length counted from -1.
 */
#include "MSL/strlen.h"

u32 strlen(const char* s)
{
    const u8* p = (const u8*)s;
    u32 n = -1;

    p--;
    do {
        n++;
    } while (*++p != 0);
    return n;
}
