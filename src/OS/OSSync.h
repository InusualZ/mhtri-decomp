/*
 * OS/OSSync.h - declarations of the symbols owned by `OS/OSSync.c` that other units call or read.
 */
#ifndef OS_OSSYNC_H
#define OS_OSSYNC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D3660 - installs the system-call vector at 0x80000C00. */
void __OSInitSystemCall(void);

/* 0x804D36C0 - an empty function. */
void DBClose(void);

#ifdef __cplusplus
}
#endif

#endif
