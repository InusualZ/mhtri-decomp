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
typedef struct OSMutexLink {
    /* +0x00 */ struct OSMutex* next;
    /* +0x04 */ struct OSMutex* prev;
} OSMutexLink; /* size: 0x08 */

typedef struct OSMutex {
    /* +0x00 */ OSThreadQueue queue;  /* threads waiting for the mutex */
    /* +0x08 */ OSThread* thread;     /* the owner */
    /* +0x0C */ s32 count;            /* recursive lock count */
    /* +0x10 */ OSMutexLink link;     /* its node in the owner's mutex queue */
} OSMutex; /* size: 0x18 */

/* 0x804D1EE0 / 0x804D1F20 / 0x804D2000 - the OS mutex set.  The callers outside the OS core pass their own spellings of
 * the mutex record, so the public entries stay untyped for them; `OS/OSMutex.c` defines `OS_MUTEX_TYPED_API` to see the
 * typed signatures it implements. */
#ifdef OS_MUTEX_TYPED_API
void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
#else
/* untyped: opaque band object, typed by the callers' views */
void OSInitMutex(void* mutex);
/* untyped: opaque band object, typed by the callers' views */
void OSLockMutex(void* mutex);
/* untyped: opaque band object, typed by the callers' views */
void OSUnlockMutex(void* mutex);
#endif

/* 0x804D20D0 - releases every mutex a terminating thread still holds. */
void __OSUnlockAllMutex(OSThread* thread);

/* 0x804D2140 / 0x804D2150 - the 4-byte branch stubs the NHTTP completion record reaches
 * `OSInitThreadQueue` (0x804D3960) and the wakeup routine (0x804D4B20, the dump's `OSWakeupThread`)
 * through.  Their names are GUESSes from the branch each one holds. */
void OSInitThreadQueueThunk(OSThreadQueue* queue);
void OSWakeupThreadThunk(OSThreadQueue* queue);

#ifdef __cplusplus
}
#endif

#endif
