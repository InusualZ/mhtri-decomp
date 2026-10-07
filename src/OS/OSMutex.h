/*
 * OS/OSMutex.h - declarations of the symbols owned by `OS/OSMutex.c` that other units call or read.
 */
#ifndef OS_OSMUTEX_H
#define OS_OSMUTEX_H

#include "types.h"
#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x18 - the OS mutex the NHTTP and NWC24 bands initialise, lock and unlock */
typedef struct OSMutex {
    /* +0x00 */ u8 pad_0x00[0x18];
} OSMutex; /* size: 0x18 */

/* untyped: opaque band object, typed by the callers' views */
void OSInitMutex(void* mutex);

/* 0x804D1F20 / 0x804D2000 (with `OSInitMutex` 0x804D1EE0 above) - the OS mutex set. */
/* untyped: opaque band object, typed by the callers' views */
void OSLockMutex(void* mutex);

/* untyped: opaque band object, typed by the callers' views */
void OSUnlockMutex(void* mutex);

/* 0x804D2140 / 0x804D2150 - the 4-byte branch stubs the NHTTP completion record reaches
 * `OSInitThreadQueue` (0x804D3960) and the wakeup routine (0x804D4B20, the dump's `OSWakeupThread`)
 * through.  Their names are GUESSes from the branch each one holds. */
void OSInitThreadQueueThunk(OSThreadQueue* queue);
void OSWakeupThreadThunk(OSThreadQueue* queue);

#ifdef __cplusplus
}
#endif

#endif
