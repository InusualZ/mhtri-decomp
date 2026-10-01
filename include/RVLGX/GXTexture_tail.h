/*
 * RVLGX/GXTexture_tail.h - the IPC client entry points and types the FS library of EXI/ProbeBarnacle drives; they are owned by
 * RVLGX/GXTexture_tail (the IPC run of 0x804B8020..0x804C1760).
 */
#ifndef RVLGX_GXTEXTURE_TAIL_H
#define RVLGX_GXTEXTURE_TAIL_H

#include "types.h"

/* One entry of an `IOS_Ioctlv` vector list. size: 0x8 */
typedef struct IPCIOVector {
    void* base; /* +0x0 */
    u32 length; /* +0x4 */
} IPCIOVector;

#ifdef __cplusplus
extern "C" {
#endif

/* The IPC client the FS library drives.  Unclaimed in the map (the IPC library sits below the band), so
 * bare prototypes - the lint's rule 2 named gap.  `IOS_Read` / `IOS_Write` are the map's `fn_804BC3F0` /
 * `fn_804BC600` (the two 0x108-byte bodies between IOS_ReadAsync's and IOS_SeekAsync's neighbours, the
 * only gap in the IOS_* run). */
s32 IOS_Open(const char* path, u32 mode);
s32 IOS_OpenAsync(const char* path, u32 mode, void* callback, void* callbackArg);
s32 IOS_Close(s32 fd);
s32 IOS_CloseAsync(s32 fd, void* callback, void* callbackArg);
s32 fn_804BC3F0(s32 fd, void* buf, s32 len);
s32 fn_804BC600(s32 fd, const void* buf, s32 len);
s32 IOS_ReadAsync(s32 fd, void* buf, s32 len, void* callback, void* callbackArg);
s32 IOS_WriteAsync(s32 fd, const void* buf, s32 len, void* callback, void* callbackArg);
s32 IOS_SeekAsync(s32 fd, s32 offset, s32 mode, void* callback, void* callbackArg);
s32 IOS_Ioctl(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize);
s32 IOS_IoctlAsync(s32 fd, s32 type, void* in, s32 inSize, void* out, s32 outSize, void* callback,
                   void* callbackArg);
s32 IOS_Ioctlv(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors);
s32 IOS_IoctlvAsync(s32 fd, s32 type, s32 inCount, s32 outCount, IPCIOVector* vectors, void* callback,
                    void* callbackArg);
s32 iosCreateHeap(void* base, u32 size);
void* iosAllocAligned(s32 handle, u32 size, u32 align);
void iosFree(s32 handle, void* ptr);
void* IPCGetBufferLo(void);
void* IPCGetBufferHi(void);
void IPCSetBufferLo(void* lo);
u32 strnlen(const char* str, u32 maxlen);

#ifdef __cplusplus
}
#endif

#endif
