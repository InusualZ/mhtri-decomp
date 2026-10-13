/*
 * RVLGX/GXTransform.h - declarations of the symbols owned by `RVLGX/GXTransform.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXTRANSFORM_H
#define MHTRI_RVLGX_GXTRANSFORM_H

#include "types.h"
#include "gx.h"

/* The projection kinds `GXSetProjection` stores (the SDK's `GXProjectionType`). size: 0x4 */
typedef enum GXProjectionType {
    GX_PERSPECTIVE,
    GX_ORTHOGRAPHIC
} GXProjectionType;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804BA230 (0x188): projects a model-space point to screen coordinates. */
void GXProject(f32 x, f32 y, f32 z, const f32 mtx[3][4], const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz);
/* 0x804BA3C0 (0x44): sends the projection to the XF registers. */
void __GXSetProjection(void);
/* 0x804BA410 (0x64): sets the projection matrix and type. */
void GXSetProjection(const f32 mtx[][4], s32 type);
/* 0x804BA480 (0x4C): sets the projection from a seven-float vector. */
void GXSetProjectionv(const f32* ptr);
/* 0x804BA4D0 (0x40): reads the projection back as a seven-float vector. */
void GXGetProjectionv(f32* ptr);
void GXLoadPosMtxImm(const f32 mtx[][4], u32 id);
void GXLoadPosMtxIndx(u16 mtxIndx, u32 id);
/* 0x804BA590 (0x58): loads a 3x4 matrix into the XF normal-matrix block of slot `id`. */
void GXLoadNrmMtxImm(const f32 mtx[][4], u32 id);
void GXLoadNrmMtxIndx3x3(u16 mtxIndx, u32 id);
void GXSetCurrentMtx(u32 id);
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, s32 type);
/* 0x804BA710 (0x90): sends the viewport scale and offset to the XF registers. */
void __GXSetViewport(void);
/* 0x804BA7A0 (0x3C): sets the viewport, shifted for the even field of an interlaced frame. */
void GXSetViewportJitter(u32 field, f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ);
/* 0x804BA7E0 (0x2C): sets the viewport rectangle and depth range. */
void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ);
/* 0x804BA810 (0x20): reads the viewport rectangle and depth range back. */
void GXGetViewportv(f32* vp);
/* 0x804BA830 (0x68): sets the scissor rectangle. */
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
/* 0x804BA8A0 (0x48): reads the scissor rectangle. */
void GXGetScissor(u32* left, u32* top, u32* width, u32* height);
/* 0x804BA8F0 (0x3C): sets the scissor box offset. */
void GXSetScissorBoxOffset(s32 xOffset, s32 yOffset);
void GXSetClipMode(u32 mode);
/* 0x804BA960 (0x88): flushes the vertex-matrix index words. */
void __GXSetMatrixIndex(s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTRANSFORM_H */
