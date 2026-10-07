/*
 * OS/OSPlayTime.h - declarations of the symbols owned by `OS/OSPlayTime.c` that other units call or read.
 */
#ifndef OS_OSPLAYTIME_H
#define OS_OSPLAYTIME_H

#include "types.h"
#include "ESP/esp.h"
#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D6A10 - whether the running title's play time is limited. */
BOOL OSPlayTimeIsLimited(void);

/* 0x804D6EC0 - reads the title's played and remaining time. */
void __OSGetPlayTime(ESTicketView* ticketView, s32* played, s32* remaining);

/* 0x804D6D10 - records that the play time has expired, when it has. */
void __OSWriteExpiredFlagIfSet(void);

#ifdef __cplusplus
}
#endif

#endif
