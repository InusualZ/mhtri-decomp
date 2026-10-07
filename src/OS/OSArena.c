/*
 * OS/OSArena.c - the OS arena bounds: the MEM1/MEM2 arena Hi/Lo getters and setters and `OSAllocFromMEM1ArenaLo`.
 * RANGE. .text 0x804CC030-0x804CC130 (13 functions); .sdata 0x80793F88-0x80793F90; .sbss 0x80795328-0x80795330.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the four arena words (.sdata 0x80793F88/0x80793F8C,
 *    .sbss 0x80795328/0x8079532C) are read only by these 13 functions.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names throughout; GUESS: the `OSGetMEM1ArenaHi`, `OSGetMEM1ArenaLo`, `OSSetMEM1ArenaHi`, `OSSetMEM2ArenaHi`, `OSSetMEM1ArenaLo`,
 *    `OSSetMEM2ArenaLo` rows (the dump names the
 *    IPC buffer accessors at those addresses; the bodies read and write the arena words).  (`s_mem2ArenaLo`/`s_mem2ArenaHi` are the map's GUESSes).
 * RESIDUALS. `OSAllocFromMEM1ArenaLo` (97.3 %): the pointer takes r5 where the target holds it in r0 and two adds take their
 *    operands in the other order.  The object's .text ends 12 bytes before the claimed end (alignment padding).
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "OS/OSArena.h"

void* __OSArenaLo = (void*)0xFFFFFFFF;
static void* s_mem2ArenaLo = (void*)0xFFFFFFFF;
static void* s_mem2ArenaHi;
void* __OSArenaHi;

/* Returns the upper arena bound. */
/* untyped: raw arena address */
void* OSGetMEM1ArenaHi(void)
{
    return __OSArenaHi;
}

/* Returns the upper arena bound. */
/* untyped: raw arena address */
void* OSGetMEM2ArenaHi(void)
{
    return s_mem2ArenaHi;
}

/* Returns the upper arena bound. */
/* untyped: raw arena address */
void* OSGetArenaHi(void)
{
    return __OSArenaHi;
}

/* Returns the lower arena bound. */
/* untyped: raw arena address */
void* OSGetMEM1ArenaLo(void)
{
    return __OSArenaLo;
}

/* Returns the lower arena bound. */
/* untyped: raw arena address */
void* OSGetMEM2ArenaLo(void)
{
    return s_mem2ArenaLo;
}

/* Returns the lower arena bound. */
/* untyped: raw arena address */
void* OSGetArenaLo(void)
{
    return __OSArenaLo;
}

/* Sets the upper arena bound. */
/* untyped: raw arena address */
void OSSetMEM1ArenaHi(void* newBound)
{
    __OSArenaHi = newBound;
}

/* Sets the upper arena bound. */
/* untyped: raw arena address */
void OSSetMEM2ArenaHi(void* newBound)
{
    s_mem2ArenaHi = newBound;
}

/* Sets the upper arena bound. */
/* untyped: raw arena address */
void OSSetArenaHi(void* newBound)
{
    __OSArenaHi = newBound;
}

/* Sets the lower arena bound. */
/* untyped: raw arena address */
void OSSetMEM1ArenaLo(void* newBound)
{
    __OSArenaLo = newBound;
}

/* Sets the lower arena bound. */
/* untyped: raw arena address */
void OSSetMEM2ArenaLo(void* newBound)
{
    s_mem2ArenaLo = newBound;
}

/* Sets the lower arena bound. */
/* untyped: raw arena address */
void OSSetArenaLo(void* newBound)
{
    __OSArenaLo = newBound;
}

/* Carves `size` bytes aligned to `align` off the low end of the MEM1 arena and returns the block. */
/* untyped: raw arena address */
void* OSAllocFromMEM1ArenaLo(u32 size, u32 align)
{
    u8* arenaLo = (u8*)__OSArenaLo;
    u8* ptr = (u8*)(((u32)arenaLo + align - 1) & ~(align - 1));

    __OSArenaLo = (void*)(((u32)ptr + size + align - 1) & ~(align - 1));
    return ptr;
}
