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

#ifdef __cplusplus
}
#endif

#endif
