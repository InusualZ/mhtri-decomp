/*
 * RVLGX/GXAttr.h - declarations of the symbols owned by `RVLGX/GXAttr.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXATTR_H
#define MHTRI_RVLGX_GXATTR_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetVtxDesc(u32 attr, u32 type);
void GXGetVtxDesc(u32 attr, s32* type);
void GXClearVtxDesc(void);
void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
void GXGetVtxAttrFmt(u32 vtxfmt, u32 attr, s32* cnt, s32* type, u8* frac);
/* 0x804B5AE0 - points GX vertex attribute `attr` at an array of `stride`-byte entries. */
void GXSetArray(u32 attr, const void* pBase, u8 stride); /* untyped: byte range */
void GXSetTexCoordGen2(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);
void GXSetNumTexGens(u8 nTexGens);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXATTR_H */
