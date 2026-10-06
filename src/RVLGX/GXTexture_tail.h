/*
 * RVLGX/GXTexture_tail.h - the IPC client entry points and types the FS library of EXI/ProbeBarnacle drives; they are owned by
 * RVLGX/GXTexture_tail (the IPC run of 0x804B8020..0x804C1760).
 */
#ifndef RVLGX_GXTEXTURE_TAIL_H
#define RVLGX_GXTEXTURE_TAIL_H

#include "types.h"
#include "gx.h"

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

/* 0x804B8020..0x804BA1B0 - the texture-object initialisers and the display-list call (callers: g3d/g3d_resmat.cpp). */
void GXInitTexObj(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrapS, u32 wrapT,
                  u8 mipmap); /* untyped: byte range */
void GXInitTexObjCI(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrapS, u32 wrapT,
                    u8 mipmap, u32 tlutName); /* untyped: byte range */
void GXInitTexObjLOD(GXTexObj* obj, u32 minFilt, u32 magFilt, f32 minLod, f32 maxLod, f32 lodBias, u8 biasClamp,
                     u8 doEdgeLod, u32 maxAniso);
void GXInitTlutObj(GXTlutObj* obj, void* lut, u32 format, u16 numEntries); /* untyped: byte range */
void GXCallDisplayList(const void* list, u32 size); /* untyped: byte range */
/* 0x804B8720 - loads a texture object into texture slot `id`. */
void GXLoadTexObj(GXTexObj* obj, u32 id);
/* 0x804BA560 / 0x804BA5F0 - the indexed matrix loads of the GX transform block (the SDK order: GXLoadPosMtxImm
 * 0x804BA510, GXLoadPosMtxIndx, GXLoadNrmMtxImm 0x804BA590, GXLoadNrmMtxIndx3x3); `g3d/g3d_state.cpp`'s shape load
 * calls both with (matrix index, 0). */
void GXLoadPosMtxIndx(u16 mtxIndx, u32 id);
void GXLoadNrmMtxIndx3x3(u16 mtxIndx, u32 id);
/* The fog, blend and TEV setters the nw4r character writer drives. */
void GXSetFog(u32 type, f32 startz, f32 endz, f32 nearz, f32 farz, GXColor color);
void GXSetTevSwapModeTable(u32 table, u32 red, u32 green, u32 blue, u32 alpha);
void GXSetZTexture(u32 op, u32 fmt, u32 bias);
void GXSetNumIndStages(u8 nIndStages);
void GXSetBlendMode(u32 type, u32 srcFactor, u32 dstFactor, u32 op);
void GXSetNumTevStages(u8 nStages);
void GXSetTevDirect(u32 stage);
void GXSetTevSwapMode(u32 stage, u32 rasSel, u32 texSel);
void GXSetTevOrder(u32 stage, u32 coord, u32 map, u32 color);
void GXSetTevColorIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevAlphaIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevColorOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 outReg);
void GXSetTevAlphaOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 outReg);
void GXSetTevOp(u32 stage, u32 mode);
void GXSetTevColor(u32 reg, GXColor color);

#ifdef __cplusplus
}
#endif

#endif
