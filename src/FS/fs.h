/*
 * FS/fs.h - declarations of the symbols owned by `FS/fs.c` (the ISFS client of the IOS file system) that other units
 *    call. A path is an absolute IOS file system path; the Async forms call `callback` with the result and `block`.
 */
#ifndef FS_FS_H
#define FS_FS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The ISFS result codes the NAND library tests. */
#define ISFS_ERROR_INVALID (-101)
#define ISFS_ERROR_ACCESS (-102)
#define ISFS_ERROR_NOEXISTS (-106)
#define ISFS_ERROR_UNKNOWN (-117)

struct NANDCommandBlock;

/* Called when an asynchronous ISFS call finishes; `result` is the ISFS result code. */
typedef void (*ISFSCallback)(s32 result, struct NANDCommandBlock* block);

/* 0x804B3000 / 0x804B3020 - read `length` bytes of the open file `fd` into `buffer`. */
/* untyped: byte range the read fills */
s32 ISFS_Read(s32 fd, void* buffer, u32 length);
/* untyped: byte range the read fills */
s32 ISFS_ReadAsync(s32 fd, void* buffer, u32 length, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B30D0 / 0x804B30F0 - write `length` bytes of `buffer` to the open file `fd`. */
/* untyped: byte range the write sends */
s32 ISFS_Write(s32 fd, const void* buffer, u32 length);
/* untyped: byte range the write sends */
s32 ISFS_WriteAsync(s32 fd, const void* buffer, u32 length, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B2F70 - move the position of the open file `fd` (`whence` 0 start, 1 current, 2 end). */
s32 ISFS_SeekAsync(s32 fd, s32 offset, s32 whence, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B2DE0 / 0x804B2EB0 - open the file at the absolute `path` (`mode` 1 read, 2 write, 3 both). */
s32 ISFS_Open(const char* path, s32 mode);
s32 ISFS_OpenAsync(const char* path, s32 mode, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B31A0 / 0x804B31B0 - close the open file `fd`. */
s32 ISFS_Close(s32 fd);
s32 ISFS_CloseAsync(s32 fd, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B1CA0 - open `/dev/fs` and set up the library's IPC heap. */
s32 ISFS_OpenLib(void);

/* 0x804B2BE0 / 0x804B2CE0 - create the file at the absolute `path` with attribute `attr` and the owner, group and
 * other access words. */
s32 ISFS_CreateFile(const char* path, u32 attr, u32 ownerAcc, u32 groupAcc, u32 othersAcc);
s32 ISFS_CreateFileAsync(const char* path, u32 attr, u32 ownerAcc, u32 groupAcc, u32 othersAcc, ISFSCallback callback,
                         struct NANDCommandBlock* block);

/* 0x804B1F50 - create the directory at the absolute `path` with attribute `attr` and the access words. */
s32 ISFS_CreateDirAsync(const char* path, u32 attr, u32 ownerAcc, u32 groupAcc, u32 othersAcc, ISFSCallback callback,
                        struct NANDCommandBlock* block);

/* 0x804B25A0 / 0x804B2680 - delete the file or directory at the absolute `path`. */
s32 ISFS_Delete(const char* path);
s32 ISFS_DeleteAsync(const char* path, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B2750 / 0x804B2860 - move the node at the absolute path `from` to `to`. */
s32 ISFS_Rename(const char* from, const char* to);
s32 ISFS_RenameAsync(const char* from, const char* to, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B2300 / 0x804B2460 - read the owner, group, attribute and access words of the node at the absolute `path`. */
s32 ISFS_GetAttr(const char* path, u32* ownerId, u16* groupId, u32* attr, u32* ownerAcc, u32* groupAcc,
                 u32* othersAcc);
s32 ISFS_GetAttrAsync(const char* path, u32* ownerId, u16* groupId, u32* attr, u32* ownerAcc, u32* groupAcc,
                      u32* othersAcc, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B2970 / 0x804B2AB0 - the blocks and inodes the directory at the absolute `path` uses. */
s32 ISFS_GetUsage(const char* path, u32* blockCount, u32* inodeCount);
s32 ISFS_GetUsageAsync(const char* path, u32* blockCount, u32* inodeCount, ISFSCallback callback,
                       struct NANDCommandBlock* block);

/* 0x804B2050 / 0x804B21B0 - list the directory at the absolute `path` into `names` (or count it when `names` is
 * NULL); `*count` is the entry count in and out. */
s32 ISFS_ReadDir(const char* path, char* names, u32* count);
s32 ISFS_ReadDirAsync(const char* path, char* names, u32* count, ISFSCallback callback, struct NANDCommandBlock* block);

/* 0x804B3240 - shut the file system down; `callback` receives `context` back. */
/* untyped: caller-owned payload */
s32 ISFS_ShutdownAsync(ISFSCallback callback, void* context);

#ifdef __cplusplus
}
#endif

#endif
