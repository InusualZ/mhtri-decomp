/*
 * OS/OSThread.h - declarations of the symbols owned by `OS/OSThread.c` that other units call or read.
 */
#ifndef OS_OSTHREAD_H
#define OS_OSTHREAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x318 - the OS thread control block (`NHTTPThreadInfo` places one at +0x30) */
typedef struct OSThread {
    /* +0x000 */ u8 pad_0x000[0x318];
} OSThread; /* size: 0x318 */

/* size: 0x08 - the head and tail of a queue of waiting threads */
typedef struct OSThreadQueue {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
} OSThreadQueue; /* size: 0x08 */

/* the entry point OSCreateThread is handed: the thread's own argument in, its exit value out */
typedef void* (*OSThreadEntry)(void* arg); /* untyped: opaque handle */

/* 0x804D3F70 / 0x804D4600 - create a thread on `stack` (growing down, `stackSize` bytes) that runs `entry(param)`,
 * and start it; non-zero on success.  The callers pass their own spellings of the thread record (an OS thread
 * block, a `u8` array) and of the entry, so the parameters are still untyped. */
s32 OSCreateThread(void* thread, void* entry, void* param, void* stack, u32 stackSize, s32 priority, u32 flags);
s32 OSResumeThread(void* thread);
/* untyped: opaque handle passed through - the OS thread record */
s32 OSIsThreadTerminated(void* thread);

/* 0x804D4CA0 - sleep the calling thread for `ticks` time-base ticks. */
void OSSleepTicks(u64 ticks);

/* 0x804D39B0 - stops thread switching; returns the previous suspend count. */
s32 OSDisableScheduler(void);

/* 0x804D3F30 - yields the processor to another ready thread. */
void OSYieldThread(void);

/* 0x804D3C00 - the thread the OS is currently running, null before the scheduler starts. */
OSThread* OSGetCurrentThread(void);

/* 0x804D44B0 - wait for the thread to exit (`OSCreateThread`/`OSResumeThread` are above). */
BOOL OSJoinThread(OSThread* thread, s32* exitValue);

#ifdef __cplusplus
}
#endif

#endif
