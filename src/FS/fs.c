/*
 * FS/fs.c - the SDK ISFS file-system client over `/dev/fs` (ISFS_*).
 *
 * RANGE. `.text` 0x804B1CA0-0x804B32D0 (25 functions / 0x1630 B); `.data` 0x8061A538-0x8061A560; `.sdata` 0x80793E38-0x80793E48;
 *   `.sbss` 0x807951B0-0x807951C8.
 *   - the string `APP ERROR: Not enough IPC arena` (`.data` 0x8061A538), `__fsFd` / the `/dev/fs` path (`.sdata`
 *     0x80793E38 / 0x80793E40) and the init flag, device path and heap id (`.sbss` 0x807951B0..0x807951C8) are read
 *     only by this run
 *   - the run starts at `ISFS_OpenLib` and ends before the GX library's first function (0x804B32D0)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16): every
 *   function start of the file is 16-aligned and `-func_align 4` loses the layout.
 * NAMES. the five entry points the map did not name are `ISFS_CreateDirAsync`, `ISFS_ReadDir`,
 *   `ISFS_ReadDirAsync`, `ISFS_GetUsageAsync` (`ISFS_CreateDirAsync` is a GUESS, `ISFS_ReadDir` is a GUESS, `ISFS_ReadDirAsync` is a GUESS and `ISFS_GetUsageAsync` is a GUESS, all in the SDK's scheme) and `ISFS_CreateFileAsync` (the ioctl number and argument shape prove them;
 *   the dump answers `ISFS_CreateFileAsync` for two addresses, so it is not evidence on its own). `_isfsFuncCb` is a
 *   GUESS (the completion callback every async ISFS command registers; the map carries the placeholder only);
 *   `__fsRequestPending` is a GUESS (the word `_isfsFuncCb` clears; one writer, no reader in the DOL).
 * RESIDUALS. none in `.text` / `.data` / `.sdata` / `.sbss`; relocdiff by name differs only on literal and local-static spellings
 *   (`@191`/`@192` vs `@1687`/`@1688`, `lo`/`hi` vs `lo$688`/`hi$689`).
 */

#include "types.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcclt.h"
#include "IPC/memory.h"
#include "NAND/nand.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* ===================================================================================================
 * FS/ISFS - the SDK file-system library's entry points at 0x804B1CA0..0x804B32D0.
 *
 * The library keeps one IPC heap and one `/dev/fs` fd, hands every ioctl a 0x140-byte command block out
 * of that heap and completes the async ones through `_isfsFuncCb`.  
 * =================================================================================================== */


#define FS_MAX_PATH 64
#define FS_DIR_NAME_MAX (12 + 1)
#define FS_HEAP_SIZE 0x1500

#define ROUND_UP_PTR(ptr, align) ((void*)(((u32)(ptr) + (align)-1) & ~((align)-1)))

#define FS_DELETE(x)                                                                                   \
    if ((x) != NULL) {                                                                                 \
        iosFree(hId, (x));                                                                    \
    }


/* The IPC result the FS library hands back; the values are the ones the target's branches load. */
typedef enum {
    IPC_RESULT_OK = 0,
    IPC_RESULT_ALLOC_FAILED = -22,
    IPC_RESULT_INVALID = -101,
    IPC_RESULT_BUSY = -118
} IPCResult;

/* For ISFS_CreateDirAsync / ISFS_CreateFileAsync. size: 0x4C */
typedef struct FSFileIoctl {
    u32 ownerId;              /* +0x0 */
    u16 groupId;              /* +0x4 */
    char path[FS_MAX_PATH];   /* +0x6 */
    u8 ownerPerm;             /* +0x46 */
    u8 groupPerm;             /* +0x47 */
    u8 otherPerm;             /* +0x48 */
    u8 attr;                  /* +0x49 */
    u8 pad_0x4A[0x4C - 0x4A]; /* +0x4A */
} FSFileIoctl;

/* For ISFS_RenameAsync. size: 0x80 */
typedef struct FSRenameIoctl {
    char from[FS_MAX_PATH]; /* +0x0 */
    char to[FS_MAX_PATH];   /* +0x40 */
} FSRenameIoctl;

/* Async context the completion callback finishes the operation with. */
typedef struct FSReadDirAsyncCtx {
    u32* fileCountOut; /* +0x0 */
} FSReadDirAsyncCtx; /* size: 0x4 */

typedef struct FSGetAttrAsyncCtx {
    u32* ownerIdOut;   /* +0x0 */
    u16* groupIdOut;   /* +0x4 */
    u32* attrOut;      /* +0x8 */
    u32* ownerPermOut; /* +0xC */
    u32* groupPermOut; /* +0x10 */
    u32* otherPermOut; /* +0x14 */
} FSGetAttrAsyncCtx; /* size: 0x18 */

typedef struct FSGetUsageAsyncCtx {
    u32* blockCountOut; /* +0x0 */
    u32* fileCountOut;  /* +0x4 */
} FSGetUsageAsyncCtx; /* size: 0x8 */

/* The stat record the SDK's stat callbacks copy out of the work area, and the two contexts that carry
 * their destination. size: 0x1C */
typedef struct FSStats {
    u8 pad_0x0[0x1C]; /* +0x0 */
} FSStats;

typedef struct FSFileStats {
    u32 length;   /* +0x0 */
    u32 position; /* +0x4 */
} FSFileStats; /* size: 0x8 */

typedef struct FSGetStatsAsyncCtx {
    FSStats* statsOut; /* +0x0 */
} FSGetStatsAsyncCtx; /* size: 0x4 */

typedef struct FSGetFileStatsAsyncCtx {
    FSFileStats* statsOut; /* +0x0 */
} FSGetFileStatsAsyncCtx; /* size: 0x4 */

/* The state `_isfsFuncCb` switches on to decide what the finished command must still copy out. */
typedef enum {
    CB_STATE_NONE,
    CB_STATE_GET_STATS,
    CB_STATE_READ_DIR,
    CB_STATE_GET_ATTR,
    CB_STATE_GET_USAGE,
    CB_STATE_GET_FILE_STATS
} FSCallbackState;

/* The FS ioctl commands (the numbers each entry point passes to IOS_Ioctl/IOS_Ioctlv). */
typedef enum {
    FS_IOCTL_READ_DIR = 4,
    FS_IOCTL_GET_ATTR = 6,
    FS_IOCTL_DELETE_PATH = 7,
    FS_IOCTL_RENAME_PATH = 8,
    FS_IOCTL_CREATE_FILE = 9,
    FS_IOCTL_CREATE_DIR = 3,
    FS_IOCTL_SHUTDOWN_FS = 13,

    FS_IOCTLV_GET_USAGE = 12
} FSIoctl;

/* The command block every FS entry point works in. size: 0x140 */
typedef struct FSCommandBlock {
    union {
        FSFileIoctl fileIoctl;     /* +0x0 */
        FSRenameIoctl renameIoctl; /* +0x0 */
        u8 ioctlWork[0x100];       /* +0x0 */
    } data;                        /* +0x0 */
    void* callback;                /* +0x100 */
    void* callbackArg;             /* +0x104 */
    u32 callbackState;             /* +0x108 (one of FSCallbackState) */
    union {
        FSGetStatsAsyncCtx getStatsCtx;         /* +0x10C */
        FSReadDirAsyncCtx readDirCtx;           /* +0x10C */
        FSGetAttrAsyncCtx getAttrCtx;   /* +0x10C */
        FSGetUsageAsyncCtx getUsageCtx;         /* +0x10C */
        FSGetFileStatsAsyncCtx getFileStatsCtx; /* +0x10C */
        u8 pad_0x10C[0x140 - 0x10C];    /* +0x10C */
    } ctx;
} FSCommandBlock;

/* The FS library's own state: the `/dev/fs` fd (-1 until opened), the heap id, the reserved IPC arena window, the
 * request-pending flag the completion callback clears, the IPC-arena device path and the initialised flag.  The
 * compiler emits `.sbss` in reverse definition order, so this order lays the words out as the target has them. */
static s32 __fsFd = -1;
static s32 hId;
static void* hi;
static void* lo;
static u32 __fsRequestPending;
static char* __devfs;
static u32 __fsInitialized;

char* strcpy(char* dst, const char* src);

/* Open the `/dev/fs` device, carve the IPC arena window for it and create the FS heap. */
s32 ISFS_OpenLib(void)
{
    s32 ret = IPC_RESULT_OK;
    u8* base;

    if (!__fsInitialized) {
        lo = IPCGetBufferLo();
        hi = IPCGetBufferHi();
    }

    __devfs = (char*)ROUND_UP_PTR(lo, 32);

    if (!__fsInitialized && __devfs + FS_MAX_PATH > (char*)hi) {
        OSReport("APP ERROR: Not enough IPC arena\n");
        ret = IPC_RESULT_ALLOC_FAILED;
    } else {
        strcpy(__devfs, "/dev/fs");
        __fsFd = IOS_Open(__devfs, 0);

        if (__fsFd < 0) {
            ret = __fsFd;
        } else {
            base = (u8*)__devfs;

            if (!__fsInitialized && base + FS_MAX_PATH + FS_HEAP_SIZE > (u8*)hi) {
                OSReport("APP ERROR: Not enough IPC arena\n");
                ret = IPC_RESULT_ALLOC_FAILED;
            } else {
                if (!__fsInitialized) {
                    IPCSetBufferLo(base + FS_MAX_PATH + FS_HEAP_SIZE);
                    __fsInitialized = TRUE;
                }

                hId = iosCreateHeap(base, FS_MAX_PATH + FS_HEAP_SIZE);

                if (hId < 0) {
                    ret = IPC_RESULT_ALLOC_FAILED;
                }
            }
        }
    }

    return ret;
}

/* Completion callback of every async FS command: finish the operation the command asked for, then hand
 * the result to the caller's callback and free the command block. */
s32 _isfsFuncCb(s32 result, void* arg)
{
    FSCommandBlock* block = (FSCommandBlock*)arg;

    if (result >= IPC_RESULT_OK) {
        switch (block->callbackState) {
        case CB_STATE_GET_STATS:
            if (result == IPC_RESULT_OK) {
                memcpy(block->ctx.getStatsCtx.statsOut, block->data.ioctlWork, sizeof(FSStats));
            }
            break;

        case CB_STATE_READ_DIR:
            if (result == IPC_RESULT_OK) {
                u8* pathWork =
                    (u8*)ROUND_UP_PTR(block->data.ioctlWork + (sizeof(IPCIOVector) * 4), 32);
                *block->ctx.readDirCtx.fileCountOut = *(u32*)ROUND_UP_PTR(pathWork + FS_MAX_PATH, 32);
            }
            break;

        case CB_STATE_GET_ATTR:
            if (result == IPC_RESULT_OK) {
                FSFileIoctl* fileIoctl =
                    (FSFileIoctl*)ROUND_UP_PTR(block->data.ioctlWork + FS_MAX_PATH, 32);

                *block->ctx.getAttrCtx.ownerIdOut = fileIoctl->ownerId;
                *block->ctx.getAttrCtx.groupIdOut = fileIoctl->groupId;
                *block->ctx.getAttrCtx.attrOut = fileIoctl->attr;
                *block->ctx.getAttrCtx.ownerPermOut = fileIoctl->ownerPerm;
                *block->ctx.getAttrCtx.groupPermOut = fileIoctl->groupPerm;
                *block->ctx.getAttrCtx.otherPermOut = fileIoctl->otherPerm;
            }
            break;

        case CB_STATE_GET_USAGE:
            if (result == IPC_RESULT_OK) {
                u8* work = (u8*)ROUND_UP_PTR(block->data.ioctlWork + (sizeof(IPCIOVector) * 4), 32);

                work = (u8*)ROUND_UP_PTR(work + FS_MAX_PATH, 32);
                *block->ctx.getUsageCtx.blockCountOut = *(u32*)work;

                work = (u8*)ROUND_UP_PTR(work + sizeof(u32), 32);
                *block->ctx.getUsageCtx.fileCountOut = *(u32*)work;
            }
            break;

        case CB_STATE_GET_FILE_STATS:
            if (result == IPC_RESULT_OK) {
                memcpy(block->ctx.getFileStatsCtx.statsOut, block->data.ioctlWork,
                       sizeof(FSFileStats));
            }
            break;

        default:
            break;
        }
    }

    __fsRequestPending = 0;

    if (block->callback != NULL) {
        ((void (*)(s32, void*))block->callback)(result, block->callbackArg);
    }

    FS_DELETE(block);
    return result;
}

/* Creates a directory: ioctl 3, the 0x4C-byte FSFileIoctl, completion
 * callback `_isfsFuncCb`. */
s32 ISFS_CreateDirAsync(const char* path, u32 attr, u32 ownerPerm, u32 groupPerm, u32 otherPerm, void* callback,
                void* callbackArg)
{
    FSCommandBlock* block;
    u32 len;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    memcpy(block->data.fileIoctl.path, path, len + 1);

    block->data.fileIoctl.attr = attr;
    block->data.fileIoctl.ownerPerm = ownerPerm;
    block->data.fileIoctl.groupPerm = groupPerm;
    block->data.fileIoctl.otherPerm = otherPerm;

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_CREATE_DIR, &block->data.fileIoctl,
                          sizeof(FSFileIoctl), NULL, 0, _isfsFuncCb, block);
}

/* Lists a directory: ioctlv 4, a 2-in/2-out vector list built in the work area. */
s32 ISFS_ReadDir(const char* path, char* filesOut, u32* fileCountOut)
{
    s32 ret;
    FSCommandBlock* block;
    char* pathWork;
    IPCIOVector* vectors;
    u32 len;
    u32* countWork;
    u32 inCount;
    u32 outCount;

    block = NULL;

    if (path == NULL || fileCountOut == NULL || __fsFd < 0 || (u32)filesOut % 32 != 0 ||
        (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            vectors = (IPCIOVector*)block->data.ioctlWork;
            pathWork = (char*)ROUND_UP_PTR((u8*)vectors + (sizeof(IPCIOVector) * 4), 32);
            memcpy(pathWork, path, len + 1);

            vectors[0].base = pathWork;
            vectors[0].length = FS_MAX_PATH;

            countWork = (u32*)ROUND_UP_PTR(pathWork + FS_MAX_PATH, 32);
            vectors[1].base = countWork;
            vectors[1].length = sizeof(u32);

            if (filesOut != NULL) {
                inCount = 2;
                outCount = 2;

                *countWork = *fileCountOut;

                vectors[2].base = filesOut;
                vectors[2].length = *fileCountOut * FS_DIR_NAME_MAX;
                vectors[3].base = countWork;
                vectors[3].length = sizeof(u32);
            } else {
                inCount = 1;
                outCount = 1;
            }

            ret = IOS_Ioctlv(__fsFd, FS_IOCTL_READ_DIR, inCount, outCount, vectors);

            if (ret == IPC_RESULT_OK) {
                *fileCountOut = *countWork;
            }
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_ReadDir`: the same vector list as `ISFS_ReadDir`, completed by
 * `_isfsFuncCb`, which copies the file count back through the stored context. */
s32 ISFS_ReadDirAsync(const char* path, char* filesOut, u32* fileCountOut, void* callback, void* callbackArg)
{
    FSCommandBlock* block;
    char* pathWork;
    IPCIOVector* vectors;
    u32 len;
    u32* countWork;
    u32 inCount;
    u32 outCount;

    if (path == NULL || fileCountOut == NULL || __fsFd < 0 || (u32)filesOut % 32 != 0 ||
        (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_READ_DIR;
    block->ctx.readDirCtx.fileCountOut = fileCountOut;

    vectors = (IPCIOVector*)block->data.ioctlWork;
    pathWork = (char*)ROUND_UP_PTR((u8*)vectors + (sizeof(IPCIOVector) * 4), 32);
    memcpy(pathWork, path, len + 1);

    vectors = (IPCIOVector*)block->data.ioctlWork;
    vectors[0].base = pathWork;
    vectors[0].length = FS_MAX_PATH;

    countWork = (u32*)ROUND_UP_PTR(pathWork + FS_MAX_PATH, 32);
    vectors[1].base = countWork;
    vectors[1].length = sizeof(u32);

    if (filesOut != NULL) {
        inCount = 2;
        outCount = 2;

        *countWork = *fileCountOut;

        vectors[2].base = filesOut;
        vectors[2].length = *fileCountOut * FS_DIR_NAME_MAX;
        vectors[3].base = countWork;
        vectors[3].length = sizeof(u32);
    } else {
        inCount = 1;
        outCount = 1;
    }

    return IOS_IoctlvAsync(__fsFd, FS_IOCTL_READ_DIR, inCount, outCount, vectors, _isfsFuncCb,
                           block);
}

/* Read a file's owner/group/permissions/attribute record. */
s32 ISFS_GetAttr(const char* path, u32* ownerIdOut, u16* groupIdOut, u32* attrOut, u32* ownerPermOut,
                 u32* groupPermOut, u32* otherPermOut)
{
    s32 ret;
    FSFileIoctl* fileIoctl;
    u32 len;
    FSCommandBlock* block;

    block = NULL;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH ||
        ownerIdOut == NULL || groupIdOut == NULL || attrOut == NULL || ownerPermOut == NULL ||
        groupPermOut == NULL || otherPermOut == NULL) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            memcpy(block->data.ioctlWork, path, len + 1);
            fileIoctl = (FSFileIoctl*)ROUND_UP_PTR(block->data.ioctlWork + FS_MAX_PATH, 32);

            ret = IOS_Ioctl(__fsFd, FS_IOCTL_GET_ATTR, block->data.ioctlWork, FS_MAX_PATH,
                            fileIoctl, sizeof(FSFileIoctl));

            if (ret == IPC_RESULT_OK) {
                *ownerIdOut = fileIoctl->ownerId;
                *groupIdOut = fileIoctl->groupId;
                *attrOut = fileIoctl->attr;
                *ownerPermOut = fileIoctl->ownerPerm;
                *groupPermOut = fileIoctl->groupPerm;
                *otherPermOut = fileIoctl->otherPerm;
            }
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_GetAttr`; `_isfsFuncCb` copies the six fields out. */
s32 ISFS_GetAttrAsync(const char* path, u32* ownerIdOut, u16* groupIdOut, u32* attrOut,
                      u32* ownerPermOut, u32* groupPermOut, u32* otherPermOut, void* callback,
                      void* callbackArg)
{
    u32 len;
    FSCommandBlock* block;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH ||
        ownerIdOut == NULL || groupIdOut == NULL || attrOut == NULL || ownerPermOut == NULL ||
        groupPermOut == NULL || otherPermOut == NULL) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->ctx.getAttrCtx.ownerIdOut = ownerIdOut;
    block->ctx.getAttrCtx.groupIdOut = groupIdOut;
    block->ctx.getAttrCtx.attrOut = attrOut;
    block->ctx.getAttrCtx.ownerPermOut = ownerPermOut;
    block->ctx.getAttrCtx.groupPermOut = groupPermOut;
    block->ctx.getAttrCtx.otherPermOut = otherPermOut;

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_GET_ATTR;

    memcpy(block->data.ioctlWork, path, len + 1);

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_GET_ATTR, block->data.ioctlWork, FS_MAX_PATH,
                          ROUND_UP_PTR(block->data.ioctlWork + FS_MAX_PATH, 32), sizeof(FSFileIoctl),
                          _isfsFuncCb, block);
}

/* Delete a file or directory. */
s32 ISFS_Delete(const char* path)
{
    s32 ret;
    u32 len;
    FSCommandBlock* block;

    block = NULL;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            memcpy(block->data.ioctlWork, path, len + 1);

            ret = IOS_Ioctl(__fsFd, FS_IOCTL_DELETE_PATH, block->data.ioctlWork, FS_MAX_PATH, 0,
                            NULL);
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_Delete`. */
s32 ISFS_DeleteAsync(const char* path, void* callback, void* callbackArg)
{
    u32 len;
    FSCommandBlock* block;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    memcpy(block->data.ioctlWork, path, len + 1);

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_DELETE_PATH, block->data.ioctlWork, FS_MAX_PATH, 0,
                          NULL, _isfsFuncCb, block);
}

/* Rename a file or directory. */
s32 ISFS_Rename(const char* from, const char* to)
{
    s32 ret;
    u32 lenFrom;
    u32 lenTo;
    FSCommandBlock* block;

    block = NULL;

    if (from == NULL || to == NULL || __fsFd < 0 ||
        (lenFrom = strnlen(from, FS_MAX_PATH)) == FS_MAX_PATH ||
        (lenTo = strnlen(to, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            memcpy(block->data.renameIoctl.from, from, lenFrom + 1);
            memcpy(block->data.renameIoctl.to, to, lenTo + 1);

            ret = IOS_Ioctl(__fsFd, FS_IOCTL_RENAME_PATH, &block->data.renameIoctl,
                            sizeof(FSRenameIoctl), 0, NULL);
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_Rename`. */
s32 ISFS_RenameAsync(const char* from, const char* to, void* callback, void* callbackArg)
{
    u32 lenFrom;
    u32 lenTo;
    FSCommandBlock* block;

    block = NULL;

    if (from == NULL || to == NULL || __fsFd < 0 ||
        (lenFrom = strnlen(from, FS_MAX_PATH)) == FS_MAX_PATH ||
        (lenTo = strnlen(to, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    memcpy(block->data.renameIoctl.from, from, lenFrom + 1);
    memcpy(block->data.renameIoctl.to, to, lenTo + 1);

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_RENAME_PATH, &block->data.renameIoctl,
                          sizeof(FSRenameIoctl), 0, NULL, _isfsFuncCb, block);
}

/* Read how many blocks and files a directory uses. */
s32 ISFS_GetUsage(const char* path, s32* blockCountOut, s32* fileCountOut)
{
    s32 ret;
    u32* blockCountWork;
    u32* fileCountWork;
    char* pathWork;
    IPCIOVector* vectors;
    FSCommandBlock* block;
    u32 len;

    block = NULL;

    if (path == NULL || __fsFd < 0 || blockCountOut == NULL || fileCountOut == NULL ||
        (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            vectors = (IPCIOVector*)block->data.ioctlWork;
            pathWork = (char*)ROUND_UP_PTR((u8*)vectors + (sizeof(IPCIOVector) * 3), 32);
            memcpy(pathWork, path, len + 1);

            vectors[0].base = pathWork;
            vectors[0].length = FS_MAX_PATH;

            blockCountWork = (u32*)ROUND_UP_PTR(pathWork + FS_MAX_PATH, 32);
            fileCountWork = (u32*)ROUND_UP_PTR((u8*)blockCountWork + sizeof(u32), 32);

            vectors[1].base = blockCountWork;
            vectors[1].length = sizeof(u32);
            vectors[2].base = fileCountWork;
            vectors[2].length = sizeof(u32);

            ret = IOS_Ioctlv(__fsFd, FS_IOCTLV_GET_USAGE, 1, 2, vectors);

            if (ret == IPC_RESULT_OK) {
                *blockCountOut = *blockCountWork;
                *fileCountOut = *fileCountWork;
            }
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_GetUsage`: the same vector list,
 * completed by `_isfsFuncCb`, which copies both counts back through the stored context. */
s32 ISFS_GetUsageAsync(const char* path, s32* blockCountOut, s32* fileCountOut, void* callback,
                void* callbackArg)
{
    u32* blockCountWork;
    FSCommandBlock* block;
    u32 len;
    IPCIOVector* vectors;
    u32* fileCountWork;
    char* pathWork;

    if (path == NULL || __fsFd < 0 || blockCountOut == NULL || fileCountOut == NULL ||
        (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_GET_USAGE;

    block->ctx.getUsageCtx.blockCountOut = (u32*)blockCountOut;
    block->ctx.getUsageCtx.fileCountOut = (u32*)fileCountOut;

    vectors = (IPCIOVector*)block->data.ioctlWork;
    pathWork = (char*)ROUND_UP_PTR((u8*)vectors + (sizeof(IPCIOVector) * 3), 32);
    memcpy(pathWork, path, len + 1);

    vectors[0].base = pathWork;
    vectors[0].length = FS_MAX_PATH;

    blockCountWork = (u32*)ROUND_UP_PTR(pathWork + FS_MAX_PATH, 32);
    fileCountWork = (u32*)ROUND_UP_PTR((u8*)blockCountWork + sizeof(u32), 32);

    vectors[1].base = blockCountWork;
    vectors[1].length = sizeof(u32);
    vectors[2].base = fileCountWork;
    vectors[2].length = sizeof(u32);

    return IOS_IoctlvAsync(__fsFd, FS_IOCTLV_GET_USAGE, 1, 2, vectors, _isfsFuncCb, block);
}

/* Create a file with the given owner/group/other permissions and attribute. */
s32 ISFS_CreateFile(const char* path, u32 attr, u32 ownerPerm, u32 groupPerm, u32 otherPerm)
{
    FSCommandBlock* block;
    s32 ret;
    u32 len;

    block = NULL;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            memcpy(block->data.fileIoctl.path, path, len + 1);

            block->data.fileIoctl.attr = attr;
            block->data.fileIoctl.ownerPerm = ownerPerm;
            block->data.fileIoctl.groupPerm = groupPerm;
            block->data.fileIoctl.otherPerm = otherPerm;

            ret = IOS_Ioctl(__fsFd, FS_IOCTL_CREATE_FILE, &block->data.fileIoctl,
                            sizeof(FSFileIoctl), NULL, 0);
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_CreateFile`: the same 0x4C-byte ioctl as `ISFS_CreateFile`,
 * completed by `_isfsFuncCb`. */
s32 ISFS_CreateFileAsync(const char* path, u32 attr, u32 ownerPerm, u32 groupPerm, u32 otherPerm, void* callback,
                void* callbackArg)
{
    FSCommandBlock* block;
    u32 len;

    if (path == NULL || __fsFd < 0 || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    memcpy(block->data.fileIoctl.path, path, len + 1);

    block->data.fileIoctl.attr = attr;
    block->data.fileIoctl.ownerPerm = ownerPerm;
    block->data.fileIoctl.groupPerm = groupPerm;
    block->data.fileIoctl.otherPerm = otherPerm;

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_CREATE_FILE, &block->data.fileIoctl,
                          sizeof(FSFileIoctl), NULL, 0, _isfsFuncCb, block);
}

/* Open a file on `/dev/fs` through the FS command block's work area. */
s32 ISFS_Open(const char* path, u32 mode)
{
    s32 ret;
    u32 len;
    FSCommandBlock* block;

    block = NULL;

    if (path == NULL || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        ret = IPC_RESULT_INVALID;
    } else {
        block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

        if (block == NULL) {
            ret = IPC_RESULT_ALLOC_FAILED;
        } else {
            memcpy(block->data.ioctlWork, path, len + 1);

            ret = IOS_Open((const char*)block->data.ioctlWork, mode);
        }
    }

    if (block != NULL) {
        FS_DELETE(block);
    }

    return ret;
}

/* The async form of `ISFS_Open`. */
s32 ISFS_OpenAsync(const char* path, u32 mode, void* callback, void* callbackArg)
{
    u32 len;
    FSCommandBlock* block;

    if (path == NULL || (len = strnlen(path, FS_MAX_PATH)) == FS_MAX_PATH) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    memcpy(block->data.ioctlWork, path, len + 1);

    return IOS_OpenAsync((const char*)block->data.ioctlWork, mode, _isfsFuncCb, block);
}

/* Move a file descriptor through the FS command block, asynchronously. */
s32 ISFS_SeekAsync(s32 fd, s32 offset, s32 mode, void* callback, void* callbackArg)
{
    FSCommandBlock* block;

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_SeekAsync(fd, offset, mode, _isfsFuncCb, block);
}

/* Read from an open file descriptor. */
s32 ISFS_Read(s32 fd, void* dst, s32 len)
{
    if (dst == NULL || (u32)dst % 32 != 0) {
        return IPC_RESULT_INVALID;
    }

    return IOS_Read(fd, dst, len);
}

/* The async form of `ISFS_Read`. */
s32 ISFS_ReadAsync(s32 fd, void* dst, s32 len, void* callback, void* callbackArg)
{
    FSCommandBlock* block;

    if (dst == NULL || (u32)dst % 32 != 0) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_ReadAsync(fd, dst, len, _isfsFuncCb, block);
}

/* Write to an open file descriptor. */
s32 ISFS_Write(s32 fd, const void* src, s32 len)
{
    if (src == NULL || (u32)src % 32 != 0) {
        return IPC_RESULT_INVALID;
    }

    return IOS_Write(fd, src, len);
}

/* The async form of `ISFS_Write`. */
s32 ISFS_WriteAsync(s32 fd, const void* src, s32 len, void* callback, void* callbackArg)
{
    FSCommandBlock* block;

    if (src == NULL || (u32)src % 32 != 0) {
        return IPC_RESULT_INVALID;
    }

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_WriteAsync(fd, src, len, _isfsFuncCb, block);
}

/* Close an open file descriptor. */
s32 ISFS_Close(s32 fd)
{
    return IOS_Close(fd);
}

/* The async form of `ISFS_Close`. */
s32 ISFS_CloseAsync(s32 fd, void* callback, void* callbackArg)
{
    FSCommandBlock* block;

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);
    if (block == NULL) {
        return IPC_RESULT_BUSY;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_CloseAsync(fd, _isfsFuncCb, block);
}

/* Shut the FS device down asynchronously (the SDK does not check the allocation result here). */
s32 ISFS_ShutdownAsync(void* callback, void* callbackArg)
{
    FSCommandBlock* block;

    block = (FSCommandBlock*)iosAllocAligned(hId, sizeof(FSCommandBlock), 32);

    if (__fsFd < 0) {
        return IPC_RESULT_INVALID;
    }

    block->callback = callback;
    block->callbackArg = callbackArg;
    block->callbackState = CB_STATE_NONE;

    return IOS_IoctlAsync(__fsFd, FS_IOCTL_SHUTDOWN_FS, NULL, 0, NULL, 0, _isfsFuncCb, block);
}
