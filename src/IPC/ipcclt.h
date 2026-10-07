/*
 * IPC/ipcclt.h - declarations of the symbols owned by `IPC/ipcclt.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_IPC_IPCCLT_H
#define MHTRI_IPC_IPCCLT_H

#include "types.h"

#include "OS/OSContext.h"
#include "OS/OSThread.h"

/* The completion callback of an asynchronous request: the IOS result and the caller's argument. */
/* untyped: caller-owned payload */
typedef void (*IPCCallback)(s32 result, void* arg);

/* One entry of an `IOS_Ioctlv` vector list. size: 0x8 */
typedef struct IPCIOVector {
    /* +0x0 */ void* base; /* untyped: a byte range of the caller */
    /* +0x4 */ u32 length;
} IPCIOVector;

/* The arguments of an open request. size: 0x8 */
typedef struct IPCOpenArgs {
    /* +0x0 */ const char* path;
    /* +0x4 */ u32 mode;
} IPCOpenArgs;

/* The arguments of a read or write request. size: 0x8 */
typedef struct IPCTransferArgs {
    /* +0x0 */ void* buffer; /* untyped: a byte range of the caller */
    /* +0x4 */ u32 length;
} IPCTransferArgs;

/* The arguments of a seek request. size: 0x8 */
typedef struct IPCSeekArgs {
    /* +0x0 */ s32 offset;
    /* +0x4 */ s32 whence;
} IPCSeekArgs;

/* The arguments of an ioctl request. size: 0x14 */
typedef struct IPCIoctlArgs {
    /* +0x00 */ u32 type;
    /* +0x04 */ void* input; /* untyped: a byte range of the caller */
    /* +0x08 */ u32 inputSize;
    /* +0x0C */ void* output; /* untyped: a byte range of the caller */
    /* +0x10 */ u32 outputSize;
} IPCIoctlArgs;

/* The arguments of an ioctlv request. size: 0x10 */
typedef struct IPCIoctlvArgs {
    /* +0x00 */ u32 type;
    /* +0x04 */ u32 inCount;
    /* +0x08 */ u32 outCount;
    /* +0x0C */ IPCIOVector* vectors;
} IPCIoctlvArgs;

/* The command-specific words of a request. size: 0x14 */
typedef union IPCRequestArgs {
    /* +0x00 */ IPCOpenArgs open;
    /* +0x00 */ IPCTransferArgs transfer;
    /* +0x00 */ IPCSeekArgs seek;
    /* +0x00 */ IPCIoctlArgs ioctl;
    /* +0x00 */ IPCIoctlvArgs ioctlv;
} IPCRequestArgs;

/* An in-flight IPC request record, 32-byte aligned (the first 0x20 bytes are what the IOS reads). size: 0x40 */
typedef struct IPCRequest {
    /* +0x00 */ u32 command;
    /* +0x04 */ s32 result;
    /* +0x08 */ s32 fd; /* the IOS reply carries the request command in this word */
    /* +0x0C */ IPCRequestArgs args;
    /* +0x20 */ IPCCallback callback;
    /* +0x24 */ void* callbackArg; /* untyped: caller-owned payload */
    /* +0x28 */ s32 reboot;
    /* +0x2C */ OSThreadQueue waiters;
    /* +0x34 */ u8 pad_0x34[0xC];
} IPCRequest;

#ifdef __cplusplus
extern "C" {
#endif

/* The IPC client entry points.  `IOS_Read` / `IOS_Write` are the synchronous twins of `IOS_ReadAsync` /
 * `IOS_WriteAsync`; the `Timed` pair opens and closes under a 2 s alarm. */
u32 strnlen(const char* str, u32 maxlen);
void IPCInterruptHandler(s16 interrupt, OSContext* context);
s32 IPCCltInit(void);
s32 IPCCltInitSmallHeap(void);
s32 IOS_OpenAsync(const char* path, u32 mode, void* callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IOS_Open(const char* path, u32 mode);
s32 IOS_OpenTimed(const char* path, u32 mode);
s32 IOS_CloseAsync(s32 fd, void* callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IOS_Close(s32 fd);
s32 IOS_CloseTimed(s32 fd);
/* untyped: a byte range of the caller, and a caller-owned payload */
s32 IOS_ReadAsync(s32 fd, void* buf, s32 len, void* callback, void* callbackArg);
/* untyped: a byte range of the caller */
s32 IOS_Read(s32 fd, void* buf, s32 len);
/* untyped: a byte range of the caller, and a caller-owned payload */
s32 IOS_WriteAsync(s32 fd, const void* buf, s32 len, void* callback, void* callbackArg);
/* untyped: a byte range of the caller */
s32 IOS_Write(s32 fd, const void* buf, s32 len);
s32 IOS_SeekAsync(s32 fd, s32 offset, s32 mode, void* callback, void* callbackArg); /* untyped: caller-owned payload */
/* untyped: byte ranges of the caller, and a caller-owned payload */
s32 IOS_IoctlAsync(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize, void* callback,
                   void* callbackArg);
/* untyped: byte ranges of the caller */
s32 IOS_Ioctl(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize);
s32 IOS_IoctlvAsync(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors, void* callback,
                    void* callbackArg); /* untyped: caller-owned payload */
s32 IOS_Ioctlv(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors);
s32 IOS_IoctlvReboot(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_IPC_IPCCLT_H */
