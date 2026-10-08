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
/* size: 0x8C - the open-file record (the callers' buffers are 0x8C bytes: `NetworkPool`, `DWCi_Np_CPUCopyFast`); the field split is a GUESS. */
struct NANDFileInfo {
    /* +0x00 */ s32 fileDescriptor;
    /* +0x04 */ s32 originalDescriptor;
    /* +0x08 */ char originalPath[64];
    /* +0x48 */ char temporaryPath[64];
    /* +0x88 */ u8 accessMode;
    /* +0x89 */ u8 stage;
    /* +0x8A */ u8 mark; /* 1 while the file is open, 2 once it has been closed */
    /* +0x8B */ u8 pad_0x8B;
}; /* size: 0x8C */

/* size: 0xC (cut at the last field) - what the async calls keep for their completion. */
struct NANDCommandBlock {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ NANDAsyncCallback callback; /* the caller's completion */
    /* +0x08 */ NANDFileInfo* info;         /* the file record of an async close */
};

/* 0x804C76E0 / 0x804C77D0 / 0x804C8610 - read, write and close the open file. */
s32 NANDRead(NANDFileInfo* info, void* buffer, u32 length); /* untyped: byte range */
s32 NANDWrite(NANDFileInfo* info, const void* buffer, u32 length); /* untyped: byte range */
s32 NANDClose(NANDFileInfo* info);
/* 0x804C9020 - copy the title's home directory path into `path`. */
s32 NANDGetHomeDir(char* path);
/* 0x804C8490 - open `path` with access mode `mode`; the public twin of `NANDPrivateOpenAsync` 0x804C8510. */
s32 NANDOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C7750 - read `length` bytes of the open file into `buffer`. */
s32 NANDReadAsync(NANDFileInfo* info, void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block); /* untyped: byte range */
/* 0x804C8680 - close the open file. */
s32 NANDCloseAsync(NANDFileInfo* info, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C78C0 - move the file position of the open file to `offset` according to `whence`. */
s32 NANDSeekAsync(NANDFileInfo* info, s32 offset, s32 whence, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C8E10 - bring the NAND filesystem up; zero on success. */
s32 NANDInit(void);
/* 0x804C7840 - write `length` bytes of `buffer` to the open file. */
s32 NANDWriteAsync(NANDFileInfo* info, const void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block); /* untyped: byte range */
/* 0x804C72F0 / 0x804C8510 / 0x804C7620 - create, open and delete `path` by its absolute name (the private
 * twins of the home-directory calls). */
s32 NANDPrivateCreateAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback, NANDCommandBlock* block);
s32 NANDPrivateOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block);
s32 NANDPrivateDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C93A0 / 0x804C7A90 - probe the node type of `path` (1 file, 2 directory) and create the directory `path`. */
s32 NANDPrivateGetTypeAsync(const char* path, u8* type, NANDAsyncCallback callback, NANDCommandBlock* block);
s32 NANDPrivateCreateDirAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback, NANDCommandBlock* block);
/* The record `NANDGetStatus` fills: owner, group (the maker code), attribute and permission. */
typedef struct NANDStatus {
    /* +0x00 */ u32 ownerId;
    /* +0x04 */ u16 groupId;
    /* +0x06 */ u8 attribute;
    /* +0x07 */ u8 permission;
} NANDStatus; /* size: 0x08 */
/* 0x804C8090 - the owner/group/attribute/permission record of `path`. */
s32 NANDGetStatus(const char* path, NANDStatus* status);
/* 0x804C8170 - reads the status record of `path` (an absolute name) asynchronously. */
s32 NANDPrivateGetStatusAsync(const char* path, NANDStatus* status, NANDAsyncCallback callback, NANDCommandBlock* block);
/* 0x804C9560 - the blocks and inodes the files below `path` use (`ISFS_GetUsage` on the absolute path). */
s32 NANDGetUsage(const char* path, u32* blockCount, u32* inodeCount);
/* 0x804C8100 - `NANDGetStatus` on an absolute path. */
s32 NANDPrivateGetStatus(const char* path, NANDStatus* status);
/* 0x804C70E0 - create `path` by its absolute name with `permission` and `attribute`. */
s32 NANDPrivateCreate(const char* path, u8 permission, u8 attribute);
/* 0x804C74A0 - delete `path` by its absolute name. */
s32 NANDPrivateDelete(const char* path);
/* 0x804C8370 - open `path` (relative to the title's home directory) with access mode `mode`. */
s32 NANDOpen(const char* path, NANDFileInfo* info, u8 mode);
/* 0x804C76E0 - read `length` bytes of the open file into `buffer`; returns the byte count. */
s32 NANDRead(NANDFileInfo* info, void* buffer, u32 length); /* untyped: byte range */
/* 0x804C73F0 - delete `path`. */
s32 NANDDelete(const char* path);
/* 0x804C8400 - open `path` by its absolute name with access mode `mode`. */
s32 NANDPrivateOpen(const char* path, NANDFileInfo* info, u8 mode);
/* 0x804C8610 - close the open file. */
s32 NANDClose(NANDFileInfo* info);
/* 0x804C77D0 - write `length` bytes of `buffer` to the open file; returns the byte count. */
s32 NANDWrite(NANDFileInfo* info, const void* buffer, u32 length); /* untyped: byte range */
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
