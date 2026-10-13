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

/* One entry of a `GXSetVtxAttrFmtv` list; the list ends at attr 0xFF (GX_VA_NULL). size: 0x10 */
typedef struct GXVtxAttrFmtList {
    /* +0x0 */ s32 attr;
    /* +0x4 */ s32 cnt;
    /* +0x8 */ s32 type;
    /* +0xC */ u8 frac;
    /* +0xD */ u8 pad_0xD[3];
} GXVtxAttrFmtList;

void GXSetVtxDesc(s32 attr, s32 type);
void GXGetVtxDesc(s32 attr, s32* type);
void GXClearVtxDesc(void);
void GXSetVtxAttrFmt(u32 vtxfmt, s32 attr, s32 cnt, s32 type, u8 frac);
void GXSetVtxAttrFmtv(u32 vtxfmt, const GXVtxAttrFmtList* list);
void GXGetVtxAttrFmt(u32 vtxfmt, s32 attr, s32* cnt, s32* type, u8* frac);
/* 0x804B5AE0 - points GX vertex attribute `attr` at an array of `stride`-byte entries. */
void GXSetArray(s32 attr, const void* pBase, u8 stride); /* untyped: byte range */
void GXInvalidateVtxCache(void);
void GXSetTexCoordGen2(s32 dst_coord, s32 func, s32 src_param, u32 mtx, u8 normalize, u32 pt_texmtx);
void GXSetNumTexGens(u8 nTexGens);
void __GXSetVCD(void);
void __GXSetVAT(void);
void __GXCalculateVLim(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXATTR_H */
