/*
 * SO/soi.h - declarations of the symbols owned by `SO/soi.cpp` that other units call or read.  It
 * includes `SO/socket.h`, the SOCKET library's own header, so the SDK's consumers reach both halves.
 */
#ifndef SO_SOI_H
#define SO_SOI_H

#include "types.h"
#include "SO/socket.h"

/* Declarations moved here from `unsplit/DWCi.h, SO.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051F8FC - open a socket (domain 2 = AF_INET, type 1 = stream); the descriptor, or a negative error. */
s32 __SOCreateSocket(s32 domain, s32 type, s32 protocol);

/* The SO library's allocator pair (the SDK's `SOLibraryConfig`): `SOInit` is handed the block's address.
 * size: 0x08 - the two words `sNetworkLibraryWii::init` tests for NULL before the call. */
typedef void* (*SOAllocFunc)(u32 name, s32 size); /* untyped: caller-owned payload */
typedef void (*SOFreeFunc)(u32 name, void* ptr, s32 size); /* untyped: caller-owned payload */
typedef struct SOLibraryConfig {
    /* +0x00 */ SOAllocFunc alloc;
    /* +0x04 */ SOFreeFunc free;
} SOLibraryConfig;

/* 0x8051E864 / 0x8051EA30 - bring the SO library up with the allocator pair / take it down; -7 when it
 * already was (the strings `sNetworkLibraryWii::init`/`final` log next to each call). */
s32 SOInit(const SOLibraryConfig* config);

s32 SOFinish(void);

/* 0x8051EB2C / 0x8051EF60 - start / stop the network interface; -7 when already done (the worker-thread
 * bodies of `sNetworkLibraryWii::start`/`stop`). */
s32 SOStartup(void);

s32 SOCleanup(void);

/* 0x8051F110 (GUESS) - the current thread's error word (OSThread +0x30C), or the library's own word when no
 * thread is running. */
s32 SOiGetLastError(void);
/* The library's work record: the allocator pair, the start-up state (-2 down, -1 busy, 0 up), the IOS fd of
 * `/dev/net/ip/top`, the 0x460-byte resolver buffer and the outstanding allocation count. size: 0x18 */
typedef struct SOSysWork {
    /* +0x00 */ SOAllocFunc alloc;
    /* +0x04 */ SOFreeFunc free;
    /* +0x08 */ s32 state;
    /* +0x0C */ s32 fd;
    /* +0x10 */ u8* hostBuffer;
    /* +0x14 */ s32 allocCount;
} SOSysWork;

/* 0x8051EB38 - brings the interface up, giving up after `timeout` milliseconds (0: never). GUESS on the name. */
s32 SOStartupEx(s32 timeout);
/* 0x8051F144 */
SOSysWork* SOiGetSysWork(void);
/* 0x8051F150 */
BOOL SOiIsBufferAddrCheck(void);
/* 0x8051F158 */
BOOL SOiIsInitialized(void);
/* 0x8051F1A8 / 0x8051F290 - the registered allocator pair, counted. */
void* SOiAlloc(u32 name, s32 size); /* untyped: caller-owned payload - an IPC buffer */
void SOiFree(u32 name, void* p, s32 size); /* untyped: caller-owned payload - an IPC buffer */
/* 0x8051F2C4 / 0x8051F3AC */
s32 SOiPrepare(const char* function, s32* fd);
s32 SOiConclude(const char* function, s32 result);
/* 0x8051F408 / 0x8051F6E0 */
s32 SOiPrepareTempRm(const char* function, s32* fd, s32* opened);
s32 SOiConcludeTempRm(const char* function, s32 result, s32 opened);
/* 0x8051F7C4 - waits until the interface has an address. GUESS on the name. */
s32 SOiWaitForDHCPEx(u32 timeout);
#ifdef __cplusplus
}
#endif

#endif
