/*
 * OS/OSExec.c - the OS program launcher: argument packing, the exec parameters, `__OSLaunchNextFirmware`,
 *    `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`.
 * RANGE. .text 0x804CDD70-0x804CF350 (11 functions); .data 0x8061C850-0x8061C8C0; .bss 0x8074D340-0x8074D360; .sdata
 *    0x80793F98-0x80793FA8; .sbss 0x80795330-0x80795348.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: .data 0x8061C850 ("OSExec(): Failed to exec") and 0x8061C8B0 (the apploader date) are read by
 *    `__OSLaunchNextFirmware` and `__OSBootDolSimple`; the first four functions are argument-packing helpers
 *    (`strlen`/`strcpy`/`memset`) whose twin heads the OSLaunch unit at 0x804D71F0.
 * FLAGS. `cflags_base` (configure.py), the default of the OS band.
 * NAMES. `OSExecPackArgs`, `OSExecJump`, `OSExecSetReady`, `OSExecSetState`, `OSExecReady`, `OSExecState`, `OSExecParams`
 *    are GUESSes (the argument packer twins `OSLaunchPackArgs`; the jump flushes the icache and branches; the two
 *    setters store one `.sbss` word each).  `__OSLaunchNextFirmware`, `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`, `__OSGetExecParams` are the
 *    map's names; the left edge (0x804CDD70) is medium evidence (no data read by the four helpers).
 * RESIDUALS. unwritten: 0x804CDEF0-0x804CE050 (2 functions, 0x1E8 B), `__OSLaunchNextFirmware`, `__OSLaunchMenu`, `__OSBootDolSimple`,
 *    `__OSBootDol` (0x804CE2B0-0x804CF350); .bss, .data, .sdata and the rest of .sbss are not emitted yet.
 * RESIDUALS (cont.). `OSExecPackArgs` differs only in the register numbering of its locals; `OSExecJump` swaps two
 *    epilogue loads after the `bctr`.
 * SHAPES. `OSExecJump` flushes the instruction cache and branches with inline asm.
 */

#include "types.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"
#include "OS/OSCache.h"
#include "OS/OSExec.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The exec parameter block's address in low memory, valid once it points into MEM1. */
#define OS_EXEC_PARAMS_ADDR (*(u32*)0x800030F0)

/* size: 0x2000 - the argument page the launched title reads. */
typedef struct OSExecArgPage {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 argsOffset; /* offset of the argc word, zero when there are no arguments */
    /* +0x0C */ u8 data[0x1FF4];
} OSExecArgPage; /* size: 0x2000 */

u32 OSExecReady;
u32 OSExecState;

/* 0x804CDD70 (0x180): packs `argc` strings of `argv` at the end of `page`, followed by the argv table and argc. */
BOOL OSExecPackArgs(OSExecArgPage* page, s32 argc, char** argv)
{
    u8* base = (u8*)page;
    u32* table;
    char** arg;
    char* str;
    u8* cursor;
    u32 count = argc;
    u32 j;

    memset(page, 0, sizeof(OSExecArgPage));
    if (argc == 0) {
        page->argsOffset = 0;
    } else {
        cursor = base + sizeof(OSExecArgPage);
        arg = argv + argc;
        while (arg--, --argc >= 0) {
            str = *arg;
            cursor -= strlen(str) + 1;
            strcpy((char*)cursor, str);
            *arg = (char*)(cursor - base);
        }
        table = (u32*)(base + (((u32)(cursor - base)) & ~3)) - (count + 1);
        for (j = 0; j < count + 1; j++) {
            table[j] = (u32)argv[j];
        }
        table[-1] = count;
        page->argsOffset = (u32)(table - 1) - (u32)base;
    }
    return TRUE;
}

/* 0x804CE220 (0x3C): flushes the instruction cache and jumps to `entry`. */
void OSExecJump(register u32 entry)
{
    ICFlashInvalidate();
    asm {
        sync
        isync
        mtctr entry
        bctr
    }
}

/* 0x804CE260 (0xC): marks the exec state ready. */
void OSExecSetReady(void)
{
    OSExecReady = 1;
}

/* 0x804CE270 (0x24): copies the exec parameter block out of low memory, or clears its valid word when none was left. */
void __OSGetExecParams(OSExecParams* params)
{
    u32 addr = OS_EXEC_PARAMS_ADDR;

    if (addr >= 0x80000000) {
        memcpy(params, (void*)addr, sizeof(OSExecParams));
    } else {
        params->valid = 0;
    }
}

/* 0x804CE2A0 (0x8): stores the exec state word. */
void OSExecSetState(u32 state)
{
    OSExecState = state;
}
