/*
 * RVLGX/GXTexture.h - declarations of the symbols owned by `RVLGX/GXTexture.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXTEXTURE_H
#define MHTRI_RVLGX_GXTEXTURE_H

#include "types.h"
#include "gx.h"

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
/* 0x804B8440 - returns the texture object's width. */
u16 GXGetTexObjWidth(const GXTexObj* obj);
/* 0x804B8460 - returns the texture object's height. */
u16 GXGetTexObjHeight(const GXTexObj* obj);
/* 0x804B8720 - loads a texture object into texture slot `id`. */
void GXLoadTexObj(const GXTexObj* obj, u32 id);
/* untyped: byte range - the lookup table's entries */
void GXInitTlutObj(GXTlutObj* obj, void* lut, u32 format, u16 entries);
void GXLoadTlut(const GXTlutObj* obj, u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTEXTURE_H */
