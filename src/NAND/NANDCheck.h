/*
 * NAND/NANDCheck.h - declarations of the symbols owned by `NAND/NANDCheck.c`: the usage and quota queries.
 */
#ifndef NAND_NANDCHECK_H
#define NAND_NANDCHECK_H

#include "NAND/nand.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C9560 - the blocks and inodes the files below `path` use (`ISFS_GetUsage` on the absolute path). */
s32 NANDGetUsage(const char* path, u32* blockCount, u32* inodeCount);
/* 0x804C9620 - whether `fsBlock` blocks and `inode` inodes still fit the home directory's quota (the answer
 * bits land in `*answer`; `ISFS_GetUsageAsync` on the home directory). */
s32 NANDCheckAsync(u32 fsBlock, u32 inode, u32* answer, NANDAsyncCallback callback, NANDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif
