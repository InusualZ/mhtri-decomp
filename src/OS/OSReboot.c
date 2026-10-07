/*
 * OS/OSReboot.c - the OS reboot support: `__OSReboot` and the save-region getter.
 * RANGE. .text 0x804D2160-0x804D21F0 (2 functions); .sbss 0x80795378-0x80795380.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: `OSGetSaveRegion` reads .sbss 0x80795378/0x8079537C (no other reader);
 *    `OSRegisterShutdownFunction` (0x804D21F0) reads the shutdown queue of OSReset.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names; GUESS: `SaveStart`/`SaveEnd` (the two words the getter copies out), `__OSBootFlag` (0x80795330, owned by OSExec).
 * RESIDUALS. none recorded yet.
 * SHAPES. plain C.
 */

#include "types.h"

#include "OS/OSArena.h"
#include "OS/OSExec.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReboot.h"

#define OS_BOOT_FLAG_SOURCE (*(u32*)0x80003194)

static void* SaveEnd;
static void* SaveStart;

/* Resets the arena bounds, records the boot flag and boots the DOL; does not return. */
void __OSReboot(u32 resetCode, u32 bootDol)
{
    const char* argv;

    OSDisableInterrupts();
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    argv = NULL;
    __OSBootFlag = OS_BOOT_FLAG_SOURCE;
    __OSBootDol(bootDol, resetCode | 0x80000000, &argv);
}

/* Returns the saved region's bounds. */
/* untyped: raw region bounds */
void OSGetSaveRegion(void** start, void** end)
{
    *start = SaveStart;
    *end = SaveEnd;
}
