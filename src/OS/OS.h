/*
 * OS/OS.h - declarations of the symbols owned by `OS/OS.c` that other units call or read.
 */
#ifndef OS_OS_H
#define OS_OS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB2B0 - the exception handler no one replaced; it saves the registers the vector left and reports the exception. */
void OSDefaultExceptionHandler(u8 exception, struct OSContext* context);

/* 0x804CA9C0 - initialises the OS: arenas, exception vectors, interrupts, caches and devices. */
void OSInit(void);

/* 0x804CAF20 - installs the exception vectors and points the handler table at the default handler. */
void OSExceptionInit(void);

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

/* 0x804CB1E0 / 0x804CB200 - sets / reads the handler for CPU exception `exception`; the setter returns the old one. */
typedef void (*OSExceptionHandler)(u8 exception, struct OSContext* context);
OSExceptionHandler __OSSetExceptionHandler(u8 exception, OSExceptionHandler handler);
OSExceptionHandler __OSGetExceptionHandler(u8 exception);

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

/* The MEM2 layout words in low memory: the base and end of the area the IOS hands the title, and the arena bounds saved for the OS. */
#define OS_MEM2_AREA_BASE (*(u32*)0x8000311C)
#define OS_MEM2_AREA_END (*(u32*)0x80003120)
#define OS_SAVED_MEM2_ARENA_LO (*(void**)0x80003124)
#define OS_SAVED_MEM2_ARENA_HI (*(void**)0x80003128)

/* size: 0x1C - the reboot request the previous title left in low memory */
typedef struct OSRebootParams {
    /* +0x00 */ u32 valid;       /* non-zero when a reboot was requested */
    /* +0x04 */ u32 resetCode;
    /* +0x08 */ u8 pad_0x08[4];
    /* +0x0C */ u32 arenaLimit; /* end of the region the previous title left to keep, zero for none */
    /* +0x10 */ u32 arenaResume; /* start of the second region to clear */
    /* +0x14 */ u8 pad_0x14[8];
} OSRebootParams; /* size: 0x1C */

/* 0x8074D280 - the pending reboot request. */
extern OSRebootParams __OSRebootParams;

/* 0x807952D0 - set on arcade (GCAM) hardware. */
extern BOOL __OSIsGcam;

/* 0x80795308 - the system time at which OSInit started. */
extern s64 __OSStartTime;

/* 0x80793F50 - the SDK build string registered with the OS. */
extern const char* __OSVersion;

/* 0x807952CC - set when the title was started from the NAND boot path. */
extern BOOL __OSInNandBoot;

/* The IOS revision and its build date. size: 0x8 */
typedef struct OSIOSRev {
    /* +0x00 */ u8 platform;
    /* +0x01 */ u8 major;
    /* +0x02 */ u8 minor;
    /* +0x03 */ u8 micro;
    /* +0x04 */ u8 month;
    /* +0x05 */ u8 day;
    /* +0x06 */ u16 year;
} OSIOSRev;

/* 0x804CA030 - reads the IOS revision. */
void __OSGetIOSRev(OSIOSRev* rev);

/* 0x807952D8 - the exception handler table the vectors dispatch through. */
extern OSExceptionHandler* OSExceptionTable;

#ifdef __cplusplus
}
#endif

#endif
