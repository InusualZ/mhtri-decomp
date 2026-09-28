/*
 * OS declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The Revolution SDK OS library's bootstrap helpers `OSRegisterVersion` (0x804CB380),
 * `OSDisableInterrupts` (0x804D0C70) and `OSRestoreInterrupts` (0x804D0CB0) are called by units whose
 * own ranges the OS module's registered units do not cover: `NWC24/nwc24_msg.c` registers the NWC24
 * library's version string through `OSRegisterVersion` and brackets its message-library state update
 * with the interrupt pair.  The module the band names is `OS` (its bracketing registered units are the
 * OS band's), and no registered unit defines any of the three, so this file is their home.
 *
 * Added with the `NWC24/nwc24_msg.c` registration (the NWC24 SDK band, 0x8051D710-0x8051E864).
 */
#ifndef MHTRI_UNSPLIT_OS_H
#define MHTRI_UNSPLIT_OS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB380 - records a library's version string with the OS. */
void OSRegisterVersion(const char* version);

/* 0x804D0C70 / 0x804D0CB0 - disable interrupts, returning the previous state; restore it. */
BOOL OSDisableInterrupts(void);
void OSRestoreInterrupts(BOOL level);

/* 0x804D1EE0 / 0x804D1F20 - the OS mutex pair `NHTTP/NHTTP_bgnend.c` initialises and locks its
 * request-list mutex with (0x804D2000, `OSUnlockMutex`, is the third of the set; it is declared
 * locally by `OS/FindContainHeap_.c` and in `include/unsplit/Network.h` and is not needed here, so
 * it is not restated).  Neither address is inside a registered unit, and the nearest registered
 * ranges below and above name different modules (`OS/OSAlarm.c` / `RSO/runtime.c`), so stylelint's
 * rule 2 reports them as an unplaceable gap - the module the band names is `OS`, which is this file. */
void OSInitMutex(void* mutex);
void OSLockMutex(void* mutex);

/* ----------------------------------------------------------------------------------------------
 * The OS thread and message-queue set the NHTTP library creates its comm thread with
 * (`NHTTP/NHTTP_bgnend.c` -> `OSInitMessageQueue` / `OSCreateThread` / `OSResumeThread` /
 * `OSJoinThread` / `OSSendMessage` / `OSReceiveMessage`), plus the two diagnostics NHTTPi_Startup
 * and NHTTPi_CleanupAsync print through.  None of these addresses is inside a registered unit
 * (0x804D1460-0x804D4600 is the OS library's own band), so rule 2 puts them here. */

/* size: 0x20 - two thread queues, the message-array pointer, the count and the two ring indices */
typedef struct OSMessageQueue { u8 pad_0x00[0x20]; } OSMessageQueue;

/* size: 0x08 - the queued message and its priority */
typedef struct OSMessage { void* msg; s32 prio; } OSMessage;

/* size: 0x318 - the OS thread control block (`NHTTPThreadInfo` places one at +0x30) */
typedef struct OSThread { u8 pad_0x000[0x318]; } OSThread;

/* size: 0x18 - the OS mutex the NHTTP and NWC24 bands initialise, lock and unlock */
typedef struct OSMutex { u8 pad_0x00[0x18]; } OSMutex;

/* 0x804D3C00 - the thread the OS is currently running, null before the scheduler starts. */
OSThread* OSGetCurrentThread(void);

/* 0x804D4350 - write a cached range back to main memory. */
void DCStoreRange(void* start, u32 size); /* untyped: byte range */

/* the entry point OSCreateThread is handed: the thread's own argument in, its exit value out */
typedef void* (*OSThreadEntry)(void* arg); /* untyped: opaque handle */

#define OS_MESSAGE_NOBLOCK 0
#define OS_MESSAGE_BLOCK   1

/* 0x804D1460 - initialise a message queue over a caller-owned array. */
void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* msgArray, s32 msgCount);

/* 0x804D14C0 / 0x804D1590 - post to / take from a queue; the block flag is OS_MESSAGE_BLOCK or
 * OS_MESSAGE_NOBLOCK. */
BOOL OSSendMessage(OSMessageQueue* queue, OSMessage* msg, BOOL block);
BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* buffer, BOOL block);

/* 0x804D3C00 - the thread the OS is currently running, null before the scheduler starts. */
OSThread* OSGetCurrentThread(void);

/* 0x804D3F70 / 0x804D4600 / 0x804D44B0 - create a thread (its stack is `stack`, growing down from
 * there, `stackSize` bytes), start it and wait for it to exit.  The thread argument is handed
 * straight to `entry` and is the caller's payload, so it stays untyped here. */
/* untyped: caller-owned payload */
OSThread* OSCreateThread(OSThread* thread, OSThreadEntry entry, void* arg, u8* stack, u32 stackSize,
                         s32 priority, u16 attribute);
s32 OSResumeThread(OSThread* thread);
BOOL OSJoinThread(OSThread* thread, s32* exitValue);

/* 0x804CD620 / 0x804CD6B0 - the OS diagnostics `NHTTPi_Startup`/`NHTTPi_CleanupAsync` report a
 * failed IP configuration through (`OSPanic` takes the source file, the line and the message). */
void OSReport(const char* format, ...);
void OSPanic(const char* file, s32 line, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_OS_H */
