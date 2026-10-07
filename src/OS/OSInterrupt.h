/*
 * OS/OSInterrupt.h - declarations of the symbols owned by `OS/OSInterrupt.c` that other units call or read.
 */
#ifndef OS_OSINTERRUPT_H
#define OS_OSINTERRUPT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL OSDisableInterrupts(void);

void OSRestoreInterrupts(BOOL level);

#ifdef __cplusplus
}
#endif

#endif
