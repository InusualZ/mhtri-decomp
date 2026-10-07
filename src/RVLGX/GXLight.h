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

/* 0x804B7C40 - loads the light object `lt_obj` into the hardware light slot selected by the `light` mask bit. */
void GXLoadLightObjImm(const GXLightObj* lt_obj, u32 light);
void GXSetChanAmbColor(u32 chan, GXColor color);
void GXSetChanMatColor(u32 chan, GXColor color);
void GXSetNumChans(u8 nChans);
void GXSetChanCtrl(u32 chan, u8 enable, u32 amb_src, u32 mat_src, u32 light_mask, u32 diff_fn, u32 attn_fn);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXLIGHT_H */
