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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTRANSFORM_H */
