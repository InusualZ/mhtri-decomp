/*
 * OS/OSStateTM.h - declarations of the symbols owned by `OS/OSStateTM.c` that other units call or read.
 */
#ifndef OS_OSSTATETM_H
#define OS_OSSTATETM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D5890 - starts the STM (state/thermal) channel that reset and shutdown requests go through. */
void __OSInitSTM(void);

/* 0x804D59B0 - powers the console down to standby; does not return. */
void __OSShutdownToSBY(void);

/* 0x804D5A30 - requests a hot reset; returns only on failure. */
void __OSHotReset(void);

/* 0x804D5BF0 - removes the state-event handler. */
void __OSUnRegisterStateEvent(void);

#ifdef __cplusplus
}
#endif

#endif
