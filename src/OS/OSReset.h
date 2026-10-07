/*
 * OS/OSReset.h - declarations of the symbols owned by `OS/OSReset.c` that other units call or read.
 */
#ifndef OS_OSRESET_H
#define OS_OSRESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x10 - the record `OSRegisterShutdownFunction` links into the OS's shutdown list; the
 * library fills `func` and `priority`, the OS owns the two list links. */
typedef struct OSShutdownFunctionInfo {
    /* +0x00 */ BOOL (*func)(BOOL final, u32 event);
    /* +0x04 */ u32 priority;
    /* +0x08 */ struct OSShutdownFunctionInfo* next;
    /* +0x0C */ struct OSShutdownFunctionInfo* prev;
} OSShutdownFunctionInfo; /* size: 0x10 */

/* 0x804D21F0 - add a shutdown-function record to the OS's ordered list. */
void OSRegisterShutdownFunction(OSShutdownFunctionInfo* info);

/* 0x804D2280 / 0x804D2330 - run the shutdown functions of `event` / shut every device down for it. */
BOOL __OSCallShutdownFunctions(BOOL final, u32 event);
void __OSShutdownDevices(u32 event);

/* 0x804D24B0 - the disc state (1 ready, 2 inserted, 3 cover open) the state-flags record keeps. */
u32 __OSGetDiscState(u32 discState);

/* 0x804D2520 - shuts the console down to standby, or to the system menu when the idle mode asks for it. */
void OSShutdownSystem(void);

/* 0x804D2AE0 - hot-resets the console; a failure is fatal. */
void __OSDoHotReset(void);

/* 0x804D2640 - restarts the title with `resetCode`. */
void OSRestart(u32 resetCode);

/* 0x804D2720 / 0x804D29B0 / 0x804D29F0 / 0x804D2A30 - return to the system menu / the data manager / after an error. */
void __OSReturnToMenu(u8 returnToMenu);
void OSReturnToMenu(void);
void OSReturnToDataManager(void);
void __OSReturnToMenuForError(void);

/* 0x804D2B40 - the reset code of the pending reboot, else the hardware's. */
u32 OSGetResetCode(void);

/* 0x804D2B70 - resets or restarts the system. */
void OSResetSystem(s32 reset, u32 resetCode, s32 forceMenu);

/* 0x80795384 - set while a return-to-menu is in progress; the Wii remote driver's shutdown callback reads it for event 5. NAME: a GUESS. */
extern s32 OSReturnToMenuPending;

#ifdef __cplusplus
}
#endif

#endif
