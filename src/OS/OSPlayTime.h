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

/* 0x804D6EC0 - reads the kind (0 or 1) and the amount of the title's play-time limit; returns 0 on success, an error code otherwise. */
s32 __OSGetPlayTime(ESTicketView* ticketView, s32* limitKind, u32* limitValue);

/* 0x804D6BF0 - writes the expired flag; returns whether it succeeded. */
BOOL __OSWriteExpiredFlag(void);

/* 0x804D6D10 - records that the play time has expired, when it has. */
BOOL __OSWriteExpiredFlagIfSet(void);

/* 0x804D7090 - reads the title's ticket and arms the expiry alarm when it has a time limit. */
void __OSInitPlayTime(void);

#ifdef __cplusplus
}
#endif

#endif
