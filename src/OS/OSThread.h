/*
 * OS/OSThread.h - declarations of the symbols owned by `OS/OSThread.c` that other units call or read.
 */
#ifndef OS_OSTHREAD_H
#define OS_OSTHREAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSThread;
struct OSMutex;

/* size: 0x08 - the head and tail of a queue of waiting threads */
typedef struct OSThreadQueue {
    /* +0x00 */ struct OSThread* head;
    /* +0x04 */ struct OSThread* tail;
} OSThreadQueue; /* size: 0x08 */

/* size: 0x08 - the head and tail of the mutexes a thread holds */
typedef struct OSMutexQueue {
    /* +0x00 */ struct OSMutex* head;
    /* +0x04 */ struct OSMutex* tail;
} OSMutexQueue; /* size: 0x08 */

/* size: 0x08 - a node in a queue of threads */
typedef struct OSThreadLink {
    /* +0x00 */ struct OSThread* next;
    /* +0x04 */ struct OSThread* prev;
} OSThreadLink; /* size: 0x08 */

/* size: 0x318 - the OS thread control block (`NHTTPThreadInfo` places one at +0x30) */
typedef struct OSThread {
    /* +0x000 */ u8 pad_0x000[0x2C8];        /* the saved CPU context (OSContext) */
    /* +0x2C8 */ u16 state;                  /* ready, running, waiting or moribund */
    /* +0x2CA */ u16 attr;                   /* detached flag */
    /* +0x2CC */ s32 suspend;                /* suspend count */
    /* +0x2D0 */ s32 priority;               /* effective priority */
    /* +0x2D4 */ s32 base;                   /* base priority */
    /* +0x2D8 */ void* val;                  /* exit value */
    /* +0x2DC */ OSThreadQueue* queue;       /* the queue the thread sleeps on */
    /* +0x2E0 */ OSThreadLink link;          /* its node in that queue */
    /* +0x2E8 */ OSThreadQueue queueJoin;    /* threads waiting to join it */
    /* +0x2F0 */ struct OSMutex* mutex;      /* the mutex it waits for */
    /* +0x2F4 */ OSMutexQueue queueMutex;    /* the mutexes it owns */
    /* +0x2FC */ OSThreadLink linkActive;    /* its node in the active-thread list */
    /* +0x304 */ u8* stackBase;              /* highest stack address */
    /* +0x308 */ u8* stackEnd;               /* lowest stack address (the canary) */
    /* +0x30C */ s32 error;                  /* last error */
    /* +0x310 */ void* specific[2];          /* per-thread storage */
} OSThread; /* size: 0x318 */

/* the entry point OSCreateThread is handed: the thread's own argument in, its exit value out */
typedef void* (*OSThreadEntry)(void* arg); /* untyped: opaque handle */

/* 0x804D3F70 / 0x804D4600 - create a thread on `stack` (growing down, `stackSize` bytes) that runs `entry(param)`,
 * and start it; non-zero on success.  The callers pass their own spellings of the thread record (an OS thread
 * block, a `u8` array) and of the entry, so the parameters are still untyped. */
s32 OSCreateThread(void* thread, void* entry, void* param, void* stack, u32 stackSize, s32 priority, u32 flags);
s32 OSResumeThread(void* thread);
/* untyped: opaque handle passed through - the OS thread record */
s32 OSIsThreadTerminated(void* thread);

/* 0x804D3960 - empties a thread queue. */
void OSInitThreadQueue(OSThreadQueue* queue);

/* 0x804D4A30 - puts the calling thread to sleep on `queue`. */
void OSSleepThread(OSThreadQueue* queue);

/* 0x804D4B20 - makes every thread sleeping on `queue` runnable. */
void OSWakeupThread(OSThreadQueue* queue);

/* 0x804D4CA0 - sleep the calling thread for `ticks` time-base ticks. */
void OSSleepTicks(u64 ticks);

/* 0x804D39B0 - stops thread switching; returns the previous suspend count. */
s32 OSDisableScheduler(void);

/* 0x804D3F30 - yields the processor to another ready thread. */
void OSYieldThread(void);

/* 0x804D3AA0 / 0x804D3C90 - a thread's priority after mutex inheritance / pushes a priority up the thread it waits on. */
s32 __OSGetEffectivePriority(OSThread* thread);
void __OSPromoteThread(OSThread* thread, s32 priority);

/* 0x804D3C00 - the thread the OS is currently running, null before the scheduler starts. */
OSThread* OSGetCurrentThread(void);

/* 0x804D44B0 - wait for the thread to exit (`OSCreateThread`/`OSResumeThread` are above). */
BOOL OSJoinThread(OSThread* thread, s32* exitValue);

#ifdef __cplusplus
}
#endif

#endif
