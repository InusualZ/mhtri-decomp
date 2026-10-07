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

/* 0x804CB420 - which kind of title is running (the SDK's `OS_APP_TYPE_*` values). */
u8 OSGetAppType(void);

/* 0x804CB390 - the 4-character game code of the running title, copied into a static buffer. */
char* OSGetAppGamename(void);

/* 0x800000F8 - the console's bus clock in Hz, read straight out of the low-memory arena.  The original
 * object carries no relocation for it, i.e. the source spelled the address out (same shape as
 * `NWC24_RTC_USER_ID` in `unsplit/NWC24.h`). */
#define OS_BUS_CLOCK (*(u32*)0x800000F8)

#ifdef __cplusplus
}
#endif

#endif
