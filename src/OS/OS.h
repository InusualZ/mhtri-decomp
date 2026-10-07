/*
 * OS/OS.h - declarations of the symbols owned by `OS/OS.c` that other units call or read.
 */
#ifndef OS_OS_H
#define OS_OS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB380 - records a library's version string with the OS. */
void OSRegisterVersion(const char* version);

/* 0x804CA0A0 - the console type word (the top nibble selects retail / development hardware). */
u32 OSGetConsoleType(void);

/* 0x804CB370 - the DI configuration byte (0xFF when no drive configuration was reported). */
u32 __OSGetDIConfig(void);

/* 0x807952C8 - set when the title was started from the IPL. */
extern BOOL __OSInIPL;

/* 0x804CB420 - which kind of title is running (the SDK's `OS_APP_TYPE_*` values). */
u8 OSGetAppType(void);

/* 0x804CB390 - the 4-character game code of the running title, copied into a static buffer. */
char* OSGetAppGamename(void);

/* 0x800000F8 - the console's bus clock in Hz, read straight out of the low-memory arena.  The original
 * object carries no relocation for it, i.e. the source spelled the address out (same shape as
 * `NWC24_RTC_USER_ID` in `unsplit/NWC24.h`). */
#define OS_BUS_CLOCK (*(u32*)0x800000F8)

/* The low-memory words the OS boot code and the module loader share (the original object carries no relocation for
 * them, so the source spelled the addresses out). */
typedef struct OSModuleQueue {
    void* head; /* +0x00 */
    void* tail; /* +0x04 */
} OSModuleQueue; /* size: 0x08 */

#define OS_MODULE_QUEUE (*(OSModuleQueue*)0x800030C8)
#define OS_STRING_TABLE (*(void**)0x800030D0)
#define OS_IPC_BUFFER_LO (*(void**)0x80003130)
#define OS_IPC_BUFFER_HI (*(void**)0x80003134)

#ifdef __cplusplus
}
#endif

#endif
