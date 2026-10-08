/*
 * OS/OSExec.h - declarations of the symbols owned by `OS/OSExec.c` that other units call or read.
 */
#ifndef OS_OSEXEC_H
#define OS_OSEXEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSExecArgPage;

/* size: 0x1C - the block the previous title left for this one in MEM1. */
typedef struct OSExecParams {
    /* +0x00 */ u32 valid;                 /* zero when no block was left */
    /* +0x04 */ u32 resetCode;
    /* +0x08 */ u32 dolOffset;             /* where the booted DOL starts on the disc (in 4-byte units) */
    /* +0x0C */ u8* saveStart;             /* start of the region the next title must not clear */
    /* +0x10 */ u8* saveEnd;
    /* +0x14 */ u32 flags;                 /* zero when the arguments were packed into `argPage` */
    /* +0x18 */ struct OSExecArgPage* argPage;
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
