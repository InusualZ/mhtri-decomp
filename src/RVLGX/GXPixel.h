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
/* The ten fog range-adjust entries, as five (low, high) pairs. size: 0x14 */
typedef struct GXFogAdjTable {
    /* +0x00 */ u16 r[10];
} GXFogAdjTable;

void GXSetFogRangeAdj(u8 enable, u16 center, const GXFogAdjTable* table);
void GXSetFieldMask(u32 odd_mask, u32 even_mask);
void GXSetBlendMode(u32 type, u32 src_factor, u32 dst_factor, u32 op);
void GXSetZMode(u8 compare_enable, u32 func, u8 update_enable);
void GXCallDisplayList(const void* list, u32 size); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXPIXEL_H */
