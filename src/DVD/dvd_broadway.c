/*
 * DVD/dvd_broadway.c - the SDK DVD low-level driver for the Broadway IOS `/dev/di` (DVDLow*).
 *
 * RANGE. `.text` 0x804AC1D0-0x804AE880; `.data` 0x806195A0-0x8061A4A8; `.bss` 0x807469A0-0x80746B60; `.sdata`
 *   0x80793E08-0x80793E18; `.sbss` 0x80795158-0x80795180 (32 functions / 0x25FC B).
 *   - `.data` is one pooled block of every string the source names, in codegen order (the target's `@3815`);
 *     `.sdata` holds `diFd` and the `/dev/di` literal; `.bss` is one pooled block of the file statics in
 *     first-use order; `.sbss` is the scalar statics in reverse declaration order.
 * FLAGS. `cflags_base` in the `OS` lib block of `configure.py` (`Wii/1.3`); no pragma. Function starts are 16-byte
 *   aligned as in the target (0x804AD8C0 (0xC) is followed by 0x804AD8D0).
 * NAMES. the unit name is a GUESS (the driver's file name in the SDK scheme). The dead functions' names are their
 *   own report strings (the `%s` ones are `__func__`); GUESS: `initDvdContexts`, `doImmBufCallback`,
 *   `allocateBuffers`, `partitionResultBuf`, `DVDContext.issued`, every dead function's parameter list and opcode,
 *   and the map's DVDLowGetStatusRegister / DVDLowGetControlRegister / DVDLowTestAlarm.
 * RESIDUALS.
 *   - every live function's instructions equal the target's; `.data` and `.bss` differ in objdiff only by map naming:
 *     the `.data` pool is split into the `@3815` / `lbl_806195D2` / `lbl_80619600` rows and the `.bss` row `ioVec`
 *     is sized 0xC0 over the open-partition result buffer at `ioVec+0x60`, so an `addi rX,r31,off` string load or
 *     the `+0x120` buffer address pairs against a different name. A map request (one pool symbol, `ioVec` 0x60 +
 *     `partitionResultBuf` 0x60) brings `.data` and `.bss` to full; the `.text` rows stay put.
 *   - the object carries the 18 dead-stripped DVDLow* functions plus the dead statics `initDvdContexts` /
 *     `doImmBufCallback` (+0x1D58 `.text` with alignment); mwld is expected to strip them on a flip (unmeasured;
 *     flipcheck refuses on the `.text` size); the `.data` pool they fill must survive the strip.
 *   - the dead functions' bodies are unmeasured guesses; only their string literals are checked.
 *   - the target's `.data` (2 B) and `.sbss` (7 B) tails are link padding our object does not carry.
 * SHAPES.
 *   - the dead functions' definition order and the order of their literals (and `__func__` uses) are load-bearing:
 *     they lay out the `.data` pool that the live `addi rX,r31,off` loads index.
 *   - the live bodies' odd expressions and misattributed messages are retail's: `((u32)low + 31) & 31`,
 *     `numCertBytes`/`numTmdBytes` passed twice, the tmd check's certificates message, `doCoverCallback`'s message.
 *   - `newContext`, `nextCommandBuf`, `ddrAllocAligned32` and `allocateBuffers` are `static inline`: their strings
 *     land in the pool at the first expansion.
 *   - `initDvdContexts` is a plain `static` defined before the callbacks: its out-of-line (dead-stripped) copy is
 *     the first code to touch `dvdContexts`, which puts it first in the `.bss` pool.
 *   - the live `doCoverCallback` reports `doTransactionCallback`'s message (target: `@3815+0`); the
 *     `(doCoverCallback)` message belongs to the dead `doImmBufCallback`.
 *   - `(u8)(inUse != FALSE) == TRUE` gives the target's `neg`/`or`/`srwi`/`cmplwi`; `newContext` reads
 *     `contextIndex` directly (a cached copy swaps the index and constant registers).
 *   - the open-partition lengths are `if`/`else`, not a conditional expression.
 */
#include "DVD/dvd_broadway.h"

#include "IPC/ipcclt.h"
#include "IPC/ipcMain.h"
#include "MSL_C/string.h"
#include "OS/OSError.h"

#define DVD_CONTEXT_MAGIC 0xFEEBDAED
#define DVD_CONTEXT_COUNT 4
#define DI_COMMAND_COUNT 4

/* One in-flight DI request: the callback to run and the bookkeeping `doTransactionCallback` checks. size: 0x20 */
typedef struct DVDContext {
    /* +0x00 */ DVDLowCallback callback;
    /* +0x04 */ BOOL issued;
    /* +0x08 */ u8 inUse;
    /* +0x0C */ u32 magic;
    /* +0x10 */ u32 contextNum;
    /* +0x14 */ u8 pad_0x14[0xC];
} DVDContext;

/* The command block an ioctl sends to `/dev/di`. size: 0x20 */
typedef struct DICommand {
    /* +0x00 */ u8 command;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ u32 args[7];
} DICommand;

/* The drive registers the last commands returned. size: 0x20 */
typedef struct DIRegisterValues {
    /* +0x00 */ u32 immBuf;
    /* +0x04 */ u32 coverReg;
    /* +0x08 */ u8 pad_0x08[0x18];
} DIRegisterValues;

static u8 resetOccurred;
static DICommand* diCommand;
static char* pathBuf;
static u32 readLength;
static u32 spinUpValue;
static u8 diOpened;
static u8 contextsInitialized;
static s32 contextIndex;
static s32 commandBufIndex;
static u8 callbackInProgress;
static u8 requestPending;

static s32 diFd = -1;

static DVDContext dvdContexts[DVD_CONTEXT_COUNT] __attribute__((aligned(32)));
static DIRegisterValues DIRegValues __attribute__((aligned(32)));
static u32 CoverRegBuf[8] __attribute__((aligned(32)));
static IPCIOVector ioVec[12] __attribute__((aligned(32)));
static u8 partitionResultBuf[0x60] __attribute__((aligned(32)));
static u32 StatusRegBuf[8] __attribute__((aligned(32)));
static u32 ControlRegBuf[8] __attribute__((aligned(32)));

/* Resets the context ring: every context free, carrying the magic and its own index. */
static void initDvdContexts(void) {
    s32 i;

    for (i = 0; i < DVD_CONTEXT_COUNT; i++) {
        dvdContexts[i].callback = NULL;
        dvdContexts[i].issued = FALSE;
        dvdContexts[i].inUse = FALSE;
        dvdContexts[i].magic = DVD_CONTEXT_MAGIC;
        dvdContexts[i].contextNum = i;
    }
    contextIndex = 0;
}

/* Runs a finished command's callback with its result, flagging a reset that happened meanwhile. */
static s32 doTransactionCallback(s32 result, DVDContext* context) {
    if (context->magic != DVD_CONTEXT_MAGIC) {
        OSReport("(doTransactionCallback) Error - context mangled!\n");
        context->magic = DVD_CONTEXT_MAGIC;
    } else {
        requestPending = FALSE;
        if (context->callback != NULL) {
            callbackInProgress = TRUE;
            if (resetOccurred == TRUE) {
                resetOccurred = FALSE;
                result |= 8;
            }
            if (result & 1) {
                readLength = 0;
            }
            context->callback(result);
            callbackInProgress = FALSE;
        }
    }
    context->inUse = FALSE;
    return 0;
}

/* Stores the cover register a cover-register read returned, then runs the command's callback. */
static s32 doCoverCallback(s32 result, DVDContext* context) {
    requestPending = FALSE;
    DIRegValues.coverReg = CoverRegBuf[0];
    if (context->magic != DVD_CONTEXT_MAGIC) {
        OSReport("(doTransactionCallback) Error - context mangled!\n");
        context->magic = DVD_CONTEXT_MAGIC;
    } else if (context->callback != NULL) {
        callbackInProgress = TRUE;
        if (resetOccurred == TRUE) {
            resetOccurred = FALSE;
            result |= 8;
        }
        context->callback(result);
        callbackInProgress = FALSE;
    }
    context->inUse = FALSE;
    return 0;
}

/* Stores the immediate buffer register a command returned, then runs the command's callback. */
static s32 doImmBufCallback(s32 result, DVDContext* context) {
    requestPending = FALSE;
    DIRegValues.immBuf = CoverRegBuf[0];
    if (context->magic != DVD_CONTEXT_MAGIC) {
        OSReport("(doCoverCallback) Error - context mangled!\n");
        context->magic = DVD_CONTEXT_MAGIC;
    } else if (context->callback != NULL) {
        callbackInProgress = TRUE;
        context->callback(result);
        callbackInProgress = FALSE;
    }
    context->inUse = FALSE;
    return 0;
}

/* 0x804AC350 (0x50): closes the DI device. */
BOOL DVDLowFinish(void) {
    if (IOS_Close(diFd) != 0) {
        OSReport("(DVDLowFinish) Error: IOS_Close failed\n");
        return FALSE;
    }
    diOpened = FALSE;
    return TRUE;
}

/* Carves `size` bytes off the low end of the IPC buffer. */
static inline u8* ddrAllocAligned32(u32 size) {
    u8* low = IPCGetBufferLo();
    u8* high = IPCGetBufferHi();
    if ((u32)low & 31) {
        low = (u8*)(((u32)low + 31) & 31);
    }
    if (low + size > high) {
        OSReport("(ddrAllocAligned32) Not enough space to allocate %d bytes\n", size);
    }
    IPCSetBufferLo(low + size);
    return low;
}

/* Allocates the command blocks and the path buffer from the IPC buffer. */
static inline BOOL allocateBuffers(void) {
    diCommand = (DICommand*)ddrAllocAligned32(sizeof(DICommand) * DI_COMMAND_COUNT);
    if (diCommand == NULL) {
        OSReport("Allocation of diCommand blocks failed\n");
        return FALSE;
    }
    pathBuf = (char*)ddrAllocAligned32(32);
    if (pathBuf == NULL) {
        OSReport("Allocation of pathBuf failed\n");
        return FALSE;
    }
    return TRUE;
}

/* 0x804AC3A0 (0x264): opens the DI device, allocating its buffers and contexts on the first call. */
BOOL DVDLowInit(void) {
    s32 result;

    if (!diOpened) {
        diOpened = TRUE;
        result = IPCCltInit();
        if (result != 0) {
            OSReport("IPCCltInit returned error: %d\n", result);
            return FALSE;
        }
        if (!allocateBuffers()) {
            return FALSE;
        }
        if (!contextsInitialized) {
            initDvdContexts();
            contextsInitialized = TRUE;
        }
    }
    strncpy(pathBuf, "/dev/di", 32);
    diFd = IOS_Open(pathBuf, 0);
    if (diFd >= 0) {
        return TRUE;
    }
    switch (diFd) {
    case -6:
        OSReport("(DVDLowInit) Error: IOS_Open failed - pathname '/dev/di' does not exist\n");
        return FALSE;
    case -1:
        OSReport("(DVDLowInit) Error: IOS_Open failed - calling thread lacks permission\n");
        return FALSE;
    case -5:
        OSReport("(DVDLowInit) Error: IOS_Open failed - connection limit has been reached\n");
        return FALSE;
    default:
        OSReport("(DVDLowInit) IOS_Open failed, errorcode = %d\n", diFd);
        return FALSE;
    }
}

/* Takes the next context of the ring for a command that reports to `callback`; spins when the ring is corrupt. */
static inline DVDContext* newContext(DVDLowCallback callback) {
    s32 returnIndex;

    requestPending = TRUE;
    if ((u8)(dvdContexts[contextIndex].inUse != FALSE) == TRUE) {
        OSReport("(newContext) ERROR: freeDvdContext.inUse (#%d) is true\n", contextIndex);
        OSReport("(newContext) Now spinning in infinite loop\n");
        while (TRUE) {
        }
    }
    if (dvdContexts[contextIndex].magic != DVD_CONTEXT_MAGIC) {
        OSReport("(newContext) Something overwrote the context magic - spinning \n");
        while (TRUE) {
        }
    }
    dvdContexts[contextIndex].callback = callback;
    dvdContexts[contextIndex].issued = TRUE;
    dvdContexts[contextIndex].inUse = TRUE;
    returnIndex = contextIndex;
    contextIndex++;
    if (contextIndex >= DVD_CONTEXT_COUNT) {
        contextIndex = 0;
    }
    return &dvdContexts[returnIndex];
}

/* Advances the command-block ring. */
static inline void nextCommandBuf(void) {
    commandBufIndex++;
    if (commandBufIndex >= DI_COMMAND_COUNT) {
        commandBufIndex = 0;
    }
}

/* 0x804AC610 (0x18C): reads the disc id into `diskId`. */
BOOL DVDLowReadDiskID(DVDDiskID* diskId, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    if (diskId == NULL) {
        OSReport("@@@@@@ WARNING - Calling DVDLowReadDiskId with NULL ptr\n");
    }
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x70;
    result = IOS_IoctlAsync(diFd, 0x70, &diCommand[commandBufIndex], sizeof(DICommand), diskId, sizeof(DVDDiskID),
                            doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowReadDiskID) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AC7A0 (0x26C): opens the partition at `offset` with its ticket and certificates; `tmd` receives the TMD. */
/* untyped: byte ranges the caller owns (ticket, certificates and TMD) */
BOOL DVDLowOpenPartition(u32 offset, void* ticket, u32 certsLength, void* certs, void* tmd, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (ticket != NULL && ((u32)ticket & 31)) {
        OSReport("(DVDLowOpenPartition) eTicket memory is unaligned\n");
        return FALSE;
    }
    if (certs != NULL && ((u32)certs & 31)) {
        OSReport("(DVDLowOpenPartition) certificates memory is unaligned\n");
        return FALSE;
    }
    if (tmd != NULL && ((u32)tmd & 31)) {
        OSReport("(DVDLowOpenPartition) certificates memory is unaligned\n");
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x8B;
    diCommand[commandBufIndex].args[0] = offset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = ticket;
    if (ticket == NULL) {
        ioVec[1].length = 0;
    } else {
        ioVec[1].length = sizeof(ESTicket);
    }
    ioVec[2].base = certs;
    if (certs == NULL) {
        ioVec[2].length = 0;
    } else {
        ioVec[2].length = certsLength;
    }
    ioVec[3].base = tmd;
    ioVec[3].length = 0x49E4;
    ioVec[4].base = partitionResultBuf;
    ioVec[4].length = 0x20;
    result = IOS_IoctlvAsync(diFd, 0x8B, 3, 2, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Opens the partition at `offset` with an explicit TMD and ticket. */
/* untyped: byte ranges the caller owns (ticket, TMD and certificates) */
BOOL DVDLowOpenPartitionWithTmdAndTicket(u32 offset, void* ticket, u32 tmdSize, void* tmd, u32 certsLength, void* certs,
                                         DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (certs != NULL && ((u32)certs & 31)) {
        return FALSE;
    }
    if (ticket != NULL && ((u32)ticket & 31)) {
        OSReport("(%s) eTicket memory is unaligned\n", __func__);
        return FALSE;
    }
    if (tmd == NULL) {
        OSReport("(%s) tmd parameter cannot be NULL\n", __func__);
        return FALSE;
    }
    if ((u32)tmd & 31) {
        OSReport("(%s) tmd memory is unaligned\n", __func__);
        return FALSE;
    }
    if (ticket == NULL) {
        OSReport("(%s) eTicket parameter cannot be NULL\n", __func__);
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x93;
    diCommand[commandBufIndex].args[0] = offset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = ticket;
    ioVec[1].length = sizeof(ESTicket);
    ioVec[2].base = tmd;
    ioVec[2].length = tmdSize;
    ioVec[3].base = certs;
    if (certs == NULL) {
        ioVec[3].length = 0;
    } else {
        ioVec[3].length = certsLength;
    }
    ioVec[4].base = partitionResultBuf;
    ioVec[4].length = 0x20;
    result = IOS_IoctlvAsync(diFd, 0x93, 4, 1, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804ACA10 (0x264): opens the partition at `offset` with an explicit TMD and ticket view. */
/* untyped: byte ranges the caller owns (TMD and certificates) */
BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, ESTicketView* view, u32 tmdSize, void* tmd, u32 certsLength,
                                             void* certs, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (certs != NULL && ((u32)certs & 31)) {
        return FALSE;
    }
    if (tmd == NULL) {
        OSReport("(%s) tmd parameter cannot be NULL\n", __func__);
        return FALSE;
    }
    if ((u32)tmd & 31) {
        OSReport("(%s) tmd memory is unaligned\n", __func__);
        return FALSE;
    }
    if (view == NULL) {
        OSReport("(%s) eTicketView parameter cannot be NULL\n", __func__);
        return FALSE;
    }
    if ((u32)view & 31) {
        OSReport("(%s) eTicketView memory is unaligned\n", __func__);
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x94;
    diCommand[commandBufIndex].args[0] = offset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = view;
    ioVec[1].length = sizeof(ESTicketView);
    ioVec[2].base = tmd;
    ioVec[2].length = tmdSize;
    ioVec[3].base = certs;
    if (certs == NULL) {
        ioVec[3].length = 0;
    } else {
        ioVec[3].length = certsLength;
    }
    ioVec[4].base = partitionResultBuf;
    ioVec[4].length = 0x20;
    result = IOS_IoctlvAsync(diFd, 0x94, 4, 1, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804ACC80 (0x210): asks the drive for the TMD and certificate sizes of the partition at `offset`. */
BOOL DVDLowGetNoDiscBufferSizes(u32 offset, u32* numTmdBytes, u32* numCertBytes, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (numTmdBytes == NULL || numCertBytes == NULL) {
        OSReport("(%s) Error: NULL pointer argument\n", __func__);
        return FALSE;
    }
    if ((u32)numTmdBytes & 31) {
        OSReport("(%s) numTmdBytes memory is unaligned\n", __func__);
        return FALSE;
    }
    if ((u32)numCertBytes & 31) {
        OSReport("(%s) certificates memory is unaligned\n", __func__);
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x92;
    diCommand[commandBufIndex].args[0] = offset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = numTmdBytes;
    ioVec[1].length = sizeof(u32);
    ioVec[2].base = numCertBytes;
    ioVec[2].length = sizeof(u32);
    result = IOS_IoctlvAsync(diFd, 0x92, 1, 2, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (%s) IOS_IoctlvAsync returned error: %d\n", __func__, result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804ACE90 (0x298): reads the ticket, TMD, certificates, data offset and H3 table of the partition at `offset`. */
BOOL DVDLowGetNoDiscOpenPartitionParams(u32 offset, ESTicket* ticket, u32* numTmdBytes, u8* tmd, u32* numCertBytes,
                                        u8* certs, u32* dataWordOffset, u8* h3Table, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (ticket == NULL || numTmdBytes == NULL || tmd == NULL || numCertBytes == NULL || certs == NULL ||
        dataWordOffset == NULL || h3Table == NULL) {
        OSReport("(%s) Error: NULL pointer argument\n", __func__);
        return FALSE;
    }
    if (((u32)ticket & 31) || ((u32)numTmdBytes & 31) || ((u32)tmd & 31) || ((u32)numCertBytes & 31) ||
        ((u32)certs & 31) || ((u32)dataWordOffset & 31) || ((u32)h3Table & 31)) {
        OSReport("(%s) pointer argument is unaligned\n", __func__);
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x90;
    diCommand[commandBufIndex].args[0] = offset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = numTmdBytes;
    ioVec[1].length = sizeof(u32);
    ioVec[2].base = numCertBytes;
    ioVec[2].length = sizeof(u32);
    ioVec[3].base = ticket;
    ioVec[3].length = sizeof(ESTicket);
    ioVec[4].base = numTmdBytes;
    ioVec[4].length = sizeof(u32);
    ioVec[5].base = tmd;
    ioVec[5].length = *numTmdBytes;
    ioVec[6].base = numCertBytes;
    ioVec[6].length = sizeof(u32);
    ioVec[7].base = certs;
    ioVec[7].length = *numCertBytes;
    ioVec[8].base = dataWordOffset;
    ioVec[8].length = sizeof(u32);
    ioVec[9].base = h3Table;
    ioVec[9].length = 0x18000;
    result = IOS_IoctlvAsync(diFd, 0x90, 3, 7, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (%s) IOS_IoctlvAsync returned error: %d\n", __func__, result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Opens the partition at `offset` from the parameters `DVDLowGetNoDiscOpenPartitionParams` read. */
BOOL DVDLowNoDiscOpenPartition(u32 offset, ESTicket* ticket, u32 numCertBytes, u8* certs, u32 numTmdBytes, u8* tmd,
                               u32 dataWordOffset, u8* h3Table, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if (ticket == NULL || certs == NULL || tmd == NULL || h3Table == NULL) {
        OSReport("(%s) Error: NULL pointer argument\n", __func__);
        return FALSE;
    }
    if (((u32)ticket & 31) || ((u32)certs & 31) || ((u32)tmd & 31) || ((u32)h3Table & 31)) {
        OSReport("(%s) pointer argument is unaligned\n", __func__);
        return FALSE;
    }
    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x91;
    diCommand[commandBufIndex].args[0] = offset;
    diCommand[commandBufIndex].args[1] = dataWordOffset;
    ioVec[0].base = &diCommand[commandBufIndex];
    ioVec[0].length = sizeof(DICommand);
    ioVec[1].base = ticket;
    ioVec[1].length = sizeof(ESTicket);
    ioVec[2].base = certs;
    ioVec[2].length = numCertBytes;
    ioVec[3].base = tmd;
    ioVec[3].length = numTmdBytes;
    ioVec[4].base = h3Table;
    ioVec[4].length = 0x18000;
    ioVec[5].base = partitionResultBuf;
    ioVec[5].length = 0x20;
    result = IOS_IoctlvAsync(diFd, 0x91, 5, 1, ioVec, doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (%s) IOS_IoctlvAsync returned error: %d\n", __func__, result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD130 (0x168): closes the open partition. */
BOOL DVDLowClosePartition(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x8C;
    context = newContext(callback);
    result = IOS_IoctlAsync(diFd, 0x8C, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, doTransactionCallback,
                            context);
    if (result != 0) {
        OSReport("@@@ (DVDLowClosePartition) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD2A0 (0x198): reads `length` bytes at `offset` without decryption. */
/* untyped: byte range the read fills */
BOOL DVDLowUnencryptedRead(void* buffer, u32 length, u32 offset, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    readLength = length;
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x8D;
    diCommand[commandBufIndex].args[0] = length;
    diCommand[commandBufIndex].args[1] = offset;
    result = IOS_IoctlAsync(diFd, 0x8D, &diCommand[commandBufIndex], sizeof(DICommand), buffer, length,
                            doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowUnencryptedRead) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD440 (0x198): stops the drive motor. */
BOOL DVDLowStopMotor(u32 eject, u32 saving, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xE3;
    diCommand[commandBufIndex].args[0] = eject;
    diCommand[commandBufIndex].args[1] = saving;
    result = IOS_IoctlAsync(diFd, 0xE3, &diCommand[commandBufIndex], sizeof(DICommand), &DIRegValues,
                            sizeof(DIRegisterValues), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowStopMotor) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Waits for the drive cover to close. */
BOOL DVDLowWaitForCoverClose(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x79;
    result = IOS_IoctlAsync(diFd, 0x79, &diCommand[commandBufIndex], sizeof(DICommand), &DIRegValues,
                            sizeof(DIRegisterValues), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowWaitForCoverClose) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD5E0 (0x16C): reads the drive's inquiry record into `info`. */
BOOL DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x12;
    result = IOS_IoctlAsync(diFd, 0x12, &diCommand[commandBufIndex], sizeof(DICommand), info, sizeof(DVDDriveInfo),
                            doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowInquiry) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD750 (0x168): asks the drive for its error code. */
BOOL DVDLowRequestError(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xE0;
    result = IOS_IoctlAsync(diFd, 0xE0, &diCommand[commandBufIndex], sizeof(DICommand), &DIRegValues,
                            sizeof(DIRegisterValues), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowRequestError) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD8C0 (0xC): records whether the next reset spins the disc up. */
BOOL DVDLowSetSpinupFlag(u32 spinUp) {
    spinUpValue = spinUp;
    return TRUE;
}

/* Tells the drive a reset is coming; refused from inside a command callback. */
BOOL DVDLowNotifyReset(void) {
    s32 result;

    if (callbackInProgress) {
        OSReport("(DVDLowSetSpinupFlag): Synch functions can't be called in callbacks\n");
        return FALSE;
    }
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x7E;
    result = IOS_IoctlAsync(diFd, 0x7E, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, NULL, NULL);
    if (result != 0) {
        OSReport("@@@ (DVDLowNotifyReset) IOS_IoctlAsync returned error: %d\n", result);
        return FALSE;
    }
    return TRUE;
}

/* 0x804AD8D0 (0x17C): resets the drive, spinning up when the spin-up flag is set. */
BOOL DVDLowReset(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x8A;
    diCommand[commandBufIndex].args[0] = spinUpValue;
    result = IOS_IoctlAsync(diFd, 0x8A, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, doTransactionCallback,
                            context);
    if (result != 0) {
        OSReport("@@@ (DVDLowReset) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804ADA50 (0x198): configures the drive's audio streaming buffer. */
BOOL DVDLowAudioBufferConfig(u32 enable, u32 size, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xE4;
    diCommand[commandBufIndex].args[0] = enable;
    diCommand[commandBufIndex].args[1] = size;
    result = IOS_IoctlAsync(diFd, 0xE4, &diCommand[commandBufIndex], sizeof(DICommand), &DIRegValues,
                            sizeof(DIRegisterValues), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowAudioBufferConfig) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Reads the cover status synchronously into the register copy; refused from inside a command callback. */
BOOL DVDLowGetCoverStatus(void) {
    s32 result;

    if (callbackInProgress) {
        OSReport("(DVDLowGetCoverStatus): Synch functions can't be called in callbacks\n");
        return FALSE;
    }
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x88;
    result = IOS_Ioctl(diFd, 0x88, &diCommand[commandBufIndex], sizeof(DICommand), &DIRegValues,
                       sizeof(DIRegisterValues));
    if (result != 0) {
        OSReport("@@@ (DVDLowGetCoverStatus) IOS_Ioctl returned error: %d\n", result);
        return FALSE;
    }
    return TRUE;
}

/* Issues one DI command with two arguments and an output buffer, reporting a failed ioctl under `name`. */
#define DVD_LOW_COMMAND2(name, opcode, a0, a1, out, outSize)                                                        \
    DVDContext* context;                                                                                           \
    s32 result;                                                                                                    \
                                                                                                                   \
    context = newContext(callback);                                                                                \
    nextCommandBuf();                                                                                              \
    diCommand[commandBufIndex].command = opcode;                                                                   \
    diCommand[commandBufIndex].args[0] = a0;                                                                       \
    diCommand[commandBufIndex].args[1] = a1;                                                                       \
    result = IOS_IoctlAsync(diFd, opcode, &diCommand[commandBufIndex], sizeof(DICommand), out, outSize,            \
                            doTransactionCallback, context);                                                       \
    if (result != 0) {                                                                                             \
        OSReport("@@@ (" name ") IOS_IoctlAsync returned error: %d\n", result);                                    \
        context->inUse = FALSE;                                                                                    \
        return FALSE;                                                                                              \
    }                                                                                                              \
    return TRUE;

/* Reads `length` DVD-video sectors at `lsn` into `buffer`. */
BOOL DVDLowReadDVD(u8* buffer, u32 length, u32 lsn, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDVD", 0xD0, length, lsn, buffer, length)
}

/* Reads the DVD-video configuration record of `layer` into `buffer`. */
BOOL DVDLowReadDVDConfig(u8* buffer, u32 layer, u32 lsn, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDVDConfig", 0xD1, layer, lsn, buffer, 0x20)
}

/* Reads the copyright record of `layer` into `buffer`. */
BOOL DVDLowReadDvdCopyright(u8* buffer, u32 layer, u32 unused, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDvdCopyright", 0xD2, layer, unused, buffer, 0x20)
}

/* Reads the physical-format record of `layer` into `buffer`. */
BOOL DVDLowReadDvdPhysical(u8* buffer, u32 layer, u32 unused, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDvdPhysical", 0xD3, layer, unused, buffer, 0x20)
}

/* Reads the disc key of `layer` into `buffer`. */
BOOL DVDLowReadDvdDiscKey(u8* buffer, u32 layer, u32 unused, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDvdDiscKey", 0xD4, layer, unused, buffer, 0x20)
}

/* 0x804ADBF0 (0x198): reads the 0x20-byte key report of `format` at sector `lsn` into `reportKey`. */
/* untyped: byte range the report fills */
BOOL DVDLowReportKey(void* reportKey, u32 format, u32 lsn, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xA4;
    diCommand[commandBufIndex].args[0] = format >> 16;
    diCommand[commandBufIndex].args[1] = lsn;
    result = IOS_IoctlAsync(diFd, 0xA4, &diCommand[commandBufIndex], sizeof(DICommand), reportKey, 0x20,
                            doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowReportKey) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Sets the drive's read offset. */
BOOL DVDLowOffset(u32 offset, u32 select, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowOffset", 0xD9, select, offset, NULL, 0)
}

/* Switches the drive laser off. */
BOOL DVDLowStopLaser(u32 unused0, u32 unused1, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowStopLaser", 0xD2, unused0, unused1, &DIRegValues, sizeof(DIRegisterValues))
}

/* Reads the disc's burst cutting area into `buffer`. */
BOOL DVDLowReadDiskBca(u8* buffer, u32 unused, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowReadDiskBca", 0xDA, unused, 0, buffer, 0x40)
}

/* Runs a servo measurement control command. */
BOOL DVDLowSerMeasControl(u32 control, u8* buffer, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowSerMeasControl", 0xDF, control, 0, buffer, 0x20)
}

/* Asks the drive for its disc status. */
BOOL DVDLowRequestDiscStatus(u32 unused0, u32 unused1, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowRequestDiscStatus", 0xDB, unused0, unused1, &DIRegValues, sizeof(DIRegisterValues))
}

/* Asks the drive for its retry count. */
BOOL DVDLowRequestRetryNumber(u32 unused0, u32 unused1, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowRequestRetryNumber", 0xDC, unused0, unused1, &DIRegValues, sizeof(DIRegisterValues))
}

/* 0x804ADD90 (0x184): limits the drive's rotation speed to `subcmd`. */
BOOL DVDLowSetMaximumRotation(u32 subcmd, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xDD;
    diCommand[commandBufIndex].args[0] = (subcmd >> 16) & 3;
    result = IOS_IoctlAsync(diFd, 0xDD, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, doTransactionCallback,
                            context);
    if (result != 0) {
        OSReport("@@@ (DVDLowSetMaxRotation) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804ADF20 (0x1B0): reads `length` bytes at `offset` (in 4-byte units) into the 32-byte aligned `buffer`. */
/* untyped: byte range the read fills */
BOOL DVDLowRead(void* buffer, u32 length, u32 offset, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    if ((u32)buffer & 31) {
        OSReport("(DVDLowRead): ERROR - destAddr buffer is not 32 byte aligned\n");
        return FALSE;
    }
    context = newContext(callback);
    readLength = length;
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x71;
    diCommand[commandBufIndex].args[0] = length;
    diCommand[commandBufIndex].args[1] = offset;
    result = IOS_IoctlAsync(diFd, 0x71, &diCommand[commandBufIndex], sizeof(DICommand), buffer, length,
                            doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowRead) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AE0D0 (0x180): moves the drive head to `offset` (in 4-byte units). */
BOOL DVDLowSeek(u32 offset, DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    context = newContext(callback);
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0xAB;
    diCommand[commandBufIndex].args[0] = offset;
    result = IOS_IoctlAsync(diFd, 0xAB, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, doTransactionCallback,
                            context);
    if (result != 0) {
        OSReport("@@@ (DVDLowSeek) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Reads the cover register synchronously into the register copy; refused from inside a command callback. */
BOOL DVDLowGetCoverReg(void) {
    s32 result;

    if (callbackInProgress) {
        OSReport("(DVDLowGetCoverReg): Synch functions can't be called in callbacks\n");
        return FALSE;
    }
    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x7A;
    result = IOS_Ioctl(diFd, 0x7A, &diCommand[commandBufIndex], sizeof(DICommand), CoverRegBuf, sizeof(CoverRegBuf));
    if (result != 0) {
        OSReport("@@@ (DVDLowGetCoverReg) IOS_Ioctl returned error: %d\n", result);
        return FALSE;
    }
    DIRegValues.coverReg = CoverRegBuf[0];
    return TRUE;
}

/* 0x804AE250 (0x10): returns the cover register value the last cover-register read stored. */
u32 DVDLowGetCoverRegister(void) {
    return DIRegValues.coverReg;
}

/* 0x804AE260 (0xC): returns the status register value the last status-register read stored. */
u32 DVDLowGetStatusRegister(void) {
    return StatusRegBuf[0];
}

/* 0x804AE270 (0xC): returns the control register value the last control-register read stored. */
u32 DVDLowGetControlRegister(void) {
    return ControlRegBuf[0];
}

/* 0x804AE280 (0x16C): reads the cover register into the driver's copy. */
BOOL DVDLowPrepareCoverRegister(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x7A;
    context = newContext(callback);
    result = IOS_IoctlAsync(diFd, 0x7A, &diCommand[commandBufIndex], sizeof(DICommand), CoverRegBuf,
                            sizeof(CoverRegBuf), doCoverCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowPrepareCoverRegsiter) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AE3F0 (0x16C): reads the status register into the driver's copy. */
BOOL DVDLowPrepareStatusRegister(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x95;
    context = newContext(callback);
    result = IOS_IoctlAsync(diFd, 0x95, &diCommand[commandBufIndex], sizeof(DICommand), StatusRegBuf,
                            sizeof(StatusRegBuf), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowPrepareStatusRegsiter) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AE560 (0x16C): reads the control register into the driver's copy. */
BOOL DVDLowPrepareControlRegister(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x96;
    context = newContext(callback);
    result = IOS_IoctlAsync(diFd, 0x96, &diCommand[commandBufIndex], sizeof(DICommand), ControlRegBuf,
                            sizeof(ControlRegBuf), doTransactionCallback, context);
    if (result != 0) {
        OSReport("@@@ (DVDLowPrepareControlRegister) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* 0x804AE6D0 (0xC): returns the immediate buffer register value the last command stored. */
u32 DVDLowGetImmBufferReg(void) {
    return DIRegValues.immBuf;
}

/* 0x804AE6E0 (0x8): unmasks the status interrupts; nothing to do on this driver. */
BOOL DVDLowUnmaskStatusInterrupts(void) {
    return TRUE;
}

/* 0x804AE6F0 (0x8): masks the cover interrupt; nothing to do on this driver. */
BOOL DVDLowMaskCoverInterrupt(void) {
    return TRUE;
}

/* 0x804AE700 (0x168): acknowledges a cover interrupt. */
BOOL DVDLowClearCoverInterrupt(DVDLowCallback callback) {
    DVDContext* context;
    s32 result;

    nextCommandBuf();
    diCommand[commandBufIndex].command = 0x86;
    context = newContext(callback);
    result = IOS_IoctlAsync(diFd, 0x86, &diCommand[commandBufIndex], sizeof(DICommand), NULL, 0, doTransactionCallback,
                            context);
    if (result != 0) {
        OSReport("@@@ (DVDLowClearCoverInterrupt) IOS_IoctlAsync returned error: %d\n", result);
        context->inUse = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* Enables or disables DVD-video reads. */
BOOL DVDLowEnableDvdVideo(u32 enable, DVDLowCallback callback) {
    DVD_LOW_COMMAND2("DVDLowEnableDvdVideo", 0x8E, enable, 0, NULL, 0)
}

/* 0x804AE870 (0x8): returns whether `alarm` belongs to the DI driver; this driver owns no alarm. */
BOOL DVDLowTestAlarm(const OSAlarm* alarm) {
    return FALSE;
}
