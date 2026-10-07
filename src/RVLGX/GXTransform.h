/*
 * RVLGX/GXTransform.h - declarations of the symbols owned by `RVLGX/GXTransform.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXTRANSFORM_H
#define MHTRI_RVLGX_GXTRANSFORM_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXGetProjectionv(f32* proj);
void GXLoadPosMtxImm(const f32 mtx[][4], u32 id);
void GXLoadPosMtxIndx(u16 mtxIndx, u32 id);
/* 0x804BA590 - loads a 3x4 matrix into the XF normal-matrix block of slot `id`. */
void GXLoadNrmMtxImm(const f32 mtx[][4], u32 id);
void GXLoadNrmMtxIndx3x3(u16 mtxIndx, u32 id);
void GXSetCurrentMtx(u32 id);
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, u32 type);
void GXSetClipMode(u32 mode);
/* 0x804BA410 - sets the projection matrix and type. */
void GXSetProjection(const f32 mtx[][4], s32 type);
/* 0x804BA7E0 - sets the viewport rectangle and depth range. */
void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ);
/* 0x804BA7A0 - sets the viewport, shifted for the even field of an interlaced frame. */
void GXSetViewportJitter(u32 field, f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ);
/* 0x804BA830 - sets the scissor rectangle. */
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
/* 0x804BA8A0 - reads the scissor rectangle. */
void GXGetScissor(u32* left, u32* top, u32* width, u32* height);
/* 0x804BA8F0 - sets the scissor box offset. */
void GXSetScissorBoxOffset(s32 xOffset, s32 yOffset);
/* 0x804BA960 - flushes the vertex-matrix index words. */
void __GXSetMatrixIndex(s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTRANSFORM_H */
