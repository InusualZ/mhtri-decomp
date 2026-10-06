/*
 * EXI/ProbeBarnacle.c - the Revolution SDK library band at 0x804B17D0..0x804B8020.
 *
 * `.text` 0x804B17D0..0x804B8020 (118 functions / 26704 B, 25928 B of code).  Registered (docs/plan.md 12).
 *
 * Module `EXI` is class-3 evidence: the runtime dump names the range's head `ProbeBarnacle` /
 * `__OSEnableBarnacle` / `EXIWriteReg`, the EXI library's own entry points, and the module keeps the SDK
 * library name the sibling SDK units use (`OS/`, `AX/`, `DWCi/`).  The lib block extended is the existing
 * `OS` block (cflags_os = cflags_base + `-func_align 4`), the same block the AX SDK band sits in; every
 * function start in the band is 16-aligned, which is `-O4,p`'s function alignment, so the source restores
 * it with `#pragma function_align 16` exactly as `AX/AXFXReverbHi.c` does.  Measured flag evidence: the
 * 28 rebuilt functions reproduce the target byte-for-byte under this lib (`recompile.py --measure`), so
 * cflags_os + `function_align 16` is the band's real command line, not a guess.
 *
 * Extent / seam.  The band is one maximal unclaimed run (discovery: `seam: null`), but it is **not one
 * original translation unit** and its functions were never one SDK file.  Read by link order it is five
 * SDK library files: the EXI library's `EXIUart.c` (ProbeBarnacle / __OSEnableBarnacle, 0x804B17D0) and
 * `EXICommon.c` (EXIWriteReg, 0x804B1B10), the FS library's `fs.c` (ISFS_OpenLib .. ISFS_ShutdownAsync,
 * 0x804B1CA0..0x804B32D0) and the GX library's `GXInit.c` / `GXFifo.c` / `GXAttr.c` / ... run
 * (fn_804B32D0..__GetImageTileCount, 0x804B32D0..0x804B8020).  The seam is unproven and unclaimed: the run
 * lands as one unit and the extent settles as its functions match (brief section 2; docs/plan.md 8.3).  The
 * FS half's own `.sdata` state (`__fsFd`, `__fsInitialized`, `__devfs`,
 * `hId`, `lbl_807951B8`) and the GX half's (`__GXData`, `__cpReg`, `__peReg`, `__piReg`,
 * `__memReg` and the two `.data` jump tables) belong to those SDK libraries, not to this range, so they are
 * declared `extern` here and never defined; the data pass owns the claims.
 *
 * NAMES. `GXLoadLightObjImm` (0x804B7C40) is the SDK name, from its callers' shape (a 0x40-byte light object and a
 *   `1 << n` light mask); its declaration is `EXI/ProbeBarnacle.h`.
 *
 * RESIDUALS. flipcheck: `.text` 0x1B20 of 0x6850; `.data` 0x21 of 0x6AC; `.sbss` 0x8 of 0x88; `.sdata` 0x8 of 0x28;
 *   `.bss` (0x6E0) and `.sdata2` (0x70) are claimed and not emitted.  Relocation names that differ from retail:
 *   `ISFS_OpenLib` reads the `.sdata`-pooled `lo$688`/`hi$689` where ours emits no symbol.
 *
 * Measured state (objdiff unit `main/EXI/ProbeBarnacle`, the only row this file moves):
 *   fuzzy_match_percent 25.78 over the band's 25928 B of code; 27 of 118 functions byte-identical
 *   (6396 B / 24.67 %), plus fn_804B2AB0 at 98.97.  Every function of the EXI + FS half is reconstructed:
 *   `.text` 0x804B17D0..0x804B32D0, 7120 B, is complete.
 *
 * Residuals (each one a measurement, not an impression):
 *   - `fn_804B2AB0` (ISFS_GetUsageAsync, 292 B) is 98.97: our object is the same size and every
 *     instruction matches except the two register colours r28/r29 are swapped (the block pointer and the
 *     path-work pointer trade places).  The nesty body is otherwise instruction-identical.
 *   - the **GX half (0x804B32D0..0x804B8020, 88 functions / 18808 B) is unwritten**.  Its functions all go
 *     through the SDK's `__GXData` state record (123 relocations) and the BP/CP/XF register structs
 *     (`__cpReg`, `__peReg`, `__piReg`, `__memReg`), which have to exist as typed records before a body can
 *     be written at all (rule 6 forbids reaching the fields by pointer arithmetic); the record layouts are
 *     the SDK's own (a distro of the same SDK generation is `doldecomp/ogws`, whose `GXAttr.c`,
 *     `GXGeometry.c`, `GXLight.c` and `GXPixel.c` are `Matching`).  That header import is the blocker, and
 *     it is a shared-file item: `gx_hw.h` (the BP/CP/XF/PE register bitfield structs) plus the
 *     `GXData` record.  Two rows of the half need no record at all and are cheap follow-ups:
 *     `GXInvalidateVtxCache` (`GXWGFifo.u8 = 0x48`, `gx.h` already has the union) and
 *     `__GXIsGPFifoReady` (a byte load of `lbl_807951F1`).
 *   - the two `lo`/`hi` IPC-arena statics of `ISFS_OpenLib` are function-local statics in the SDK; the map
 *     spells their symbols `lo$688_807951BC` / `hi$689_807951C0`, which C cannot declare, so this unit
 *     emits them (8 bytes of `.sdata`, a section this unit does not claim).  Every other FS static is a
 *     declaration.
 *
 * Data sections: `.text` only (the unit's block in `configure.py` carries only the `.text` line in
 * `splits.txt`).  `python tools/units/datagap.py --unit EXI/ProbeBarnacle.c` reports the emitted `.sdata`
 * object (`lo`/`hi`) as ours-extra; nothing of the target's data is claimed because none of it is in the
 * band's own ranges.
 *
 * Naming note: the symbol map carries only `fn_XXXXXXXX` for part of this range (checked with
 * `dumpmap.py lookup` over the inventory: `fn_804B1F50`, `fn_804B2050`, `fn_804B21B0`, `fn_804B2AB0`,
 * `fn_804B2CE0`, `fn_804B32D0`, `fn_804B33F0`, `fn_804B4460`, ... - the dump answers a real name for a few
 * of them, but the same dump answers `ISFS_CreateFileAsync` for two different addresses, so its names are
 * not evidence on their own).  The four FS rows the disassembly identifies are named in the comment above
 * each body (`fn_804B1F50` = `ISFS_CreateDirAsync`, `fn_804B2050` = `ISFS_ReadDir`, `fn_804B21B0` =
 * `ISFS_ReadDirAsync`, `fn_804B2AB0` = `ISFS_GetUsageAsync`, `fn_804B2CE0` = `ISFS_CreateFileAsync`); a
 * rename pass can move the map to those names, and the source with it.
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for OSAlarm,
 * but every start in this range is 16-byte aligned, so restore it. */
#pragma function_align 16

#include "types.h"
#include "gx.h"
#include "RVLGX/GXTexture_tail.h"
#include "NAND/nand.h"

/* ---------------------------------------------------------------------------------------------------
 * The SDK entry points the band calls.  They are defined outside the band (the EXI and FS libraries sit
 * in the unclaimed runs below it), so they are bare prototypes here - the lint's rule 2 named gap
 * (docs/plan.md 6.5).
 * --------------------------------------------------------------------------------------------------- */

typedef enum { EXI_CHAN_0, EXI_CHAN_1, EXI_CHAN_2, EXI_MAX_CHAN } EXIChannel;
typedef enum { EXI_READ, EXI_WRITE, EXI_TYPE_2, EXI_MAX_TYPE } EXIType;
typedef enum { EXI_DEV_EXT, EXI_DEV_INT, EXI_DEV_NET, EXI_MAX_DEV } EXIDev;
typedef enum {
    EXI_FREQ_1MHZ,
    EXI_FREQ_2MHZ,
    EXI_FREQ_4MHZ,
    EXI_FREQ_8MHZ,
    EXI_FREQ_16MHZ,
    EXI_FREQ_32HZ,
    EXI_MAX_FREQ
} EXIFreq;
typedef void (*EXICallback)(EXIChannel chan, void* ctx);

BOOL EXIAttach(EXIChannel chan, EXICallback callback);
BOOL EXIDetach(EXIChannel chan);
BOOL EXILock(EXIChannel chan, u32 dev, EXICallback callback);
BOOL EXIUnlock(EXIChannel chan);
BOOL EXISelect(EXIChannel chan, u32 dev, u32 freq);
BOOL EXIDeselect(EXIChannel chan);
BOOL EXIImm(EXIChannel chan, void* buf, s32 len, u32 type, EXICallback callback);
BOOL EXISync(EXIChannel chan);
BOOL EXIGetID(EXIChannel chan, u32 dev, u32* out);

/* The EXI library statics the barnacle records its result in: the channel/device it was last enabled on
 * and the two `0xA5FF005A` flags the EUART path reads.  They live outside the band (the EXI library's own
 * .sdata), so they are declarations, never definitions. */
extern u32 lbl_807951AC;
extern u32 lbl_807951A8;
extern u32 lbl_807951A4;
extern u32 lbl_807951A0;

/* The device IDs the barnacle probe recognises (the SDK's EXIDeviceID). */
typedef enum {
    EXI_ID_MEMCARD_59 = 0x00000004,
    EXI_ID_MEMCARD_123 = 0x00000008,
    EXI_ID_MEMCARD_251 = 0x00000010,
    EXI_ID_MEMCARD_507 = 0x00000020,
    EXI_ID_USB_ADAPTER = 0x01010000,
    EXI_ID_BROADBAND_ADAPTER = 0x04020200,
    EXI_ID_INVALID = 0xFFFFFFFF
} EXIDeviceID;

/* Probe a device for the barnacle protocol: attach, lock, select, send the probe command word and read the
 * device's answer.  Returns whether the device answered with a valid (not `EXI_ID_INVALID`) ID. */
BOOL ProbeBarnacle(EXIChannel chan, u32 dev, u32* id)
{
    BOOL error;
    u32 cmd;

    if (chan != EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        if (!EXIAttach(chan, NULL)) {
            return FALSE;
        }
    }

    error = !EXILock(chan, dev, NULL);
    if (!error) {
        error = !EXISelect(chan, dev, EXI_FREQ_1MHZ);
        if (!error) {
            cmd = 0x20011300;
            error = !EXIImm(chan, &cmd, sizeof(u32), EXI_WRITE, NULL);
            error |= !EXISync(chan);
            error |= !EXIImm(chan, id, sizeof(u32), EXI_READ, NULL);
            error |= !EXISync(chan);
            error |= !EXIDeselect(chan);
        }
        EXIUnlock(chan);
    }

    if (chan != EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        EXIDetach(chan);
    }

    if (error) {
        return FALSE;
    }

    return *id != EXI_ID_INVALID;
}

/* Enable the barnacle on a channel/device pair the SDK already knows is present, unless the device is one
 * of the IDs that does not answer the probe protocol. */
void __OSEnableBarnacle(EXIChannel chan, u32 dev)
{
    u32 id;

    if (!EXIGetID(chan, dev, &id)) {
        return;
    }

    switch (id) {
    case EXI_ID_MEMCARD_59:
    case EXI_ID_MEMCARD_123:
    case EXI_ID_MEMCARD_251:
    case EXI_ID_MEMCARD_507:
    case EXI_ID_USB_ADAPTER:
    case 0x01020000:
    case 0x02020000:
    case 0x03010000:
    case 0x04020100:
    case EXI_ID_BROADBAND_ADAPTER:
    case 0x04020300:
    case 0x04040404:
    case 0x04060000:
    case 0x04120000:
    case 0x04130000:
    case 0x04220000:
    case 0x80000004:
    case 0x80000008:
    case 0x80000010:
    case 0x80000020:
    case EXI_ID_INVALID:
        break;
    default:
        if (ProbeBarnacle(chan, dev, &id)) {
            lbl_807951AC = chan;
            lbl_807951A8 = dev;
            lbl_807951A4 = 0xA5FF005A;
            lbl_807951A0 = 0xA5FF005A;
        }
        break;
    }
}

/* Write a command plus a 1-, 2- or 4-byte big-endian word to a register, checking every EXI step. */
BOOL EXIWriteReg(EXIChannel chan, u32 dev, u32 cmd, const void* buf, s32 len)
{
    BOOL error = FALSE;
    u32 write_val;

    switch (len) {
    case 1:
        write_val = *(u8*)buf << 24;
        break;
    case 2:
        write_val = *(u16*)buf << 24 | (*(u16*)buf & 0xFF00) << 8;
        break;
    default:
        write_val = *(u32*)buf >> 24 & 0x000000FF | *(u32*)buf >> 8 & 0x0000FF00 |
                    *(u32*)buf << 8 & 0x00FF0000 | *(u32*)buf << 24 & 0xFF000000;
        break;
    }

    error |= !EXILock(chan, dev, NULL);
    if (error) {
        return FALSE;
    }

    error |= !EXISelect(chan, dev, EXI_FREQ_16MHZ);
    if (error) {
        EXIUnlock(chan);
        return FALSE;
    }

    error |= !EXIImm(chan, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIImm(chan, &write_val, sizeof(write_val), EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    error |= !EXIUnlock(chan);

    return error == FALSE;
}

/* ===================================================================================================
 * FS/ISFS - the SDK file-system library's entry points at 0x804B1CA0..0x804B32D0.
 *
 * The library keeps one IPC heap and one `/dev/fs` fd, hands every ioctl a 0x140-byte command block out
 * of that heap and completes the async ones through `_isfsFuncCb`.  The map's `fn_804B...` names are kept
 * for the five entry points the map does not name (Naming note above); the comment on each one names
 * the SDK function the disassembly proves it is (the ioctl number and argument shape).
 * =================================================================================================== */

#include "Runtime.PPCEABI.H/memcpy.h"

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

/* The FS library's own state: the `/dev/fs` fd, the heap id, the initialised flag, the reserved IPC
 * arena window and the outstanding-async counter.  They live outside the band (the FS library's .sdata
 * at 0x80793E38 / 0x807951B0, an unclaimed auto range), so they are declarations, never definitions -
 * except `lo`/`hi`, which the map spells with MWCC's local-static suffix and which therefore cannot be
 * declared from C (the two 4-byte .sdata words this unit emits). */
extern s32 __fsFd;
extern u32 __fsInitialized;
extern char* __devfs;
extern s32 hId;
extern u32 lbl_807951B8;

/* The GX library's GP-FIFO-ready byte flag (.sdata, outside the band). */
extern u8 lbl_807951F1;

char* strcpy(char* dst, const char* src);

/* Open the `/dev/fs` device, carve the IPC arena window for it and create the FS heap. */
s32 ISFS_OpenLib(void)
{
    static void* lo;
    static void* hi;

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

    lbl_807951B8 = 0;

    if (block->callback != NULL) {
        ((void (*)(s32, void*))block->callback)(result, block->callbackArg);
    }

    FS_DELETE(block);
    return result;
}

/* fn_804B1F50 is the SDK's `ISFS_CreateDirAsync`: ioctl 3, the 0x4C-byte FSFileIoctl, completion
 * callback `_isfsFuncCb`. */
s32 fn_804B1F50(const char* path, u32 attr, u32 ownerPerm, u32 groupPerm, u32 otherPerm, void* callback,
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

/* fn_804B2050 is the SDK's `ISFS_ReadDir`: ioctlv 4, a 2-in/2-out vector list built in the work area. */
s32 fn_804B2050(const char* path, char* filesOut, u32* fileCountOut)
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

/* fn_804B21B0 is the SDK's `ISFS_ReadDirAsync`: the same vector list as `fn_804B2050`, completed by
 * `_isfsFuncCb`, which copies the file count back through the stored context. */
s32 fn_804B21B0(const char* path, char* filesOut, u32* fileCountOut, void* callback, void* callbackArg)
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

/* fn_804B2AB0 is the SDK's `ISFS_GetUsageAsync`: the same 1-in/2-out vector list as `ISFS_GetUsage`,
 * completed by `_isfsFuncCb`, which copies both counts back through the stored context. */
s32 fn_804B2AB0(const char* path, s32* blockCountOut, s32* fileCountOut, void* callback,
                void* callbackArg)
{
    FSCommandBlock* block;
    u32* blockCountWork;
    u32* fileCountWork;
    char* pathWork;
    IPCIOVector* vectors;
    u32 len;

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

/* fn_804B2CE0 is the SDK's `ISFS_CreateFileAsync`: the same 0x4C-byte ioctl as `ISFS_CreateFile`,
 * completed by `_isfsFuncCb`. */
s32 fn_804B2CE0(const char* path, u32 attr, u32 ownerPerm, u32 groupPerm, u32 otherPerm, void* callback,
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

    return fn_804BC3F0(fd, dst, len);
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

    return fn_804BC600(fd, src, len);
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

/* ===================================================================================================
 * GX - the start of the band's GX library half.  Only the two rows that need no register record are
 * reconstructed; every other GX body (0x804B32D0..0x804B8020) needs `__GXData` and the BP/CP/XF/PE
 * records as typed structs (see the shared-file request in the worker's outbox and the header above).
 * =================================================================================================== */

/* Whether the GP FIFO is ready (the byte flag the FIFO setup leaves set). */
BOOL __GXIsGPFifoReady(void)
{
    return lbl_807951F1;
}

/* Tell the hardware the vertex cache's contents are stale (a write-gather-pipe command). */
void GXInvalidateVtxCache(void)
{
    GXWGFifo.u8 = 0x48;
}
