/*
 * OS/OSSync.c - the OS system-call vector and its installer, `__OSInitSystemCall`.
 * RANGE. .text 0x804D3640-0x804D36D0 (3 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `__OSSystemCallVectorStart`/`End` labels bracket the 0x20-byte vector body at 0x804D3640 and
 *    `__OSInitSystemCall` copies it; a 4-byte `blr` follows; `__OSThreadInit` (0x804D36D0) opens the thread unit.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `__OSInitSystemCall`, `__OSSystemCallVectorStart`/`End` are the map's names; GUESS: `SystemCallVector` (the SDK's
 *    static name of the vector body) and `DBClose` (the dump's name for the bare `blr`; a signature match on a 4-byte body,
 *    not further evidence).
 * RESIDUALS. the object's .text ends 12 bytes before the claimed end (alignment padding).
 * SHAPES. the vector is an asm function with `nofralloc` that defines the two map labels with `entry` (playbook 104).
 */

#include "types.h"

#include "OS/DCInvalidateRange.h"
#include "OS/OSCache.h"
#include "OS/OSSync.h"
#include "Runtime.PPCEABI.H/memcpy.h"

void __OSSystemCallVectorStart(void);
void __OSSystemCallVectorEnd(void);

/* The system-call exception vector: enables the instruction cache bypass bit, then returns from the interrupt. */
__declspec(export) asm void SystemCallVector(void)
{
    nofralloc
    entry __OSSystemCallVectorStart
    mfspr r9, HID0
    ori r10, r9, 8
    mtspr HID0, r10
    isync
    sync
    mtspr HID0, r9
    rfi
    entry __OSSystemCallVectorEnd
    nop
}

/* Copies the system-call vector to 0x80000C00 and makes it visible to the instruction fetch. */
void __OSInitSystemCall(void)
{
    memcpy((void*)0x80000C00, (void*)__OSSystemCallVectorStart,
           (u32)__OSSystemCallVectorEnd - (u32)__OSSystemCallVectorStart);
    DCFlushRangeNoSync((void*)0x80000C00, 0x100);
    __sync();
    ICInvalidateRange((void*)0x80000C00, 0x100);
}

/* Does nothing. */
void DBClose(void)
{
}
