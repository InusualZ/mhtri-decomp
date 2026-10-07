/* USB/usb.cpp - the IOS USB client (`IUSB_*` open/close and the bulk, interrupt, control and isochronous transfer helpers).
 * RANGE. .text 0x804E46F0-0x804E6710 (22 functions); .data 0x8062AB98-0x8062B3C8; .sdata 0x80794150-0x80794160;
 *   .sbss 0x807955C8-0x807955D8.
 *   Edges: the two print helpers 0x804E46F0 ("USB: ") and 0x804E47A0 ("USB ERR: ") open the range and are called by
 *   most of the others; the "USB ERR: " string object (.data 0x8062AB98) is one copy read from both halves
 *   of the range (0x804E4850..0x804E63E0), so the halves are one TU.  The right edge 0x804E6710 is the first
 *   reader of the VI state (.sbss 0x80795608, .bss 0x8075B110).
 * FLAGS. `cflags_base` (-O4,p).  `__sthbrx` is the byte-reversed halfword store intrinsic.
 * NAMES. `IUSB_OpenLib`, `IUSB_CloseLib`, `IUSB_OpenDeviceIds`, `IUSB_CloseDeviceAsync`, `__LongBlkMsgInt` and
 *   `__IntrBlkMsgInt` are the map's names; the log strings name `IUSB_OpenDeviceIdsAsync`, `IUSB_CloseDevice`,
 *   `IUSB_IsoMsgAsync` and `__CtrlMsgInt`.  GUESS names: `IUSB_Log`, `IUSB_LogError` (the "USB: " and "USB ERR: " printers),
 *   `IUSB_RequestDone` (the completion of every request), `IUSB_ReadCtrlMsgAsync` / `IUSB_WriteCtrlMsgAsync` (the
 *   invalidate / flush wrappers of `__CtrlMsgInt`), `usbIsoPacketTotal` (the packet size check of the iso request),
 *   `IUSB_RegisterInsertionNotifyAsync` (ioctl 0x1B with a path, the log strings name a device list / insertion notify).
 * RESIDUALS. All 22 rows have a body.  Data: the target .data holds the strings of functions the link dropped (0x8062B0CC..
 *   0x8062B290, 750 B), so every string offset after them differs and the object's .data is 1346 B against 2096 B; .sbss and
 *   .sdata are 9/14 B against 16/16 B (end padding).  `__CtrlMsgInt`: the target stores the three `sthbrx` operands without
 *   the `clrlwi` the intrinsic's u16 parameter adds.  `usbIsoPacketTotal` keeps the result in r31 and rotates the loop
 *   (the `return result` spelling lets the compiler fold `result`).  `IUSB_RegisterInsertionNotifyAsync` zeroes the request
 *   twice (two memset calls in the target).  `IUSB_RequestDone`: loop pointer, index and block swap r28..r30.
 * SHAPES. A single exit through `do { ... break; ... } while (0)` reproduces the target's one shared epilogue (a `return`
 *   in the middle or a flag measured worse); the transfer helpers take `s32` parameters so callers do not re-extend them.
 */

#include "types.h"
#include "USB/usb.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcclt.h"
#include "IPC/memory.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OSError.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"
#include "MSL_C/printf.h"

/* .sdata */
static s32 usbHeapId = -1;
static u8 usbErrorLogEnabled = 1;

/* .sbss */
static u8* usbBufferLo;
static u8* usbBufferHi;
static u8 usbLogEnabled;

/* Prints a trace line behind a "USB: " prefix while tracing is enabled. */
void IUSB_Log(const char* format, ...)
{
    va_list args;

    if (usbLogEnabled != 0) {
        OSReport("USB: ");
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

/* Prints an error line behind a "USB ERR: " prefix while error logging is enabled. */
void IUSB_LogError(const char* format, ...)
{
    va_list args;

    if (usbErrorLogEnabled != 0) {
        OSReport("USB ERR: ");
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

/* Reserves the library's IPC heap the first time it is called; returns 0 or a negative error. */
s32 IUSB_OpenLib(void)
{
    s32 result = 0;
    u32 level = OSDisableInterrupts();

    if (usbHeapId != -1) {
        IUSB_Log("Library is already initialized. Heap Id = %d\n", usbHeapId);
    } else {
        do {
            if (usbBufferLo == NULL) {
                usbBufferLo = IPCGetBufferLo();
                usbBufferHi = IPCGetBufferHi();
                IUSB_Log("iusb size: %d lo: %x hi: %x\n", 0x80, usbBufferLo, usbBufferHi);
                if (usbBufferLo + 0x4000 > usbBufferHi) {
                    IUSB_LogError("Not enough IPC arena\n");
                    result = -22;
                    break;
                }
                IPCSetBufferLo(usbBufferLo + 0x4000);
            }
            usbHeapId = iosCreateHeap(usbBufferLo, 0x4000);
            if (usbHeapId < 0) {
                IUSB_LogError("Not enough heaps\n");
                result = -22;
            }
        } while (0);
    }
    OSRestoreInterrupts(level);
    return result;
}

/* Closing the library needs no work. */
s32 IUSB_CloseLib(void)
{
    return 0;
}

/* Frees the buffers a finished request owns, runs its callback and frees the request. */
s32 IUSB_RequestDone(s32 result, USBRequest* request)
{
    void* block;
    IUSB_Log("_intrBlkCtrlIsoCb returned: %d\n", result);
    IUSB_Log("_intrBlkCtrlIsoCb: nclean = %d\n", request->cleanCount);
    if ((u32)(request->cleanCount - 2) > 2 && request->cleanCount != 7 && request->cleanCount != 0) {
        IUSB_LogError("__intrBlkCtrlIsoCb: got invalid nclean\n");
    } else {
        u32 i;
        for (i = 0; i < request->cleanCount; i++) {
            IUSB_Log("Freeing clean[%d] = %x\n", i, request->clean[i]);
            block = request->clean[i];
            if (block != NULL) {
                s32 freed = iosFree(usbHeapId, block);
                if (freed < 0) {
                    IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, block, freed);
                }
            }
        }
        request->cleanCount = 0;
    }
    IUSB_Log("cb = %x cbArg = %x\n", request->callback, request->callbackArg);
    if (request->callback != NULL) {
        request->callback(result, request->callbackArg);
    } else if (request->callbackEx != NULL) {
        IUSB_Log("calling iso callback\n");
        request->callbackEx(result, request->callbackExtra, request->callbackArg);
    }
    if (request != NULL) {
        s32 freed = iosFree(usbHeapId, request);
        if (freed < 0) {
            IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, request, freed);
        }
    }
    return result;
}

/* Opens `/dev/usb/<bus>/<vendor>/<product>` and stores its descriptor in `fd`; returns the descriptor or a negative error. */
s32 IUSB_OpenDeviceIds(const char* bus, s32 vendor, s32 product, s32* fd)
{
    s32 result;
    USBRequest* request = NULL;

    if (fd == NULL) {
        result = -4;
    } else {
        request = (USBRequest*)iosAllocAligned(usbHeapId, sizeof(USBRequest), 0x20);
        if (request == NULL) {
            IUSB_LogError("iosAllocAligned(%d, %u) failed: %d\n", usbHeapId, sizeof(USBRequest), request);
        }
        if (request == NULL) {
            IUSB_LogError("OpenDeviceIds: Not enough memory\n");
            result = -22;
        } else {
            memset(request, 0, sizeof(USBRequest));
            snprintf(request->path, sizeof(request->path), "/dev/usb/%s/%x/%x", bus, vendor, product);
            IUSB_Log("OpenDevice - %s\n", request->path);
            result = IOS_Open(request->path, 0);
            IUSB_Log("OpenDevice returned: %d\n", result);
            *fd = result;
        }
    }
    if (request != NULL) {
        s32 freed = iosFree(usbHeapId, request);
        if (freed < 0) {
            IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, request, freed);
        }
    }
    return result;
}

/* Opens `/dev/usb/<bus>/<vendor>/<product>` and reports the descriptor through `callback`; returns 0 or a negative error. */
s32 IUSB_OpenDeviceIdsAsync(const char* bus, s32 vendor, s32 product, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    USBRequest* request;
    s32 result;

    IUSB_Log("OpenDevice\n");
    request = (USBRequest*)iosAllocAligned(usbHeapId, sizeof(USBRequest), 0x20);
    if (request == NULL) {
        IUSB_LogError("iosAllocAligned(%d, %u) failed: %d\n", usbHeapId, sizeof(USBRequest), request);
    }
    if (request == NULL) {
        IUSB_LogError("OpenDeviceIdsAsync: Not enough memory\n");
        result = -22;
    } else {
        memset(request, 0, sizeof(USBRequest));
        request->callback = callback;
        request->callbackArg = callbackArg;
        request->cleanCount = 0;
        snprintf(request->path, sizeof(request->path), "/dev/usb/%s/%x/%x", bus, vendor, product);
        IUSB_Log("OpenDevice - %s\n", request->path);
        result = IOS_OpenAsync(request->path, 0, (void*)IUSB_RequestDone, request);
        IUSB_Log("OpenDevice returned: %d\n", result);
        if (result < 0 && request != NULL) {
            s32 freed = iosFree(usbHeapId, request);
            if (freed < 0) {
                IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, request, freed);
            }
        }
    }
    return result;
}

/* Closes the device descriptor `fd`; returns the IPC result. */
s32 IUSB_CloseDevice(s32 fd)
{
    s32 result;

    IUSB_Log("CloseDevice\n");
    result = IOS_Close(fd);
    IUSB_Log("CloseDevice returned: %d\n", result);
    return result;
}

/* Closes the device descriptor `fd` and reports the result through `callback`; returns 0 or a negative error. */
s32 IUSB_CloseDeviceAsync(s32 fd, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    USBRequest* request;
    s32 result;

    IUSB_Log("CloseDevice\n");
    request = (USBRequest*)iosAllocAligned(usbHeapId, sizeof(USBRequest), 0x20);
    if (request == NULL) {
        IUSB_LogError("iosAllocAligned(%d, %u) failed: %d\n", usbHeapId, sizeof(USBRequest), request);
    }
    if (request == NULL) {
        IUSB_LogError("CloseDeviceAsync: Not enough memory\n");
        result = -22;
    } else {
        memset(request, 0, sizeof(USBRequest));
        request->callback = callback;
        request->callbackArg = callbackArg;
        request->cleanCount = 0;
        result = IOS_CloseAsync(fd, (void*)IUSB_RequestDone, request);
        IUSB_Log("CloseDevice returned: %d\n", result);
        if (result < 0 && request != NULL) {
            s32 freed = iosFree(usbHeapId, request);
            if (freed < 0) {
                IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, request, freed);
            }
        }
    }
    return result;
}

/* Frees one IPC heap block and logs a failure. */
static inline void usbFree(void* block) /* untyped: caller-owned block */
{
    if (block != NULL) {
        s32 freed = iosFree(usbHeapId, block);
        if (freed < 0) {
            IUSB_LogError("iosFree(%d, 0x%x) failed: %d\n", usbHeapId, block, freed);
        }
    }
}

/* Takes an aligned IPC heap block and logs a failure. */
static inline void* usbAlloc(u32 size) /* untyped: caller-owned block */
{
    void* block = iosAllocAligned(usbHeapId, size, 0x20);
    if (block == NULL) {
        IUSB_LogError("iosAllocAligned(%d, %u) failed: %d\n", usbHeapId, size, block);
    }
    return block;
}

/* Runs a long bulk transfer on `endpoint` of device `fd`; blocks unless `async` is set, then `callback` runs on completion. */
s32 __LongBlkMsgInt(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg, s32 async) /* untyped: caller-owned payload */
{
    s32 result;
    IPCIOVector* vectors = (IPCIOVector*)usbAlloc(0x60);
    s8* endpointBuf = (s8*)usbAlloc(0x20);
    u32* lengthBuf = (u32*)usbAlloc(0x20);

    do {
                    if (vectors == NULL || endpointBuf == NULL || lengthBuf == NULL) {
                        IUSB_LogError("__LongBlkMsgInt: Not enough memory\n");
                        result = -22;
                    } else {
                        *endpointBuf = endpoint;
                        *lengthBuf = length;
                        vectors[0].base = endpointBuf;
                        vectors[0].length = 1;
                        vectors[1].base = lengthBuf;
                        vectors[1].length = 4;
                        vectors[2].base = data;
                        vectors[2].length = length;
                        DCFlushRange(endpointBuf, 0x20);
                        DCFlushRange(lengthBuf, 0x20);
                        DCFlushRange(vectors, 0x60);
                        if (async == 0) {
                            result = IOS_Ioctlv(fd, 0xA, 2, 1, vectors);
                            IUSB_Log("Long bulk ioctl returned: %d\n", result);
                        } else {
                            USBRequest* request = (USBRequest*)usbAlloc(sizeof(USBRequest));
                            if (request == NULL) {
                                IUSB_LogError("LongBlkMsgInt (async): Not enough memory\n");
                                result = -22;
                            } else {
                                memset(request, 0, sizeof(USBRequest));
                                request->callback = callback;
                                request->callbackArg = callbackArg;
                                IUSB_Log("longblkmsg: cb = 0x%x cbArg = 0x%x\n", callback, callbackArg);
                                request->cleanCount = 3;
                                request->clean[0] = endpointBuf;
                                request->clean[1] = lengthBuf;
                                request->clean[2] = vectors;
                                request->transfer.data = data;
                                request->transfer.length = length;
                                result = IOS_IoctlvAsync(fd, 0xA, 2, 1, vectors, (void*)IUSB_RequestDone, request);
                                if (result >= 0) {
                                    break;
                                }
                                if (result == -22) {
                                    OSReport("%s: IoctlvAsync returned error %d\n", "__LongBlkMsgInt", result);
                                }
                                usbFree(request);
                            }
                        }
                    }
                    usbFree(endpointBuf);
                    usbFree(lengthBuf);
                    usbFree(vectors);
    } while (0);
    return result;
}

/* Runs an interrupt or short bulk transfer (`ioctl` selects which) on `endpoint` of device `fd`; blocks unless `async` is set. */
s32 __IntrBlkMsgInt(s32 fd, s32 endpoint, s32 length, void* data, s32 ioctl, USBCallback callback, void* callbackArg, s32 async) /* untyped: caller-owned payload */
{
    s32 result;
    IPCIOVector* vectors = (IPCIOVector*)usbAlloc(0x60);
    s8* endpointBuf = (s8*)usbAlloc(0x20);
    s16* lengthBuf = (s16*)usbAlloc(0x20);

    do {
                if (vectors == NULL || endpointBuf == NULL || lengthBuf == NULL) {
                    IUSB_LogError("__IntrBlkMsgInt: Not enough memory\n");
                    result = -22;
                } else {
                    *endpointBuf = endpoint;
                    *lengthBuf = length;
                    vectors[0].base = endpointBuf;
                    vectors[0].length = 1;
                    vectors[1].base = lengthBuf;
                    vectors[1].length = 2;
                    vectors[2].base = data;
                    vectors[2].length = length;
                    DCFlushRange(endpointBuf, 0x20);
                    DCFlushRange(lengthBuf, 0x20);
                    DCFlushRange(vectors, 0x60);
                    if (async == 0) {
                        result = IOS_Ioctlv(fd, ioctl, 2, 1, vectors);
                        IUSB_Log("intr/blk ioctl returned: %d\n", result);
                    } else {
                        USBRequest* request = (USBRequest*)usbAlloc(sizeof(USBRequest));
                        if (request == NULL) {
                            IUSB_LogError("IntBlkMsgInt (async): Not enough memory\n");
                            result = -22;
                        } else {
                            memset(request, 0, sizeof(USBRequest));
                            request->callback = callback;
                            request->callbackArg = callbackArg;
                            IUSB_Log("intrblkmsg: cb = 0x%x cbArg = 0x%x\n", callback, callbackArg);
                            request->cleanCount = 3;
                            request->clean[0] = endpointBuf;
                            request->clean[1] = lengthBuf;
                            request->clean[2] = vectors;
                            request->transfer.data = data;
                            request->transfer.length = length;
                            result = IOS_IoctlvAsync(fd, ioctl, 2, 1, vectors, (void*)IUSB_RequestDone, request);
                            if (result >= 0) {
                                break;
                            }
                            if (result == -22) {
                                OSReport("%s: IoctlvAsync returned error %d\n", "__IntrBlkMsgInt", result);
                            }
                            usbFree(request);
                        }
                    }
                }
                usbFree(endpointBuf);
                usbFree(lengthBuf);
                usbFree(vectors);
    } while (0);
    return result;
}

/* Starts an interrupt-endpoint read into `data` (the cache range is invalidated first). */
s32 IUSB_ReadIntrMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCInvalidateRange(data, length);
    return __IntrBlkMsgInt(fd, endpoint, length, data, 2, callback, callbackArg, 1);
}

/* Starts a bulk read into `data`: short transfers use the interrupt-style path, long ones the long bulk path. */
s32 IUSB_ReadBlkMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCInvalidateRange(data, length);
    if (length <= 0xFFFF) {
        return __IntrBlkMsgInt(fd, endpoint, length, data, 1, callback, callbackArg, 1);
    }
    return __LongBlkMsgInt(fd, endpoint, length, data, callback, callbackArg, 1);
}

/* Starts a bulk write from `data`: the cache range is flushed first, then the transfer is sized like a read. */
s32 IUSB_WriteBlkMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCFlushRange(data, length);
    if (length <= 0xFFFF) {
        return __IntrBlkMsgInt(fd, endpoint, length, data, 1, callback, callbackArg, 1);
    }
    return __LongBlkMsgInt(fd, endpoint, length, data, callback, callbackArg, 1);
}

/* Runs a control transfer (`requestType`/`request`/`value`/`index`/`length`) with an optional data stage; blocks unless `async` is set. */
s32 __CtrlMsgInt(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg, u8 async) /* untyped: caller-owned payload */
{
    s32 result;
    IPCIOVector* vectors;
    s8* typeBuf;
    s8* requestBuf;
    s8* stageBuf;
    u16* valueBuf;
    u16* indexBuf;
    u16* lengthBuf;

    do {
                if ((data == NULL && length != 0) || ((u32)data & 0x1F) != 0) {
                    result = -4;
                    IUSB_LogError("ctrlmsg: bad data buffer\n");
                    break;
                }
                vectors = (IPCIOVector*)usbAlloc(0xE0);
                typeBuf = (s8*)usbAlloc(0x20);
                requestBuf = (s8*)usbAlloc(0x20);
                stageBuf = (s8*)usbAlloc(0x20);
                valueBuf = (u16*)usbAlloc(0x20);
                indexBuf = (u16*)usbAlloc(0x20);
                lengthBuf = (u16*)usbAlloc(0x20);
                if (typeBuf == NULL || requestBuf == NULL || stageBuf == NULL || valueBuf == NULL || indexBuf == NULL || lengthBuf == NULL
                    || vectors == NULL) {
                    IUSB_LogError("Ctrl Msg: Not enough memory\n");
                    result = -22;
                } else {
                    *typeBuf = requestType;
                    *requestBuf = request;
                    __sthbrx(value, valueBuf, 0);
                    __sthbrx(index, indexBuf, 0);
                    __sthbrx(length, lengthBuf, 0);
                    *stageBuf = 0;
                    vectors[0].base = typeBuf;
                    vectors[0].length = 1;
                    vectors[1].base = requestBuf;
                    vectors[1].length = 1;
                    vectors[2].base = valueBuf;
                    vectors[2].length = 2;
                    vectors[3].base = indexBuf;
                    vectors[3].length = 2;
                    vectors[4].base = lengthBuf;
                    vectors[4].length = 2;
                    vectors[5].base = stageBuf;
                    vectors[5].length = 1;
                    vectors[6].base = data;
                    vectors[6].length = length;
                    DCFlushRange(typeBuf, 0x20);
                    DCFlushRange(requestBuf, 0x20);
                    DCFlushRange(stageBuf, 0x20);
                    DCFlushRange(valueBuf, 0x20);
                    DCFlushRange(indexBuf, 0x20);
                    DCFlushRange(lengthBuf, 0x20);
                    DCFlushRange(vectors, 0xE0);
                    if (async == 0) {
                        result = IOS_Ioctlv(fd, 0, 6, 1, vectors);
                    } else {
                        USBRequest* pending = (USBRequest*)usbAlloc(sizeof(USBRequest));
                        if (pending == NULL) {
                            IUSB_LogError("CtrlMsgInt (async): Not enough memory\n");
                            result = -22;
                        } else {
                            memset(pending, 0, sizeof(USBRequest));
                            pending->callback = callback;
                            pending->callbackArg = callbackArg;
                            IUSB_Log("ctrlmsgint: cb = 0x%x cbArg = 0x%x\n", callback, callbackArg);
                            pending->cleanCount = 7;
                            pending->clean[0] = typeBuf;
                            pending->clean[1] = requestBuf;
                            pending->clean[2] = valueBuf;
                            pending->clean[3] = indexBuf;
                            pending->clean[4] = lengthBuf;
                            pending->clean[5] = stageBuf;
                            pending->clean[6] = vectors;
                            pending->transfer.data = data;
                            pending->transfer.length = length;
                            result = IOS_IoctlvAsync(fd, 0, 6, 1, vectors, (void*)IUSB_RequestDone, pending);
                            IUSB_Log("Ctrl Msg async returned: %d\n", result);
                            if (result >= 0) {
                                break;
                            }
                            usbFree(pending);
                        }
                    }
                }
                usbFree(typeBuf);
                usbFree(requestBuf);
                usbFree(valueBuf);
                usbFree(indexBuf);
                usbFree(lengthBuf);
                usbFree(stageBuf);
                usbFree(vectors);
    } while (0);
    return result;
}

/* Starts a control transfer whose data stage reads into `data` (the cache range is invalidated first). */
s32 IUSB_ReadCtrlMsgAsync(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCInvalidateRange(data, length);
    return __CtrlMsgInt(fd, requestType, request, value, index, length, data, callback, callbackArg, 1);
}

/* Starts a control transfer whose data stage writes from `data` (the cache range is flushed first). */
s32 IUSB_WriteCtrlMsgAsync(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCFlushRange(data, length);
    return __CtrlMsgInt(fd, requestType, request, value, index, length, data, callback, callbackArg, 1);
}

/* Issues the driver's ioctl 0x1D on `fd`, which takes no payload. */
s32 IUSB_Ioctl1D(s32 fd)
{
    return IOS_Ioctl(fd, 0x1D, 0, 0, 0, 0);
}

/* Asks the driver to notify `callback` when a device is removed. */
s32 IUSB_DeviceRemovalNotifyAsync(s32 fd, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    IUSB_Log("DeviceRemovalNotifyAsync\n");
    return IOS_IoctlAsync(fd, 0x1A, 0, 0, 0, 0, (void*)callback, callbackArg);
}

/* Sums the packet sizes of an isochronous request into `total`; returns 0, or -4 for an empty or oversized description. */
#pragma dont_inline on
static s32 usbIsoPacketTotal(USBIsoRequest* iso, u16* total)
{
    s32 result = -4;

    if (iso != NULL && iso->data != NULL && iso->packetCount != 0 && iso->packetCount <= 8 && iso->packetSizes != NULL) {
        u32 i;
        *total = 0;
        for (i = 0; i < iso->packetCount; i++) {
            u16 size = iso->packetSizes[i];
            if (size > 0xC00) {
                IUSB_LogError("packet %u too big: %u\n", i, iso->packetSizes[i]);
                return result;
            }
            *total += size;
        }
        result = 0;
    }
    return result;
}
#pragma dont_inline reset

/* Starts an isochronous transfer described by `iso` on `endpoint`; `callback` receives the request's description. */
/* untyped: caller-owned payload */
s32 IUSB_IsoMsgAsync(s32 fd, s32 endpoint, USBIsoRequest* iso, USBCallbackEx callback, void* callbackArg)
{
    s32 result;
    u16 total;
    s8* endpointBuf;
    u8* countBuf;
    u16* totalBuf;
    IPCIOVector* vectors;
    USBRequest* request;

    do {
            if (endpoint == 0 || callback == NULL || usbIsoPacketTotal(iso, &total) < 0) {
                IUSB_LogError("Invalid parameters for ISO transfer request\n");
                result = -4;
                break;
            }
            vectors = (IPCIOVector*)usbAlloc(0xA0);
            endpointBuf = (s8*)usbAlloc(0x20);
            totalBuf = (u16*)usbAlloc(0x20);
            countBuf = (u8*)usbAlloc(0x20);
            request = (USBRequest*)usbAlloc(sizeof(USBRequest));
            if (vectors == NULL || endpointBuf == NULL || totalBuf == NULL || request == NULL) {
                IUSB_LogError("IUSB_IsoMsgAsync: Not enough memory\n");
                result = -22;
                usbFree(request);
            } else {
                memset(request, 0, sizeof(USBRequest));
                *endpointBuf = endpoint;
                *totalBuf = total;
                *countBuf = iso->packetCount;
                vectors[0].base = endpointBuf;
                vectors[0].length = 1;
                vectors[1].base = totalBuf;
                vectors[1].length = 2;
                vectors[2].base = countBuf;
                vectors[2].length = 1;
                vectors[3].base = iso->packetSizes;
                vectors[3].length = iso->packetCount * 2;
                vectors[4].base = iso->data;
                vectors[4].length = total;
                DCFlushRange(endpointBuf, 0x20);
                DCFlushRange(totalBuf, 0x20);
                DCFlushRange(countBuf, 0x20);
                DCFlushRange(vectors, 0xA0);
                request->callbackEx = callback;
                request->callbackExtra = iso;
                request->callbackArg = callbackArg;
                request->cleanCount = 4;
                request->clean[0] = endpointBuf;
                request->clean[1] = totalBuf;
                request->clean[2] = countBuf;
                request->clean[3] = vectors;
                result = IOS_IoctlvAsync(fd, 9, 3, 2, vectors, (void*)IUSB_RequestDone, request);
                if (result >= 0) {
                    break;
                }
                usbFree(request);
            }
            usbFree(endpointBuf);
            usbFree(totalBuf);
            usbFree(countBuf);
            usbFree(vectors);
    } while (0);
    return result;
}

/* Registers `callback` for the insertion of a device matching `classId` and `subId` under `path`. */
s32 IUSB_RegisterInsertionNotifyAsync(const char* path, s16 classId, s16 subId, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    s32 result;
    s32 handle;
    IPCIOVector* vectors;
    s16* classBuf;
    s16* subBuf;
    USBRequest* request;

    do {
        if (path == NULL || classId == 0 || subId == 0) {
            result = -4;
            break;
        }
        handle = IOS_OpenTimed(path, 0);
        if (handle < 0) {
            result = handle;
            IUSB_LogError("Open(%s) failed\n", path);
            break;
        }
        vectors = (IPCIOVector*)usbAlloc(0x40);
        subBuf = (s16*)usbAlloc(0x20);
        classBuf = (s16*)usbAlloc(0x20);
        request = (USBRequest*)usbAlloc(sizeof(USBRequest));
        if (vectors == NULL || subBuf == NULL || classBuf == NULL || request == NULL) {
            IUSB_LogError("getDeviceList: Not enough memory\n");
            result = -22;
        } else {
            memset(request, 0, sizeof(USBRequest));
            memset(request, 0, sizeof(USBRequest));
            *classBuf = classId;
            *subBuf = subId;
            vectors[0].base = classBuf;
            vectors[0].length = 2;
            vectors[1].base = subBuf;
            vectors[1].length = 2;
            DCFlushRange(classBuf, 0x20);
            DCFlushRange(subBuf, 0x20);
            DCFlushRange(vectors, 0x40);
            request->callback = callback;
            request->callbackArg = callbackArg;
            request->cleanCount = 3;
            request->clean[0] = classBuf;
            request->clean[1] = subBuf;
            request->clean[2] = vectors;
            result = IOS_IoctlvAsync(handle, 0x1B, 2, 0, vectors, (void*)IUSB_RequestDone, request);
            if (result >= 0) {
                result = IOS_CloseTimed(handle);
                if (result >= 0) {
                    break;
                }
            }
        }
        usbFree(classBuf);
        usbFree(subBuf);
        usbFree(vectors);
        usbFree(request);
    } while (0);
    return result;
}
