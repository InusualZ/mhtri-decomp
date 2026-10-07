/*
 * RVLGX/GXTev.h - declarations of the symbols owned by `RVLGX/GXTev.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXTEV_H
#define MHTRI_RVLGX_GXTEV_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetTevOp(u32 stage, u32 mode);
void GXSetTevColorIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevAlphaIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevColorOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg);
void GXSetTevAlphaOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg);
void GXSetTevColor(u32 reg, GXColor color);
void GXSetTevKColor(u32 id, GXColor color);
void GXSetTevKColorSel(u32 stage, u32 sel);
void GXSetTevKAlphaSel(u32 stage, u32 sel);
void GXSetTevSwapMode(u32 stage, u32 ras_sel, u32 tex_sel);
void GXSetTevSwapModeTable(u32 table, u32 red, u32 green, u32 blue, u32 alpha);
void GXSetAlphaCompare(u32 comp0, u8 ref0, u32 op, u32 comp1, u8 ref1);
void GXSetZTexture(u32 op, u32 fmt, u32 bias);
void GXSetTevOrder(u32 stage, u32 coord, u32 map, u32 color);
void GXSetNumTevStages(u8 nStages);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTEV_H */
