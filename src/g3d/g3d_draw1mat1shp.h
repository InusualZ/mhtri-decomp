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

#ifdef __cplusplus
}

namespace nw4r {
namespace math {

/* 0x800774A0 - `x` raised to the power `y` (a tail call into the C library). */
f32 FPow(f32 x, f32 y);

}  // namespace math
}  // namespace nw4r

#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_resvtx.h"

class G3dIndMtxCallback;

namespace nw4r {
namespace g3d {

/* The material handles a replacement block swaps in for one draw (`g3d/g3d_draw.cpp` builds it, 0x80078DC0, and
 * refills it per material, 0x80077E70): one handle per resource kind, then the three vertex-table arrays the
 * replacement carries.  size: 0x38 */
struct Draw1Mat1ShpSwap {
    /* +0x00 */ ResTexObj texObj;
    /* +0x04 */ ResTlutObj tlutObj;
    /* +0x08 */ ResGenMode genMode;
    /* +0x0C */ ResTev tev;
    /* +0x10 */ ResMatPix pix;
    /* +0x14 */ ResMatTevColor tevColor;
    /* +0x18 */ ResMatIndMtxAndScale indMtxAndScale;
    /* +0x1C */ ResMatChan chan;
    /* +0x20 */ ResMatTexCoordGen texCoordGen;
    /* +0x24 */ ResMatMisc misc;
    /* +0x28 */ ResTexSrt texSrt;
    /* +0x2C */ ResVtxPosData** vtxPosTable;   /* per vertex-position id; NULL = the model's own */
    /* +0x30 */ ResVtxNrmData** vtxNrmTable;
    /* +0x34 */ ResVtxClrData** vtxClrTable;

    Draw1Mat1ShpSwap();
};

/* 0x800769F4 - draws one shape with one material: loads the material state (or the swap's replacements), the
 * matrices and the indirect matrices, then the shape's display lists. */
void Draw1Mat1ShpDirectly(ResMat mat, ResShp shp, const math::MTX34* pViewPos, const math::MTX34* pViewNrm, u32 ctrl,
                          Draw1Mat1ShpSwap* pSwap, G3dIndMtxCallback* pIndMtxOp);

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_DRAW1MAT1SHP_H */
