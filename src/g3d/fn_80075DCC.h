/* g3d/fn_80075DCC.h - the cross-unit declarations of `g3d/fn_80075DCC.cpp` (C linkage, plain map stems). */
#ifndef MHTRI_G3D_FN_80075DCC_H
#define MHTRI_G3D_FN_80075DCC_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056F668 - the "G3dObj" type-name record (`.rodata`: a length word, then the NUL-terminated name) `g3d/g3d_anmchr.cpp`
 * reads. */
extern u8 anm_typename_G3dObj[];

/* 0x80075DCC - a one-line thunk: `math_sincos_idx(lbl_80795DFC * angle)`.  The callers pass a sin/cos
 * out-pair plus an angle; the shipped body reads only the angle. */
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle);
/* 0x80075DD8 - sets up the camera projection for `p`. */
void fn_80075DD8(void* p);

/* 0x80077420 - writes one XF register through the pipe (callers: gx/fn_8009AA78.c, g3d/g3d_state.cpp). */
void GDWriteXFCmd(u16 addr, u32 value);
/* 0x8007740C - writes a colour channel's material colour (caller: g3d/g3d_state.cpp). */
void g3d_gd_set_chan_mat_color(u32 chan, GXColor color);

/* 0x80077DF0 - assembles an MTX34 from twelve floats (the first eight in FPRs, the last four on the
 * stack).  Callers: ef/ef_drawfreestrategy.cpp, ef/ef_drawstrategyimpl.cpp. */
void mtx34_set(Mtx34* dst, f32 m00, f32 m01, f32 m02, f32 m03,
                 f32 m10, f32 m11, f32 m12, f32 m13,
                 f32 m20, f32 m21, f32 m22, f32 m23);

/* 0x8007A5E4/0x8007A5A8/0x8007A724 - the 3-float setters (caller: g3d/fn_80063888.cpp).  The object
 * arrives in r3 and the three values in f1-f3. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z);
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z);
void fn_8007A724(void* self, f32 x, f32 y, f32 z);

/* 0x8007BB8C/0x8007B72C - the type-name store copies the ScnLeaf and ScnGroup run-time type members call (and
 * `g3d/g3d_state.cpp`'s ScnRoot ones): each stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v);
const u8** type_obj_set_name_scngroup(const u8** out, const u8* v);

/* The ScnMdl/ScnMdlSimple material and draw-buffer helpers `g3d/g3d_scnmdl.cpp` calls. */
u32 fn_8007C464(void* pSelf);
s32 fn_80077E34(s32 pOut, void* pIn);  /* 0x80077E34 - builds the model view the node walks read */

s32 fn_80078904(s32 pNode);            /* 0x80078904 - the node's visibility test */

/* The `g3d_resmat_ac.h` validity predicates `g3d/g3d_resfile.cpp`'s accessors call. */
/* 0x8007B878 - the alignment-asserting offset helper, declared as the owner defines it; `g3d/g3d_resanmtexsrt.cpp`
 * casts at its two call sites. */
u32 fn_8007B878(s32 pDst, s32 offset);

/* 0x8007A510 - the softreset/return-to-title request (callers: src/mh3_pad.cpp, src/pad_connect.cpp). */
void fn_8007A510(void);

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

/* 0x8007B870 - the placement `operator new` (`mr r3,r4; blr`: hands the caller's address back); the network
 * work record builds its friend list in place with it. */
/* untyped: opaque handle passed through - the placement address the caller hands in */
void* operator new(unsigned long size, void* place);
#endif

#ifdef __cplusplus
namespace nw4r {
namespace math {

/* 0x800774A0 - `x` raised to the power `y` (a tail call into the C library). */
f32 FPow(f32 x, f32 y);

}  // namespace math
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_FN_80075DCC_H */

