/*
 * OS/OSPlayRecord.h - declarations of the symbols owned by `OS/OSPlayRecord.c` that other units call or read.
 */
#ifndef OS_OSPLAYRECORD_H
#define OS_OSPLAYRECORD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D6330 - stops the play-time recording. */
void __OSStopPlayRecord(void);

/* 0x804D62D0 - starts the periodic play-record save alarm. */
void __OSStartPlayRecord(void);

#ifdef __cplusplus
}
#endif

#endif
