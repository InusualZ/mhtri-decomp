/*
 * RVLGX/GXLight.h - declarations of the symbols owned by `RVLGX/GXLight.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXLIGHT_H
#define MHTRI_RVLGX_GXLIGHT_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXInitLightAttn(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2);
void GXInitLightAttnA(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2);
void GXInitLightAttnK(GXLightObj* lt_obj, f32 k0, f32 k1, f32 k2);
void GXInitLightSpot(GXLightObj* lt_obj, f32 cutoff, s32 spot_func);
void GXInitLightDistAttn(GXLightObj* lt_obj, f32 ref_dist, f32 ref_br, s32 dist_func);
void GXInitLightPos(GXLightObj* lt_obj, f32 x, f32 y, f32 z);
void GXGetLightPos(const GXLightObj* lt_obj, f32* x, f32* y, f32* z);
void GXInitLightDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz);
void GXGetLightDir(const GXLightObj* lt_obj, f32* nx, f32* ny, f32* nz);
void GXInitSpecularDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz);
void GXInitLightColor(GXLightObj* lt_obj, GXColor color);
void GXGetLightColor(const GXLightObj* lt_obj, GXColor* color);
/* 0x804B7C40 (0x80): loads the light object `lt_obj` into the hardware light slot selected by the `light` mask bit. */
void GXLoadLightObjImm(const GXLightObj* lt_obj, u32 light);
void GXSetChanAmbColor(s32 chan, GXColor color);
void GXSetChanMatColor(s32 chan, GXColor color);
void GXSetNumChans(u8 nChans);
void GXSetChanCtrl(s32 chan, u8 enable, s32 amb_src, s32 mat_src, u32 light_mask, s32 diff_fn, s32 attn_fn);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXLIGHT_H */
