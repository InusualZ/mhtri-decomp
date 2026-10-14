/*
 * NAND/NANDCheck.c - the Revolution SDK NAND library's usage and quota checks: the banner initializer, the usage
 *    query and the asynchronous home-directory quota check with its user-area completion.
 * RANGE. .text 0x804C9480-0x804C9910 (4 functions); .data 0x8061BAF0-0x8061BBD8 (the user-area directory strings, the
 *    pointer table that lists them, and the completion's strings); .sdata 0x80793F30-0x80793F48 (the banner's wide
 *    literals L"" and L" ", then "/meta" and "/ticket").  Cut from the NAND library 0x804C70E0-0x804C9F00.  Seam with
 *    NAND/NANDCore.c at 0x804C9480: `NANDGetUsage` (0x804C9584) and `NANDCheckAsync` (0x804C9648) call
 *    `nandIsInitialized` out of line while NAND/NANDCore.c inlines it.  Seam with NAND/NANDLogging.c: data order only
 *    (no shared statics).  The banner initializer 0x804C9480 has no proven side and is kept at this unit's head.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. `NANDGetUsage` and `NANDCheckAsync` are the map's (dump); `nandUserAreaCallback` is evidenced by its own
 *    function-name string at 0x8061BB9C.  GUESS: the unit name (SDK spelling), `NANDInitBanner` (SDK spelling),
 *    `s_userDirList` (the pointer table of the user-area directories), `nandCheckQuota` (the inline answer-bit test)
 *    and the `NANDBanner` field names (SDK layout).
 * RESIDUALS. `nandUserAreaCallback`: the argument setup of the follow-on `ISFS_GetUsageAsync` computes `&workInodes` before
 *    `&workBlocks` (the target computes `&workBlocks` from r4 first and `&workInodes` from r31).
 * SHAPES. `nandCheckQuota` takes the six counts by value so their loads are hoisted ahead of the compares, as in the
 *    target.
 */

#include "types.h"
#include "FS/fs.h"
#include "MSL_C/wstring.h"
#include "OS/OSError.h"
#include "NAND/NANDCheck.h"
#include "Runtime.PPCEABI.H/memset.h"

#define NAND_CHECK_STATE_QUOTA 20
#define NAND_CHECK_STATE_AVAILABLE 21
#define NAND_HOME_MAX_BLOCKS 1024
#define NAND_HOME_MAX_INODES 33
#define NAND_USER_MAX_BLOCKS 17408
#define NAND_USER_MAX_INODES 4000
#define NAND_CHECK_HOME_BLOCKS 0x1
#define NAND_CHECK_HOME_INODES 0x2
#define NAND_CHECK_USER_BLOCKS 0x4
#define NAND_CHECK_USER_INODES 0x8

#define NAND_BANNER_SIGNATURE 0x5749424E /* 'WIBN' */
#define NAND_BANNER_COMMENT_LENGTH 32

/* size: 0xF0A0 - the save banner a title writes as banner.bin: its signature, flags, two comment lines and the
 * banner and icon textures (the clear `NANDInitBanner` runs is 0xF0A0 bytes). */
struct NANDBanner {
    /* +0x0000 */ u32 signature;
    /* +0x0004 */ u32 flags;
    /* +0x0008 */ u16 iconSpeed;
    /* +0x000A */ u8 pad_0xA[0x16];
    /* +0x0020 */ u16 comment[2][NAND_BANNER_COMMENT_LENGTH];
    /* +0x00A0 */ u8 bannerTexture[0x6000];
    /* +0x60A0 */ u8 iconTexture[8][0x1200];
}; /* size: 0xF0A0 */

static void nandUserAreaCallback(s32 result, NANDCommandBlock* block);

/* 0x804C9480 (0xDC): clears `banner`, stamps its signature and `flags`, and copies `title` and `comment` (an empty
 * string becomes a single space). */
void NANDInitBanner(NANDBanner* banner, u32 flags, const u16* title, const u16* comment)
{
    memset(banner, 0, sizeof(NANDBanner));
    banner->signature = NAND_BANNER_SIGNATURE;
    banner->flags = flags;
    if (wcscmp(title, L"") == 0) {
        wcsncpy(banner->comment[0], L" ", NAND_BANNER_COMMENT_LENGTH);
    } else {
        wcsncpy(banner->comment[0], title, NAND_BANNER_COMMENT_LENGTH);
    }
    if (wcscmp(comment, L"") == 0) {
        wcsncpy(banner->comment[1], L" ", NAND_BANNER_COMMENT_LENGTH);
    } else {
        wcsncpy(banner->comment[1], comment, NAND_BANNER_COMMENT_LENGTH);
    }
}

/* 0x804C9560 (0xB8): reads the blocks and inodes the files below `path` use. */
s32 NANDGetUsage(const char* path, u32* blockCount, u32* inodeCount)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    } else {
        char absPath[64] = { 0 };

        nandGenerateAbsPath(absPath, path);
        return nandConvertErrorCode(ISFS_GetUsage(absPath, blockCount, inodeCount));
    }
}

/* The user-area directories whose usage counts against the user quota. */
static const char* s_userDirList[] = {
    "/meta",
    "/ticket",
    "/title/00010000",
    "/title/00010001",
    "/title/00010003",
    "/title/00010004",
    "/title/00010005",
    "/title/00010006",
    "/title/00010007",
    "/shared2/title",
    NULL,
};

/* 0x804C9620 (0xAC): starts checking whether `fsBlock` blocks and `inode` inodes still fit the quotas. */
s32 NANDCheckAsync(u32 fsBlock, u32 inode, u32* answer, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    block->fsBlock = fsBlock;
    block->inode = inode;
    block->answer = answer;
    block->userBlocks = 0;
    block->userInodes = 0;
    block->workBlocks = 0;
    block->workInodes = 0;
    block->dir = s_userDirList;
    block->state = NAND_CHECK_STATE_QUOTA;
    return nandConvertErrorCode(ISFS_GetUsageAsync(nandGetHomeDir(), &block->homeBlocks, &block->homeInodes,
                                                   nandUserAreaCallback, block));
}

/* The quota bits `fsBlock` more blocks and `inode` more inodes would overrun, from the home and user-area usage. */
static inline u32 nandCheckQuota(u32 fsBlock, u32 inode, u32 homeBlocks, u32 homeInodes, u32 userBlocks,
                                 u32 userInodes)
{
    u32 answer = 0;

    if (homeBlocks + fsBlock > NAND_HOME_MAX_BLOCKS) {
        answer |= NAND_CHECK_HOME_BLOCKS;
    }
    if (homeInodes + inode > NAND_HOME_MAX_INODES) {
        answer |= NAND_CHECK_HOME_INODES;
    }
    if (userBlocks + fsBlock > NAND_USER_MAX_BLOCKS) {
        answer |= NAND_CHECK_USER_BLOCKS;
    }
    if (userInodes + inode > NAND_USER_MAX_INODES) {
        answer |= NAND_CHECK_USER_INODES;
    }
    return answer;
}

/* 0x804C96D0 (0x234): sums the usage of each user-area directory in turn, then answers the quota check. */
static void nandUserAreaCallback(s32 result, NANDCommandBlock* block)
{
    if (result == 0 || result == ISFS_ERROR_NOEXISTS) {
        const char* path = *block->dir;

        if (result == 0) {
            block->userBlocks += block->workBlocks;
            block->userInodes += block->workInodes;
        }
        if (path != NULL) {
            s32 ret;

            block->dir++;
            ret = ISFS_GetUsageAsync(path, &block->workBlocks, &block->workInodes, nandUserAreaCallback, block);
            if (ret != 0) {
                block->callback(nandConvertErrorCode(ret), block);
            }
        } else if (block->state == NAND_CHECK_STATE_QUOTA) {
            *block->answer = nandCheckQuota(block->fsBlock, block->inode, block->homeBlocks, block->homeInodes,
                                            block->userBlocks, block->userInodes);
            block->callback(nandConvertErrorCode(0), block);
        } else if (block->state == NAND_CHECK_STATE_AVAILABLE) {
            u32 userBlocks = block->userBlocks > NAND_USER_MAX_BLOCKS ? 0 : NAND_USER_MAX_BLOCKS - block->userBlocks;
            u32 userInodes = block->userInodes > NAND_USER_MAX_INODES ? 0 : NAND_USER_MAX_INODES - block->userInodes;
            u32 homeBlocks = block->homeBlocks > NAND_HOME_MAX_BLOCKS ? 0 : NAND_HOME_MAX_BLOCKS - block->homeBlocks;
            u32 homeInodes = block->homeInodes > NAND_HOME_MAX_INODES ? 0 : NAND_HOME_MAX_INODES - block->homeInodes;

            *block->bytes = userBlocks < homeBlocks ? userBlocks : homeBlocks;
            *block->inodes = userInodes < homeInodes ? userInodes : homeInodes;
            block->callback(nandConvertErrorCode(0), block);
        } else {
            OSReport("Illegal status is detected at %s()", __func__);
            block->callback(nandConvertErrorCode(ISFS_ERROR_UNKNOWN), block);
        }
    } else {
        block->callback(nandConvertErrorCode(result), block);
    }
}
