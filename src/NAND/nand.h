/*
 * NAND/nand.h - declarations of the symbols owned by `NAND/nand.c` that other units call or read, and the aggregator
 * of the OS library headers `NAND/nand.h` used to declare (every includer keeps its view).
 */
#ifndef NAND_NAND_H
#define NAND_NAND_H

#include "types.h"
#include "OS/OS.h"
#include "OS/OSCache.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSLaunch.h"
#include "OS/OSMutex.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"

#ifdef __cplusplus
extern "C" {
#endif

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
/* 0x804C7840 - write `length` bytes of `buffer` to the open file. */
s32 NANDWriteAsync(NANDFileInfo* info, const void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block); /* untyped: byte range */
/* 0x804C72F0 / 0x804C8510 / 0x804C7620 - create, open and delete `path` by its absolute name (the private
 * twins of the home-directory calls). */
s32 NANDPrivateCreateAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback, NANDCommandBlock* block);
s32 NANDPrivateOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block);
s32 NANDPrivateDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block);
/* The record `NANDGetStatus` fills: owner, group (the maker code), attribute and permission. */
typedef struct NANDStatus {
    /* +0x00 */ u32 ownerId;
    /* +0x04 */ u16 groupId;
    /* +0x06 */ u8 attribute;
    /* +0x07 */ u8 permission;
} NANDStatus; /* size: 0x08 */
/* 0x804C8090 - the owner/group/attribute/permission record of `path`. */
s32 NANDGetStatus(const char* path, NANDStatus* status);
/* 0x804C7540 - delete `path` (`ISFS_DeleteAsync` on the absolute path); the public twin of
 * `NANDPrivateDeleteAsync` 0x804C7620. */
s32 NANDDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C9620 - whether `fsBlock` blocks and `inode` inodes still fit the home directory's quota (the answer
 * bits land in `*answer`; `ISFS_GetUsageAsync` on the home directory). */
s32 NANDCheckAsync(u32 fsBlock, u32 inode, u32* answer, NANDAsyncCallback callback, NANDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif
