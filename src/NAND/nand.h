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

/* 0x804D1F20 / 0x804D2000 (with `OSInitMutex` 0x804D1EE0 above) - the OS mutex set. */
/* untyped: opaque band object, typed by the callers' views */
void OSLockMutex(void* mutex);

/* untyped: opaque band object, typed by the callers' views */
void OSUnlockMutex(void* mutex);

/* 0x804D3F70 / 0x804D4600 - create a thread on `stack` (growing down, `stackSize` bytes) that runs `entry(param)`,
 * and start it; non-zero on success.  The callers pass their own spellings of the thread record (an OS thread
 * block, a `u8` array) and of the entry, so the parameters are still untyped. */
s32 OSCreateThread(void* thread, void* entry, void* param, void* stack, u32 stackSize, s32 priority, u32 flags);
s32 OSResumeThread(void* thread);
/* untyped: opaque handle passed through - the OS thread record */
s32 OSIsThreadTerminated(void* thread);

/* 0x804D4CA0 - sleep the calling thread for `ticks` time-base ticks. */
void OSSleepTicks(u64 ticks);

void OSReport(const char* format, ...);

/* 0x804D4D70 / 0x804D4D50 - the low 32 bits of the time base / the whole 64-bit time base (the SDK's
 * signed `OSTime`; one tick is a quarter of the bus clock). */
u32 OSGetTick(void);

s64 OSGetTime(void);

/* The NAND file API the EC (shop) client `Network/NetworkPool.cpp` drives.  The command block and the file
 * record are the SDK's; their layouts stay with the NAND band (only their addresses are passed here). */
typedef struct NANDCommandBlock NANDCommandBlock;
typedef struct NANDFileInfo NANDFileInfo;
typedef void (*NANDAsyncCallback)(s32 result, NANDCommandBlock* block);

/* 0x804C9020 - copy the title's home directory path into `path`. */
s32 NANDGetHomeDir(char* path);
/* 0x804C8490 - open `path` with access mode `mode`; the public twin of `NANDPrivateOpenAsync` 0x804C8510. */
s32 NANDOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C7750 - read `length` bytes of the open file into `buffer`. */
s32 NANDReadAsync(NANDFileInfo* info, void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block); /* untyped: byte range */
/* 0x804C8680 - close the open file. */
s32 NANDCloseAsync(NANDFileInfo* info, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C7540 - delete `path` (`ISFS_DeleteAsync` on the absolute path); the public twin of
 * `NANDPrivateDeleteAsync` 0x804C7620. */
s32 NANDDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C9620 - whether `fsBlock` blocks and `inode` inodes still fit the home directory's quota (the answer
 * bits land in `*answer`; `ISFS_GetUsageAsync` on the home directory). */
s32 NANDCheckAsync(u32 fsBlock, u32 inode, u32* answer, NANDAsyncCallback callback, NANDCommandBlock* block);

/* 0x804D7ED0 - leaves the game for the Wii Shop Channel's help page: tears down the state events, rebuilds
 * the MEM1 arena, then launches title 00010002-484142xx (`HABA`, or `HABK`/`HABC` when the SC area code 0x804DD2C0 returns 4/5) with the
 * argument "/startup?initpage=showHelp" (.data 0x80629A04).  NAME (a GUESS in the scheme of the
 * `OSLaunchPDChannel` log text beside it); it does not return. */
void OSLaunchShopChannelHelp(void);

#ifdef __cplusplus
}
#endif

#endif
