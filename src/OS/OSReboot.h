/*
 * OS/OSReboot.h - declarations of the symbols owned by `OS/OSReboot.c` that other units call or read.
 */
#ifndef OS_OSREBOOT_H
#define OS_OSREBOOT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D2160 - reboots into the DOL at `bootDol` with `resetCode`; does not return. */
void __OSReboot(u32 resetCode, u32 bootDol);

/* 0x804D21D0 - copies the save region's bounds out. */
/* untyped: raw region bounds */
void OSGetSaveRegion(void** start, void** end);

#ifdef __cplusplus
}
#endif

#endif
