/*
 * NAND/NANDCheck.h - declarations of the symbols owned by `NAND/NANDCheck.c`: the usage and quota queries.
 */
#ifndef NAND_NANDCHECK_H
#define NAND_NANDCHECK_H

#include "NAND/nand.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAND_BANNER_COMMENT_LENGTH 32

/* size: 0xF0A0 - the save banner a title writes as banner.bin: its signature, flags, two comment lines and the
 * banner and icon textures (the clear `NANDInitBanner` runs is 0xF0A0 bytes; `menu/menu_plsearch.cpp` builds one). */
typedef struct NANDBanner {
    /* +0x0000 */ u32 signature;
    /* +0x0004 */ u32 flags;
    /* +0x0008 */ u16 iconSpeed;
    /* +0x000A */ u8 pad_0xA[0x16];
    /* +0x0020 */ u16 comment[2][NAND_BANNER_COMMENT_LENGTH];
    /* +0x00A0 */ u8 bannerTexture[0x6000];
    /* +0x60A0 */ u8 iconTexture[8][0x1200];
} NANDBanner; /* size: 0xF0A0 */

/* 0x804C9480 - clears `banner`, stamps its signature and `flags`, and copies `title` and `comment` (an empty string
 * becomes a single space). */
void NANDInitBanner(NANDBanner* banner, u32 flags, const u16* title, const u16* comment);
/* 0x804C9560 - the blocks and inodes the files below `path` use (`ISFS_GetUsage` on the absolute path). */
s32 NANDGetUsage(const char* path, u32* blockCount, u32* inodeCount);
/* 0x804C9620 - whether `fsBlock` blocks and `inode` inodes still fit the home directory's quota (the answer
 * bits land in `*answer`; `ISFS_GetUsageAsync` on the home directory). */
s32 NANDCheckAsync(u32 fsBlock, u32 inode, u32* answer, NANDAsyncCallback callback, NANDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif
