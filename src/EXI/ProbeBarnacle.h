/*
 * Declarations for the symbols `src/EXI/ProbeBarnacle.c` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_EXI_PROBEBARNACLE_H
#define MHTRI_EXI_PROBEBARNACLE_H

#include "types.h"
#include "gx.h" /* GXLightObj */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B5AE0 - points GX vertex attribute `attr` at an array of `stride`-byte entries. */
void GXSetArray(u32 attr, const void* pBase, u8 stride); /* untyped: byte range */

/* 0x804B6870 - opens a primitive of `nverts` vertices of type `type` in vertex format `vtxfmt`. */
void GXBegin(u32 type, u32 vtxfmt, u16 nverts);

/* The vertex-format and lighting-channel setters the nw4r character writer drives. */
void GXSetVtxDesc(u32 attr, u32 type);
void GXClearVtxDesc(void);
void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
void GXSetNumChans(u8 nChans);
void GXSetChanCtrl(u32 chan, u8 enable, u32 ambSrc, u32 matSrc, u32 lightMask, u32 diffFn, u32 attnFn);
void GXSetNumTexGens(u8 nTexGens);

/* 0x804B7C40 - loads the light object `lt_obj` into the hardware light slot selected by the `light` mask bit. */
void GXLoadLightObjImm(const GXLightObj* lt_obj, u32 light);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EXI_PROBEBARNACLE_H */
