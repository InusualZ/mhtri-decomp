/*
 * IPC/ipcclt.c - the SDK IPC client (IOS_Open/Close/Read/Write/Seek/Ioctl/Ioctlv, interrupt handler).
 *
 * RANGE. `.text` 0x804BB310-0x804BD070; `.bss` 0x80747300-0x807473C0; `.sdata` 0x80793EA8-0x80793EB0; `.sbss`
 *   0x80795240-0x80795258 (25 functions / 0x1CD8 B).
 *   - `.sdata` `__mailboxAck` / `hid` (0x80793EA8 / 0x80793EAC), `.bss` `__responses`, `__timeout_alarm` and the
 *     reboot buffer (0x80747300..0x807473C0) and `.sbss` 0x80795240..0x80795258 are read only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `__ios_ReplyHandler` is a GUESS, `IPCInterruptHandler` is a GUESS, `IPCCltInitSmallHeap` is a GUESS, `__ios_IpcTimed` is a GUESS, `__ios_TimeoutHandler` is a GUESS, `IOS_OpenTimed` is a GUESS, `IOS_CloseTimed` is a GUESS, `IOS_Read` is a GUESS, `IOS_Write` is a GUESS (from the use: the reply half of the
 *   interrupt handler, the open and close that sleep under a 2 s alarm, the 0x800-byte twin of `IPCCltInit`, the
 *   synchronous twins of the async transfers); the `.sbss` words `__rebootPending`, `__rebootReplyPtr`,
 *   `__rebootRequest`, `__timeoutRequest`, `__timedOut` and the reboot buffer `__rebootBuffer` are GUESSes too;
 *   `IPCRequest` fields are named from the commands that fill them.
 * RESIDUALS. `IOS_IoctlvReboot`: the target forms the ring checks as `(0 - issued) + queued` (`subfic`+`add`) where
 *   `queued - issued` gives `subf`, and it reloads `req` from the stack into a shared saved register (8 B short in
 *   `.text`: flipcheck blocker); `__ios_ReplyHandler`: the target dispatches `{3, 6, 7}` through a pivot tree
 *   (`cmpwi 6; beq; bge`) where the switch here is a linear chain, and keeps a separate vector byte offset;
 *   `IOS_Open` / `IOS_OpenAsync` / `IOS_OpenTimed`: the inlined `strnlen` takes r3/r4 swapped (path pointer and
 *   count). The `__ios_*` statics are out-of-line in the target and the other helpers are inlined: they are
 *   `static inline` here. The ring check expressions are kept as the explicit `a < b ? a - b : ...` forms that
 *   reproduce the target's branchy compare.
 */

#include "types.h"
#include "IPC/ipcclt.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcProfile.h"
#include "IPC/memory.h"
#include "OS/OSCache.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSThread.h"

#define IPC_REG_STATUS 1
#define IPC_REG_ACK_MASK 0x30
#define IPC_REQUEST_SIZE 0x40
#define IPC_REQUEST_ALIGN 0x20
#define IPC_RING_SIZE 16
#define IPC_TIMEOUT_MS 2000
#define PHYS_OFFSET 0x80000000

/* The ring of submitted requests and its counters. size: 0x50 */
typedef struct IPCResponses {
    /* +0x00 */ u32 issued;
    /* +0x04 */ u32 queued;
    /* +0x08 */ u32 readIndex;
    /* +0x0C */ u32 writeIndex;
    /* +0x10 */ IPCRequest* slots[IPC_RING_SIZE];
} IPCResponses;

static IPCRequest __rebootBuffer;
static OSAlarm __timeout_alarm;
static IPCResponses __responses;

static s32 __mailboxAck = 1;
static s32 hid = -1;

static s32 __timedOut;
static IPCRequest* __timeoutRequest;
static s32 __rebootRequest;
static IPCRequest* __rebootReplyPtr;
static s32 __rebootPending;

u32 strnlen(const char* str, u32 maxlen)
{
    const u8* p = (const u8*)str;

    while (*p != 0 && maxlen-- != 0) {
        p++;
    }
    return p - (const u8*)str;
}

static inline int IPCRingEmpty(void)
{
    if (__responses.queued < __responses.issued) {
        return __responses.queued - __responses.issued;
    }
    return (__responses.queued - __responses.issued) == 0;
}

static inline int IPCRingFull(void)
{
    if (__responses.queued < __responses.issued) {
        return __responses.queued - __responses.issued;
    }
    return __responses.queued - __responses.issued >= IPC_RING_SIZE;
}

/* Hands the next queued request to the IOS. */
static inline void IPCSubmitNext(void)
{
    IPCRequest* req;

    if (!IPCRingEmpty()) {
        req = __responses.slots[__responses.readIndex];
        if (req != NULL) {
            if (req->reboot != 0) {
                __mailboxAck--;
            }
            IPCWriteReg(0, (u32)req + PHYS_OFFSET);
            __responses.readIndex = (__responses.readIndex + 1) & (IPC_RING_SIZE - 1);
            __responses.issued++;
            __mailboxAck--;
            IPCWriteReg(IPC_REG_STATUS, (IPCReadReg(IPC_REG_STATUS) & IPC_REG_ACK_MASK) | 1);
        }
    }
}

/* The IOS reply half of the interrupt handler: completes the finished request. */
static void __ios_ReplyHandler(s16 interrupt, OSContext* context)
{
    u32 phys = IPCReadReg(2);
    IPCRequest* req;
    OSContext ctx;

    if (phys != 0) {
        req = (IPCRequest*)(phys + PHYS_OFFSET);
        IPCWriteReg(IPC_REG_STATUS, (IPCReadReg(IPC_REG_STATUS) & IPC_REG_ACK_MASK) | 4);
        *(volatile u32*)0xCD000030 = 0x40000000;
        DCInvalidateRange(req, 0x20);
        switch (req->fd) { /* the reply carries the request command in this word */
        case 3:
            req->args.transfer.buffer = req->args.transfer.buffer != NULL
                                            ? (void*)((u32)req->args.transfer.buffer + PHYS_OFFSET)
                                            : NULL;
            if (req->result > 0) {
                DCInvalidateRange(req->args.transfer.buffer, req->result);
            }
            break;
        case 6:
            req->args.ioctl.output =
                req->args.ioctl.output != NULL ? (void*)((u32)req->args.ioctl.output + PHYS_OFFSET) : NULL;
            DCInvalidateRange(req->args.ioctl.input, req->args.ioctl.inputSize);
            DCInvalidateRange(req->args.ioctl.output, req->args.ioctl.outputSize);
            break;
        case 7: {
            u32 i;
            IPCIOVector* vec;

            req->args.ioctlv.vectors = req->args.ioctlv.vectors != NULL
                                           ? (IPCIOVector*)((u32)req->args.ioctlv.vectors + PHYS_OFFSET)
                                           : NULL;
            DCInvalidateRange(req->args.ioctlv.vectors, (req->args.ioctlv.inCount + req->args.ioctlv.outCount) * 8);
            for (i = 0; i < req->args.ioctlv.inCount + req->args.ioctlv.outCount; i++) {
                vec = req->args.ioctlv.vectors;
                vec[i].base = vec[i].base != NULL ? (void*)((u32)vec[i].base + PHYS_OFFSET) : NULL;
                vec = req->args.ioctlv.vectors;
                DCInvalidateRange(vec[i].base, vec[i].length);
            }
            if (__rebootPending != 0 && (u32)__rebootRequest == (u32)req) {
                __rebootPending = 0;
                if (__mailboxAck < 1) {
                    __mailboxAck++;
                }
            }
            break;
        }
        }
        if (req->callback != NULL) {
            OSClearContext(&ctx);
            OSSetCurrentContext(&ctx);
            req->callback(req->result, req->callbackArg);
            OSClearContext(&ctx);
            OSSetCurrentContext(context);
            iosFree(hid, req);
        } else {
            OSWakeupThread(&req->waiters);
        }
        IPCWriteReg(IPC_REG_STATUS, (IPCReadReg(IPC_REG_STATUS) & IPC_REG_ACK_MASK) | 8);
        IPCiProfReply(req, req->fd);
    }
}

/* The IPC hardware interrupt: completes a reply and hands out the next request. */
void IPCInterruptHandler(s16 interrupt, OSContext* context)
{
    s32 ack;

    if ((IPCReadReg(IPC_REG_STATUS) & 0x14) == 0x14) {
        __ios_ReplyHandler(interrupt, context);
    }
    if ((IPCReadReg(IPC_REG_STATUS) & 0x22) == 0x22) {
        IPCWriteReg(IPC_REG_STATUS, (IPCReadReg(IPC_REG_STATUS) & IPC_REG_ACK_MASK) | 2);
        *(volatile u32*)0xCD000030 = 0x40000000;
        ack = __mailboxAck;
        if (ack < 1) {
            ack++;
            __mailboxAck = ack;
            IPCiProfAck();
        }
        if (ack > 0) {
            if (__rebootPending != 0) {
                __rebootReplyPtr->result = 0;
                __rebootPending = 0;
                OSWakeupThread(&__rebootReplyPtr->waiters);
                IPCWriteReg(IPC_REG_STATUS, (IPCReadReg(IPC_REG_STATUS) & IPC_REG_ACK_MASK) | 8);
            }
            IPCSubmitNext();
        }
    }
}

/* Carves the request heap out of the IPC buffer, installs the interrupt handler and enables the IPC interrupt. */
s32 IPCCltInit(void)
{
    static s32 initialized = 0;
    u8* hi;
    s32 ret;
    u8* lo;

    ret = 0;
    if (initialized == 0) {
        initialized = 1;
        IPCInit();
        lo = IPCGetBufferLo();
        hi = lo + 0x1000;
        if (hi > IPCGetBufferHi()) {
            ret = -0x16;
        } else {
            hid = iosCreateHeap(lo, 0x1000);
            IPCSetBufferLo(hi);
            __OSSetInterruptHandler(0x1B, IPCInterruptHandler);
            __OSUnmaskInterrupts(0x10);
            IPCWriteReg(IPC_REG_STATUS, 0x38);
            IPCiProfInit();
            OSCreateAlarm(&__timeout_alarm);
        }
    }
    return ret;
}

/* Carves a 0x800-byte request heap out of the IPC buffer. */
s32 IPCCltInitSmallHeap(void)
{
    u8* hi;
    s32 ret;
    u8* lo;

    ret = 0;
    lo = IPCGetBufferLo();
    hi = lo + 0x800;

    if (hi > IPCGetBufferHi()) {
        ret = -0x16;
    } else {
        hid = iosCreateHeap(lo, 0x800);
        IPCSetBufferLo(hi);
    }
    return ret;
}

/* Queues a filled request and returns -8 when the ring is full. */
static inline s32 IPCEnqueue(IPCRequest* req)
{
    s32 ret = 0;

    if (IPCRingFull()) {
        ret = -8;
    } else {
        __responses.slots[__responses.writeIndex] = req;
        __responses.writeIndex = (__responses.writeIndex + 1) & (IPC_RING_SIZE - 1);
        __responses.queued++;
        IPCiProfQueueReq(req, req->fd);
    }
    return ret;
}

/* Sends a request; a null `callback` makes the call synchronous (the caller sleeps until the reply). */
static s32 __ios_Ipc2(IPCRequest* req, IPCCallback callback)
{
    s32 ret;
    s32 level;

    if (req == NULL) {
        ret = -4;
    } else {
        if (callback == NULL) {
            OSInitThreadQueue(&req->waiters);
        }
        DCFlushRange(req, 0x20);
        level = OSDisableInterrupts();
        ret = IPCEnqueue(req);
        if (ret != 0) {
            OSRestoreInterrupts(level);
            if (callback != NULL) {
                iosFree(hid, req);
            }
        } else {
            if (__mailboxAck > 0) {
                IPCSubmitNext();
            }
            if (callback == NULL) {
                OSSleepThread(&req->waiters);
            }
            OSRestoreInterrupts(level);
            if (callback == NULL) {
                ret = req->result;
            }
        }
    }
    if (req != NULL && callback == NULL) {
        iosFree(hid, req);
    }
    return ret;
}

static void __ios_TimeoutHandler(OSAlarm* alarm, OSContext* context);

/* Sends a request and sleeps under a 2 s alarm; -3 when the alarm fired first. */
static s32 __ios_IpcTimed(IPCRequest* req)
{
    s32 ret;
    s32 level;

    if (req == NULL) {
        ret = -4;
    } else {
        OSInitThreadQueue(&req->waiters);
        DCFlushRange(req, 0x20);
        level = OSDisableInterrupts();
        ret = IPCEnqueue(req);
        if (ret != 0) {
            OSRestoreInterrupts(level);
            iosFree(hid, req);
        } else {
            if (__mailboxAck > 0) {
                IPCSubmitNext();
            }
            __timedOut = 0;
            __timeoutRequest = req;
            OSSetAlarm(&__timeout_alarm, (*(u32*)0x800000F8 / 4 / 1000) * IPC_TIMEOUT_MS, __ios_TimeoutHandler);
            OSSleepThread(&req->waiters);
            OSRestoreInterrupts(level);
            if (__timedOut == 0) {
                OSCancelAlarm(&__timeout_alarm);
                ret = req->result;
                iosFree(hid, req);
            } else {
                ret = -3;
            }
        }
    }
    return ret;
}

/* Marks the pending timed request as timed out and wakes its sleeper. */
static void __ios_TimeoutHandler(OSAlarm* alarm, OSContext* context)
{
    __timedOut = 1;
    OSWakeupThread(&__timeoutRequest->waiters);
}

/* Allocates a request and fills its common header. */
/* untyped: caller-owned buffers and payloads */
static inline s32 IPCAllocRequest(IPCRequest** out, u32 command, s32 fd, IPCCallback callback, void* callbackArg)
{
    s32 ret = 0;

    if (out == NULL) {
        ret = -4;
    } else {
        IPCRequest* req = iosAllocAligned(hid, IPC_REQUEST_SIZE, IPC_REQUEST_ALIGN);
        *out = req;
        if (req == NULL) {
            ret = -0x16;
        } else {
            (*out)->callback = callback;
            (*out)->callbackArg = callbackArg;
            (*out)->reboot = 0;
            req->command = command;
            req->fd = fd;
        }
    }
    return ret;
}

/* Fills the path and mode of an open request. */
static inline s32 IPCFillOpen(IPCRequest* req, const char* path, u32 mode)
{
    s32 ret = 0;

    if (req == NULL) {
        ret = -4;
    } else {
        DCFlushRange((void*)path, strnlen(path, 0x40) + 1);
        req->args.open.path = (const char*)((u32)path + PHYS_OFFSET);
        req->args.open.mode = mode;
    }
    return ret;
}

/* Fills the buffer of a read request. */
/* untyped: caller-owned buffers and payloads */
static inline s32 IPCFillRead(IPCRequest* req, void* buffer, s32 length)
{
    s32 ret = 0;

    if (req == NULL) {
        ret = -4;
    } else {
        DCInvalidateRange(buffer, length);
        req->args.transfer.buffer = buffer != NULL ? (void*)((u32)buffer + PHYS_OFFSET) : NULL;
        req->args.transfer.length = length;
    }
    return ret;
}

/* Fills the buffer of a write request. */
/* untyped: caller-owned buffers and payloads */
static inline s32 IPCFillWrite(IPCRequest* req, const void* buffer, s32 length)
{
    s32 ret = 0;

    if (req == NULL) {
        ret = -4;
    } else {
        req->args.transfer.buffer = buffer != NULL ? (void*)((u32)buffer + PHYS_OFFSET) : NULL;
        req->args.transfer.length = length;
        DCFlushRange((void*)buffer, length);
    }
    return ret;
}

/* Fills the offset and origin of a seek request. */
static inline s32 IPCFillSeek(IPCRequest* req, s32 offset, s32 whence)
{
    s32 ret = 0;

    if (req == NULL) {
        ret = -4;
    } else {
        req->args.seek.offset = offset;
        req->args.seek.whence = whence;
    }
    return ret;
}

/* Fills the buffers of an ioctl request. */
/* untyped: caller-owned buffers and payloads */
static inline s32 IPCFillIoctl(IPCRequest* req, s32 type, void* in, s32 inSize, void* out, s32 outSize)
{
    s32 ret = 0;

    if (req == NULL) {
        ret = -4;
    } else {
        req->args.ioctl.type = type;
        req->args.ioctl.output = out != NULL ? (void*)((u32)out + PHYS_OFFSET) : NULL;
        req->args.ioctl.outputSize = outSize;
        req->args.ioctl.input = in != NULL ? (void*)((u32)in + PHYS_OFFSET) : NULL;
        req->args.ioctl.inputSize = inSize;
        DCFlushRange(in, inSize);
        DCFlushRange(out, outSize);
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_OpenAsync(const char* path, u32 mode, void* callback, void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 1, 0, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = IPCFillOpen(req, path, mode);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

s32 IOS_Open(const char* path, u32 mode)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 1, 0, NULL, NULL);

    if (ret == 0) {
        ret = IPCFillOpen(req, path, mode);
        if (ret == 0) {
            ret = __ios_Ipc2(req, NULL);
        }
    }
    return ret;
}

/* Opens a device synchronously under the 2 s timeout alarm. */
s32 IOS_OpenTimed(const char* path, u32 mode)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 1, 0, NULL, NULL);

    if (ret == 0) {
        ret = IPCFillOpen(req, path, mode);
        if (ret == 0) {
            ret = __ios_IpcTimed(req);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_CloseAsync(s32 fd, void* callback, void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 2, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = __ios_Ipc2(req, (IPCCallback)callback);
    }
    return ret;
}

s32 IOS_Close(s32 fd)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 2, fd, NULL, NULL);

    if (ret == 0) {
        ret = __ios_Ipc2(req, NULL);
    }
    return ret;
}

/* Closes a device synchronously under the 2 s timeout alarm. */
s32 IOS_CloseTimed(s32 fd)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 2, fd, NULL, NULL);

    if (ret == 0) {
        ret = __ios_IpcTimed(req);
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_ReadAsync(s32 fd, void* buf, s32 len, void* callback, void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 3, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = IPCFillRead(req, buf, len);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_Read(s32 fd, void* buf, s32 len)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 3, fd, NULL, NULL);

    if (ret == 0) {
        ret = IPCFillRead(req, buf, len);
        if (ret == 0) {
            ret = __ios_Ipc2(req, NULL);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_WriteAsync(s32 fd, const void* buf, s32 len, void* callback, void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 4, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = IPCFillWrite(req, buf, len);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_Write(s32 fd, const void* buf, s32 len)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 4, fd, NULL, NULL);

    if (ret == 0) {
        ret = IPCFillWrite(req, buf, len);
        if (ret == 0) {
            ret = __ios_Ipc2(req, NULL);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_SeekAsync(s32 fd, s32 offset, s32 mode, void* callback, void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 5, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = IPCFillSeek(req, offset, mode);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_IoctlAsync(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize, void* callback,
                   void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 6, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = IPCFillIoctl(req, type, in, inSize, out, outSize);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_Ioctl(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 6, fd, NULL, NULL);

    if (ret == 0) {
        ret = IPCFillIoctl(req, type, in, inSize, out, outSize);
        if (ret == 0) {
            ret = __ios_Ipc2(req, NULL);
        }
    }
    return ret;
}

/* Converts the vector list of an ioctlv request to physical addresses and flushes it. */
static s32 __ios_Ioctlv(IPCRequest* req, s32 type, u32 inCount, u32 outCount, IPCIOVector* vectors)
{
    s32 ret = 0;
    u32 i;

    if (req == NULL) {
        ret = -4;
    } else {
        req->args.ioctlv.type = type;
        req->args.ioctlv.inCount = inCount;
        req->args.ioctlv.outCount = outCount;
        req->args.ioctlv.vectors = vectors;
        for (i = 0; i < req->args.ioctlv.outCount; i++) {
            IPCIOVector* vec = &(req->args.ioctlv.vectors + inCount)[i];
            DCFlushRange(vec->base, vec->length);
            vec = &(req->args.ioctlv.vectors + inCount)[i];
            vec->base = vec->base != NULL ? (void*)((u32)vec->base + PHYS_OFFSET) : NULL;
        }
        for (i = 0; i < req->args.ioctlv.inCount; i++) {
            IPCIOVector* vec = &req->args.ioctlv.vectors[i];
            DCFlushRange(vec->base, vec->length);
            vec = &req->args.ioctlv.vectors[i];
            vec->base = vec->base != NULL ? (void*)((u32)vec->base + PHYS_OFFSET) : NULL;
        }
        DCFlushRange(req->args.ioctlv.vectors, (req->args.ioctlv.inCount + req->args.ioctlv.outCount) * 8);
        req->args.ioctlv.vectors = vectors != NULL ? (IPCIOVector*)((u32)vectors + PHYS_OFFSET) : NULL;
    }
    return ret;
}

/* untyped: caller-owned buffers and payloads */
s32 IOS_IoctlvAsync(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors, void* callback,
                    void* callbackArg)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 7, fd, (IPCCallback)callback, callbackArg);

    if (ret == 0) {
        ret = __ios_Ioctlv(req, type, inCount, outCount, vectors);
        if (ret == 0) {
            ret = __ios_Ipc2(req, (IPCCallback)callback);
        }
    }
    return ret;
}

s32 IOS_Ioctlv(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors)
{
    IPCRequest* req;
    s32 ret = IPCAllocRequest(&req, 7, fd, NULL, NULL);

    if (ret == 0) {
        ret = __ios_Ioctlv(req, type, inCount, outCount, vectors);
        if (ret == 0) {
            ret = __ios_Ipc2(req, NULL);
        }
    }
    return ret;
}

/* Sends the reboot ioctlv: the request is copied into a static buffer so the reply survives the heap teardown. */
s32 IOS_IoctlvReboot(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors)
{
    s32 ret;
    IPCRequest* req;
    s32 level;
    s32 level2;

    level = OSDisableInterrupts();
    if (__rebootPending != 0) {
        OSRestoreInterrupts(level);
        ret = -10;
    } else {
        __rebootPending = 1;
        OSRestoreInterrupts(level);
        ret = IPCAllocRequest(&req, 7, fd, NULL, NULL);
        if (ret == 0) {
            __rebootRequest = (s32)req;
            req->reboot = 1;
            ret = __ios_Ioctlv(req, type, inCount, outCount, vectors);
            if (ret == 0) {
                memcpy(&__rebootBuffer, req, IPC_REQUEST_SIZE);
                __rebootReplyPtr = &__rebootBuffer;
                OSInitThreadQueue(&__rebootBuffer.waiters);
                DCFlushRange(req, 0x20);
                level2 = OSDisableInterrupts();
                ret = IPCEnqueue(req);
                if (ret != 0) {
                    OSRestoreInterrupts(level2);
                } else {
                    if (__mailboxAck > 0) {
                        IPCSubmitNext();
                    }
                    OSSleepThread(&__rebootReplyPtr->waiters);
                    OSRestoreInterrupts(level2);
                    ret = __rebootReplyPtr->result;
                }
            }
        }
        __rebootPending = 0;
        __rebootRequest = 0;
        if (req != NULL && ret != 0) {
            iosFree(hid, req);
        }
    }
    return ret;
}
