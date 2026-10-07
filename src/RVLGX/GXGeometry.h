/*
 * RVLGX/GXGeometry.h - declarations of the symbols owned by `RVLGX/GXGeometry.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXGEOMETRY_H
#define MHTRI_RVLGX_GXGEOMETRY_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B6870 - opens a primitive of `nverts` vertices of type `type` in vertex format `vtxfmt`. */
void GXBegin(u32 type, u32 vtxfmt, u16 nverts);
void GXSetLineWidth(u8 width, u32 tex_offsets);
void GXSetPointSize(u8 size, u32 tex_offsets);
void GXEnableTexOffsets(u32 coord, u8 line_enable, u8 point_enable);
void GXSetCullMode(u32 mode);
void GXSetCoPlanar(u8 enable);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXGEOMETRY_H */
