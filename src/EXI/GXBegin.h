/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the Revolution SDK GX entry points whose map addresses fall in
 * `EXI/ProbeBarnacle.c`'s registered range (0x804B17D0-0x804B8020: `GXAttr.c`, `GXGeometry.c`, `GXLight.c`), so that
 * unit is their owner here.  The prototypes are the SDK's, with each enumeration spelled as its 32-bit integer and
 * each `GXBool` as `u8`.
 */
#ifndef MHTRI_EXI_GXBEGIN_H
#define MHTRI_EXI_GXBEGIN_H

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
/* untyped: byte range - the vertex attribute array GX indexes */
void GXSetArray(u32 attr, const void* base, u8 stride);
void GXSetNumTexGens(u8 count);
void GXBegin(u32 type, u32 vtxfmt, u16 nverts);
void GXSetLineWidth(u8 width, u32 tex_offsets);
void GXSetPointSize(u8 size, u32 tex_offsets);
void GXEnableTexOffsets(u32 coord, u8 line_enable, u8 point_enable);
void GXSetCullMode(u32 mode);
void GXSetCoPlanar(u8 enable);
void GXSetChanAmbColor(u32 chan, GXColor color);
void GXSetChanMatColor(u32 chan, GXColor color);
void GXSetNumChans(u8 count);
void GXSetChanCtrl(u32 chan, u8 enable, u32 amb_src, u32 mat_src, u32 light_mask, u32 diff_fn, u32 attn_fn);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EXI_GXBEGIN_H */
