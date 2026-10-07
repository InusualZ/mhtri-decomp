/*
 * OS/OSExec.h - declarations of the symbols owned by `OS/OSExec.c` that other units call or read.
 */
#ifndef OS_OSEXEC_H
#define OS_OSEXEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80795330 - the boot-kind word `__OSBootDolSimple` and `__OSLaunchNextFirmware` compare against the one at 0x80003194. NAME: a GUESS. */
extern u32 __OSBootFlag;

/* 0x804CF170 - boots the DOL at `bootDol`, passing `resetCode` and the argument vector. */
void __OSBootDol(u32 bootDol, u32 resetCode, const char** argv);

#ifdef __cplusplus
}
#endif

#endif
