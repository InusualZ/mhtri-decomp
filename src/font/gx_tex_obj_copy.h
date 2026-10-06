/* font/gx_tex_obj_copy.h - the leaf declaration of `gx_tex_obj_copy`, which `font/flfnt.cpp` defines (its callers are
 *   flfntFlush and g3d/g3d_state.cpp's texture-object cache).  The owner spells the 0x20-byte block as its own
 *   `MtxBlock8`; the callers copy a `GXTexObj`. */
#ifndef MHTRI_FONT_GX_TEX_OBJ_COPY_H
#define MHTRI_FONT_GX_TEX_OBJ_COPY_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005C50C - copies the 0x20-byte texture object `src` into `dst`. */
void gx_tex_obj_copy(GXTexObj* dst, const GXTexObj* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FONT_GX_TEX_OBJ_COPY_H */
