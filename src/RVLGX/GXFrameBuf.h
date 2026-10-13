/*
 * RVLGX/GXFrameBuf.h - declarations of the symbols owned by `RVLGX/GXFrameBuf.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXFRAMEBUF_H
#define MHTRI_RVLGX_GXFRAMEBUF_H

#include "types.h"
#include "gx.h"
#include "gx/GXRenderModeObj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXNtsc480ProgSoft;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXEurgb60Hz480IntDf;

void GXAdjustForOverscan(const GXRenderModeObj* rmin, GXRenderModeObj* rmout, u16 hor, u16 ver);
void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht);
void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht);
void GXSetDispCopyDst(u16 wd, u16 ht);
void GXSetTexCopyDst(u16 wd, u16 ht, s32 fmt, u8 mipmap);
void GXSetDispCopyFrame2Field(s32 mode);
void GXSetCopyClamp(s32 clamp);
f32 GXGetYScaleFactor(u16 efbHeight, u16 xfbHeight);
u32 GXSetDispCopyYScale(f32 vscale);
void GXSetCopyClear(GXColor clear_clr, u32 clear_z);
void GXSetCopyFilter(u8 aa, const u8 sample_pattern[12][2], u8 vf, const u8 vfilter[7]);
void GXSetDispCopyGamma(s32 gamma);
/* untyped: byte range - the destination frame buffer */
void GXCopyDisp(void* dest, u8 clear);
/* untyped: byte range - the destination texture buffer */
void GXCopyTex(void* dest, u8 clear);
void GXClearBoundingBox(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXFRAMEBUF_H */
