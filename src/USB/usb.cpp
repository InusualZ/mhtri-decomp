/* USB/usb.cpp - the IOS USB client (`IUSB_*` open/close and the bulk, interrupt and control transfer helpers).
 * RANGE. .text 0x804E46F0-0x804E6710 (22 functions); .data 0x8062AB98-0x8062B3C8; .sdata 0x80794150-0x80794160;
 *   .sbss 0x807955C8-0x807955D8.
 *   Edges: the two print helpers 0x804E46F0 ("USB: ") and 0x804E47A0 ("USB ERR: ") open the range and are called by
 *   most of the others; the "USB ERR: " string object (.data 0x8062AB98) is one copy read from both halves
 *   of the range (0x804E4850..0x804E63E0), so the halves are one TU.  The right edge 0x804E6710 is the first
 *   reader of the VI state (.sbss 0x80795608, .bss 0x8075B110).
 * FLAGS. `cflags_base` (-O4,p).
 * NAMES. `IUSB_OpenLib`, `IUSB_CloseLib`, `IUSB_OpenDeviceIds`, `IUSB_CloseDeviceAsync`, `__LongBlkMsgInt` and
 *   `__IntrBlkMsgInt` are the map's names; the log strings name `IUSB_OpenDeviceIdsAsync` and `IUSB_CloseDevice`.  GUESS
 *   names: `IUSB_Log`, `IUSB_LogError` (the "USB: " and "USB ERR: " printers), `IUSB_RequestDone` (the completion of the open
 *   and close requests); the other functions still carry generated names.
 * RESIDUALS. Written: the loggers, `IUSB_OpenLib`, `IUSB_CloseLib`, `IUSB_RequestDone`, the open and close functions; not
 *   attempted: 0x804E4F40 `__LongBlkMsgInt`, 0x804E52A0 `__IntrBlkMsgInt` and the ten transfer, control and descriptor
 *   functions between 0x804E5600 and 0x804E6710.  The unit's data (.data strings, .sdata flags, .sbss heap bounds) is
 *   not claimed.
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
        if (usbBufferLo == NULL) {
            usbBufferLo = IPCGetBufferLo();
            usbBufferHi = IPCGetBufferHi();
            IUSB_Log("iusb size: %d lo: %x hi: %x\n", 0x80, usbBufferLo, usbBufferHi);
            if (usbBufferLo + 0x4000 > usbBufferHi) {
                IUSB_LogError("Not enough IPC arena\n");
                result = -22;
            } else {
                IPCSetBufferLo(usbBufferLo + 0x4000);
            }
        }
        if (result == 0) {
            usbHeapId = iosCreateHeap(usbBufferLo, 0x4000);
            if (usbHeapId < 0) {
                IUSB_LogError("Not enough heaps\n");
                result = -22;
            }
        }
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
    IUSB_Log("_intrBlkCtrlIsoCb returned: %d\n", result);
    IUSB_Log("_intrBlkCtrlIsoCb: nclean = %d\n", request->cleanCount);
    if ((u32)(request->cleanCount - 2) > 2 && request->cleanCount != 7 && request->cleanCount != 0) {
        IUSB_LogError("__intrBlkCtrlIsoCb: got invalid nclean\n");
    } else {
        u32 i;
        for (i = 0; i < request->cleanCount; i++) {
            void* block;
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
s32 __LongBlkMsgInt(s32 fd, s8 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg, s32 async) /* untyped: caller-owned payload */
{
    s32 result;
    IPCIOVector* vectors = (IPCIOVector*)usbAlloc(0x60);
    s8* endpointBuf = (s8*)usbAlloc(0x20);
    u32* lengthBuf = (u32*)usbAlloc(0x20);

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
                if (result < 0) {
                    if (result == -22) {
                        OSReport("%s: IoctlvAsync returned error %d\n", "__LongBlkMsgInt", result);
                    }
                    usbFree(request);
                } else {
                    return result;
                }
            }
        }
    }
    usbFree(endpointBuf);
    usbFree(lengthBuf);
    usbFree(vectors);
    return result;
}

/* Runs an interrupt or short bulk transfer (`ioctl` selects which) on `endpoint` of device `fd`; blocks unless `async` is set. */
s32 __IntrBlkMsgInt(s32 fd, s8 endpoint, s16 length, void* data, s32 ioctl, USBCallback callback, void* callbackArg, s32 async) /* untyped: caller-owned payload */
{
    s32 result;
    IPCIOVector* vectors = (IPCIOVector*)usbAlloc(0x60);
    s8* endpointBuf = (s8*)usbAlloc(0x20);
    s16* lengthBuf = (s16*)usbAlloc(0x20);

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
                if (result < 0) {
                    if (result == -22) {
                        OSReport("%s: IoctlvAsync returned error %d\n", "__IntrBlkMsgInt", result);
                    }
                    usbFree(request);
                } else {
                    return result;
                }
            }
        }
    }
    usbFree(endpointBuf);
    usbFree(lengthBuf);
    usbFree(vectors);
    return result;
}

/* Starts an interrupt-endpoint read into `data` (the cache range is invalidated first). */
s32 IUSB_ReadIntrMsgAsync(s32 fd, s8 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCInvalidateRange(data, length);
    return __IntrBlkMsgInt(fd, endpoint, length, data, 2, callback, callbackArg, 1);
}

/* Starts a bulk read into `data`: short transfers use the interrupt-style path, long ones the long bulk path. */
s32 IUSB_ReadBlkMsgAsync(s32 fd, s8 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCInvalidateRange(data, length);
    if (length <= 0xFFFF) {
        return __IntrBlkMsgInt(fd, endpoint, length, data, 1, callback, callbackArg, 1);
    }
    return __LongBlkMsgInt(fd, endpoint, length, data, callback, callbackArg, 1);
}

/* Starts a bulk write from `data`: the cache range is flushed first, then the transfer is sized like a read. */
s32 IUSB_WriteBlkMsgAsync(s32 fd, s8 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg) /* untyped: caller-owned payload */
{
    DCFlushRange(data, length);
    if (length <= 0xFFFF) {
        return __IntrBlkMsgInt(fd, endpoint, length, data, 1, callback, callbackArg, 1);
    }
    return __LongBlkMsgInt(fd, endpoint, length, data, callback, callbackArg, 1);
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
