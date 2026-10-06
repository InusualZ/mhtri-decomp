/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the Revolution SDK GX entry points whose map addresses fall in
 * `RVLGX/GXTexture_tail.cpp`'s registered range (0x804B8020-0x804C1760: `GXTexture.c`, `GXBump.c`, `GXTev.c`,
 * `GXPixel.c`, `GXTransform.c`), so that unit is their owner here.  The prototypes are the SDK's, with each
 * enumeration spelled as its 32-bit integer and each `GXBool` as `u8`; the texture records are
 * `gx.h`'s.
 */
#ifndef MHTRI_RVLGX_GXSETTEVORDER_H
#define MHTRI_RVLGX_GXSETTEVORDER_H

#include "types.h"
#include "gx.h"
#include "RVLGX/GXSetZCompLoc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: byte range - the texel image the texture object points the GPU at */
void GXInitTexObj(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrap_s, u32 wrap_t,
                  u8 mipmap);
/* untyped: byte range - the colour-index image the texture object points the GPU at */
void GXInitTexObjCI(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrap_s, u32 wrap_t,
                    u8 mipmap, u32 tlut_name);
void GXInitTexObjLOD(GXTexObj* obj, u32 min_filt, u32 mag_filt, f32 min_lod, f32 max_lod, f32 lod_bias,
                     u8 bias_clamp, u8 do_edge_lod, u32 max_aniso);
void GXLoadTexObj(const GXTexObj* obj, u32 id);
/* untyped: byte range - the lookup table's entries */
void GXInitTlutObj(GXTlutObj* obj, void* lut, u32 format, u16 entries);
void GXLoadTlut(const GXTlutObj* obj, u32 id);
void GXSetTevIndirect(u32 tev_stage, u32 ind_stage, u32 format, u32 bias_sel, u32 matrix_sel, u32 wrap_s,
                      u32 wrap_t, u8 add_prev, u8 ind_lod, u32 alpha_sel);
void GXSetIndTexMtx(u32 mtx_id, const f32 offset[2][3], s8 scale_exp);
void GXSetIndTexCoordScale(u32 ind_stage, u32 scale_s, u32 scale_t);
void GXSetIndTexOrder(u32 ind_stage, u32 tex_coord, u32 tex_map);
void GXSetNumIndStages(u8 count);
void GXSetTevDirect(u32 tev_stage);
void GXSetTevColorIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevAlphaIn(u32 stage, u32 a, u32 b, u32 c, u32 d);
void GXSetTevColorOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg);
void GXSetTevAlphaOp(u32 stage, u32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg);
void GXSetTevColor(u32 id, GXColor color);
void GXSetTevKColor(u32 id, GXColor color);
void GXSetTevKColorSel(u32 stage, u32 sel);
void GXSetTevKAlphaSel(u32 stage, u32 sel);
void GXSetTevSwapMode(u32 stage, u32 ras_sel, u32 tex_sel);
void GXSetTevSwapModeTable(u32 table, u32 red, u32 green, u32 blue, u32 alpha);
void GXSetAlphaCompare(u32 comp0, u8 ref0, u32 op, u32 comp1, u8 ref1);
void GXSetTevOrder(u32 stage, u32 coord, u32 map, u32 color);
void GXSetNumTevStages(u8 count);
void GXSetFog(u32 type, f32 start_z, f32 end_z, f32 near_z, f32 far_z, GXColor color);
void GXSetBlendMode(u32 type, u32 src_factor, u32 dst_factor, u32 op);
void GXSetZMode(u8 compare_enable, u32 func, u8 update_enable);
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, u32 type);
void GXLoadPosMtxImm(const f32 mtx[][4], u32 id);
void GXSetCurrentMtx(u32 id);
void GXSetClipMode(u32 mode);
void GXGetProjectionv(f32* proj);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXSETTEVORDER_H */
