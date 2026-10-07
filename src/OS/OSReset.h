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

/* 0x80795384 - set while a return-to-menu is in progress; the Wii remote driver's shutdown callback reads it for event 5. NAME: a GUESS. */
extern s32 OSReturnToMenuPending;

#ifdef __cplusplus
}
#endif

#endif
