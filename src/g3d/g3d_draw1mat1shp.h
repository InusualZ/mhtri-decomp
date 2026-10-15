/* g3d/g3d_draw1mat1shp.h - the declarations `g3d/g3d_draw1mat1shp.cpp` owns (C linkage keeps the map's stems; FPow is
 *   nw4r::math's). */
#ifndef MHTRI_G3D_G3D_DRAW1MAT1SHP_H
#define MHTRI_G3D_G3D_DRAW1MAT1SHP_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80077420 - writes one XF register through the pipe (callers: gx/fn_8009AA78.c, g3d/g3d_state.cpp). */
void GDWriteXFCmd(u16 addr, u32 value);
/* 0x8007740C - writes a colour channel's material colour (caller: g3d/g3d_state.cpp). */
void g3d_gd_set_chan_mat_color(u32 chan, GXColor color);
/* 0x80077590 (0x30): copy-constructs the material's misc block handle (`g3d/g3d_draw.cpp`'s fn_800777B0 calls it). */
s32 res_mat_misc_copy_ctor(s32 pDst, const s32* pSrc);

#ifdef __cplusplus
}

namespace nw4r {
namespace math {

/* 0x800774A0 - `x` raised to the power `y` (a tail call into the C library). */
f32 FPow(f32 x, f32 y);

}  // namespace math
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_DRAW1MAT1SHP_H */
