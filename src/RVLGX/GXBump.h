/*
 * RVLGX/GXBump.h - declarations of the symbols owned by `RVLGX/GXBump.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXBUMP_H
#define MHTRI_RVLGX_GXBUMP_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetTevIndirect(u32 tev_stage, u32 ind_stage, u32 format, u32 bias_sel, u32 matrix_sel, u32 wrap_s,
                      u32 wrap_t, u8 add_prev, u8 ind_lod, u32 alpha_sel);
void GXSetIndTexMtx(u32 mtx_id, const f32 offset[2][3], s8 scale_exp);
void GXSetIndTexCoordScale(u32 ind_stage, u32 scale_s, u32 scale_t);
void GXSetIndTexOrder(u32 ind_stage, u32 tex_coord, u32 tex_map);
void GXSetNumIndStages(u8 nIndStages);
void GXSetTevDirect(u32 tev_stage);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXBUMP_H */
