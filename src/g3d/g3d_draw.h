/* g3d/g3d_draw.h - the cross-unit declarations of `g3d/g3d_draw.cpp` (C linkage, plain map stems). */
#ifndef MHTRI_G3D_G3D_DRAW_H
#define MHTRI_G3D_G3D_DRAW_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80077DF0 - assembles an MTX34 from twelve floats (the first eight in FPRs, the last four on the
 * stack).  Callers: ef/ef_drawfreestrategy.cpp, ef/ef_drawstrategyimpl.cpp. */
void mtx34_set(Mtx34* dst, f32 m00, f32 m01, f32 m02, f32 m03,
                 f32 m10, f32 m11, f32 m12, f32 m13,
                 f32 m20, f32 m21, f32 m22, f32 m23);

/* The ScnMdl/ScnMdlSimple material and draw-buffer helpers `g3d/g3d_scnmdl.cpp` calls. */
s32 fn_80077E34(s32 pOut, void* pIn);  /* 0x80077E34 - builds the model view the node walks read */

s32 fn_80078904(s32 pNode);            /* 0x80078904 - the node's visibility test */



#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ResMdl; } } /* only pointed to here: the full class is g3d/g3d_resmat.h's */

/* 0x800793A4 - draws the model directly: the opaque or translucent byte code over the view matrices, with an
 * optional replacement block, in a draw mode. */
/* untyped: caller-owned payload - the replacement block */
extern "C" void g3d_draw_res_mdl_directly(const nw4r::g3d::ResMdl* pMdl, const nw4r::math::MTX34* pViewPosMtxArray,
                                          const nw4r::math::MTX33* pViewNrmMtxArray,
                                          const nw4r::math::MTX34* pViewTexMtxArray, const u8* pByteCodeOpa,
                                          const u8* pByteCodeXlu, const void* pReplacement, u32 drawMode);

#endif

#endif /* MHTRI_G3D_G3D_DRAW_H */

