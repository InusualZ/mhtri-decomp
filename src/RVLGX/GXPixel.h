/*
 * RVLGX/GXPixel.h - declarations of the symbols owned by `RVLGX/GXPixel.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXPIXEL_H
#define MHTRI_RVLGX_GXPIXEL_H

#include "types.h"
#include "gx.h"
#include "RVLGX/GXSetZCompLoc.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetFog(u32 type, f32 start_z, f32 end_z, f32 near_z, f32 far_z, GXColor color);
void GXSetBlendMode(u32 type, u32 src_factor, u32 dst_factor, u32 op);
void GXSetZMode(u8 compare_enable, u32 func, u8 update_enable);
void GXCallDisplayList(const void* list, u32 size); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXPIXEL_H */
