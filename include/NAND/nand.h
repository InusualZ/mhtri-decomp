/*
 * NAND/nand.h - declarations of the symbols owned by `NAND/nand.c` that other units call or read.
 */
#ifndef NAND_NAND_H
#define NAND_NAND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSPanic(const char* file, int line, const char* msg, ...);

BOOL OSDisableInterrupts(void);

void OSRestoreInterrupts(BOOL level);

/* untyped: opaque band object, typed by the callers' views */
void OSInitMutex(void* mutex);

/* Foreign OS mutex API (unowned in the map; bare prototypes, rule 2 shared-file request in the outbox). */
/* untyped: opaque band object, typed by the callers' views */
void OSLockMutex(void* mutex);

/* untyped: opaque band object, typed by the callers' views */
void OSUnlockMutex(void* mutex);

void OSReport(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
