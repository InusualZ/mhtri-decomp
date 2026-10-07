/*
 * IPC/ipcclt.h - declarations of the symbols owned by `IPC/ipcclt.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_IPC_IPCCLT_H
#define MHTRI_IPC_IPCCLT_H

#include "types.h"

/* One entry of an `IOS_Ioctlv` vector list. size: 0x8 */
typedef struct IPCIOVector {
    void* base; /* +0x0 */
    u32 length; /* +0x4 */
} IPCIOVector;

#ifdef __cplusplus
extern "C" {
#endif

/* The IPC client entry points.  `IOS_Read` / `IOS_Write` are the map's `fn_804BC3F0` / `fn_804BC600` (the two
 * 0x108-byte bodies between IOS_ReadAsync's and IOS_SeekAsync's neighbours, the only gap in the IOS_* run). */
u32 strnlen(const char* str, u32 maxlen);
s32 IOS_OpenAsync(const char* path, u32 mode, void* callback, void* callbackArg);
s32 IOS_Open(const char* path, u32 mode);
s32 IOS_CloseAsync(s32 fd, void* callback, void* callbackArg);
s32 IOS_Close(s32 fd);
s32 IOS_ReadAsync(s32 fd, void* buf, s32 len, void* callback, void* callbackArg);
s32 fn_804BC3F0(s32 fd, void* buf, s32 len);
s32 IOS_WriteAsync(s32 fd, const void* buf, s32 len, void* callback, void* callbackArg);
s32 fn_804BC600(s32 fd, const void* buf, s32 len);
s32 IOS_SeekAsync(s32 fd, s32 offset, s32 mode, void* callback, void* callbackArg);
s32 IOS_IoctlAsync(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize, void* callback,
                   void* callbackArg);
s32 IOS_Ioctl(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize);
s32 IOS_IoctlvAsync(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors, void* callback,
                    void* callbackArg);
s32 IOS_Ioctlv(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_IPC_IPCCLT_H */
