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
void GXInitTexObjWrapMode(GXTexObj* obj, u32 wrap_s, u32 wrap_t);
void GXInitTexObjTlut(GXTexObj* obj, u32 tlut_name);
/* untyped: caller-owned payload - the user pointer a texture object carries */
void GXInitTexObjUserData(GXTexObj* obj, void* user_data);
/* untyped: caller-owned payload - the user pointer a texture object carries */
void* GXGetTexObjUserData(const GXTexObj* obj);
/* untyped: byte range - the texel image address the texture object holds */
void GXGetTexObjAll(const GXTexObj* obj, void** image_ptr, u16* width, u16* height, u32* format, u32* wrap_s,
                    u32* wrap_t, u8* mipmap);
/* 0x804B8440 - returns the texture object's width. */
u16 GXGetTexObjWidth(const GXTexObj* obj);
/* 0x804B8460 - returns the texture object's height. */
u16 GXGetTexObjHeight(const GXTexObj* obj);
s32 GXGetTexObjFmt(const GXTexObj* obj);
u32 GXGetTexObjWrapS(const GXTexObj* obj);
u32 GXGetTexObjWrapT(const GXTexObj* obj);
u8 GXGetTexObjMipMap(const GXTexObj* obj);
void GXGetTexObjLODAll(const GXTexObj* obj, u32* min_filt, u32* mag_filt, f32* min_lod, f32* max_lod,
                       f32* lod_bias, u8* bias_clamp, u8* do_edge_lod, u32* max_aniso);
u32 GXGetTexObjTlut(const GXTexObj* obj);
void GXLoadTexObjPreLoaded(const GXTexObj* obj, const GXTexRegion* region, u32 id);
/* 0x804B8720 - loads a texture object into texture slot `id`. */
void GXLoadTexObj(const GXTexObj* obj, u32 id);
/* untyped: byte range - the lookup table's entries */
void GXInitTlutObj(GXTlutObj* obj, void* lut, u32 format, u16 entries);
void GXLoadTlut(const GXTlutObj* obj, u32 id);
void GXInitTexCacheRegion(GXTexRegion* region, u8 is_32b_mipmap, u32 tmem_even, s32 size_even, u32 tmem_odd,
                          s32 size_odd);
void GXInitTlutRegion(GXTlutRegion* region, u32 tmem_addr, u32 tlut_size);
void GXInvalidateTexAll(void);
GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback f);
GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback f);
/* 0x804B7F60 - computes the row, column and total tile counts of a `wd` x `ht` image in format `fmt`. */
void __GetImageTileCount(s32 fmt, u16 wd, u16 ht, u32* rowTiles, u32* colTiles, u32* cmpTiles);
/* 0x804B8A40 (0x164): sends the setup-unit texture-coordinate scale registers of the active TEV stages. */
void __GXSetSUTexRegs(void);
void __GXSetTmemConfig(u32 config);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXTEXTURE_H */
