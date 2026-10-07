/*
 * DB/db.c - the SDK debugger interface (exception destination hook, DBPrintf).
 *
 * RANGE. `.text` 0x804A4AC0-0x804A4BC0; `.data` 0x806184E0-0x806184F8; `.sbss` 0x80795040-0x80795048 (5 functions /
 *   0xE8 B).
 *   - the five functions are the whole debugger-interface run: the exception destination thunk, its aux handler
 *     (OSReport + OSDumpContext + PPCHalt) and the `__DBInterface` accessors
 *   - `.data` 0x806184E0 is the aux handler's format string; `.sbss` `__DBInterface` / `DBVerbose` are read by no
 *     other unit
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the SDK's names; `DBInterface` and its fields are named from their use (`GUESS`: `exceptionMask`,
 *   `exceptionHook`); `__DBExceptionDestination` is a GUESS (the asm tail the exception vector jumps to).
 * RESIDUALS. none in `.text` (`DBPrintf` is empty in retail: its 0x50 bytes are the varargs register-save prologue); `__DBExceptionDestination` is an `asm` function (mfmsr / ori / mtmsr / b).
 */

#include "types.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/PPCHalt.h"

/* The record the OS keeps at 0x80000040 for an attached debugger. */
typedef struct DBInterface {
    /* +0x00 */ u32 bPresent;
    /* +0x04 */ u32 exceptionMask;
    /* +0x08 */ void (*exceptionHook)(void);
    /* +0x0C */ u32 bpAddress;
} DBInterface; /* size: 0x10 */

#define OS_CACHED_BASE 0x80000000
#define OSPhysicalToCached(paddr) ((void*)((u32)(paddr) + OS_CACHED_BASE))

#define OS_CURRENT_CONTEXT_PHYS (*(u32*)0xC0)

/* `.sbss` is laid out in reverse definition order. */
u32 DBVerbose;
DBInterface* __DBInterface;

void __DBExceptionDestination(void);

/* Installs the exception destination hook in the debugger interface record. */
void DBInit(void)
{
    __DBInterface = (DBInterface*)OSPhysicalToCached(0x40);
    __DBInterface->exceptionHook = (void (*)(void))OSPhysicalToCached(__DBExceptionDestination);
    DBVerbose = TRUE;
}

/* Reports the exception, dumps the interrupted context and halts. */
void __DBExceptionDestinationAux(void)
{
    OSContext* context;

    context = (OSContext*)OSPhysicalToCached(OS_CURRENT_CONTEXT_PHYS);
    OSReport("DBExceptionDestination\n");
    OSDumpContext(context);
    PPCHalt();
}

/* Re-enables address translation and enters the aux handler. */
asm void __DBExceptionDestination(void)
{
    nofralloc
    mfmsr r3
    ori r3, r3, 0x30
    mtmsr r3
    b __DBExceptionDestinationAux
}

/* Whether the debugger has marked an exception number. */
BOOL __DBIsExceptionMarked(u8 exception)
{
    u32 mask = 1 << exception;

    return __DBInterface->exceptionMask & mask;
}

/* Writes a debug message (the release build discards it). */
void DBPrintf(const char* format, ...)
{
}
