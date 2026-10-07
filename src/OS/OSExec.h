/*
 * OS/OSExec.h - declarations of the symbols owned by `OS/OSExec.c` that other units call or read.
 */
#ifndef OS_OSEXEC_H
#define OS_OSEXEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x1C - the block the previous title left for this one in MEM1. */
typedef struct OSExecParams {
    /* +0x00 */ u32 valid; /* zero when no block was left */
    /* +0x04 */ u8 pad_0x04[0x18];
} OSExecParams; /* size: 0x1C */

/* 0x804CE270 - copies the exec parameter block, or clears `valid`. */
void __OSGetExecParams(OSExecParams* params);

/* 0x80795330 - the boot-kind word `__OSBootDolSimple` and `__OSLaunchNextFirmware` compare against the one at 0x80003194. NAME: a GUESS. */
extern u32 __OSBootFlag;

/* 0x804CF170 - boots the DOL at `bootDol`, passing `resetCode` and the argument vector. */
void __OSBootDol(u32 bootDol, u32 resetCode, const char** argv);

/* 0x8079533C - set while the OS is rebooting into another DOL. */
extern BOOL __OSInReboot;

/* 0x804CE940 - launches the system menu; returns only on failure. */
void __OSLaunchMenu(void);

#ifdef __cplusplus
}
#endif

#endif
