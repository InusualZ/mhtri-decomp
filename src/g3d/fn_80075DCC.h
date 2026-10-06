/* g3d/fn_80075DCC.h - the cross-unit declarations of `g3d/fn_80075DCC.cpp` (C linkage, plain map stems). */
#ifndef MHTRI_G3D_FN_80075DCC_H
#define MHTRI_G3D_FN_80075DCC_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* 0x8007B5F4/0x8007BB8C - the ScnRoot state lookups `g3d/g3d_state.cpp` calls: fn_8007BB8C stores what it finds
 * through its out-parameter and returns it; fn_8007B5F4 registers `pKey` under the state object. */
u32 fn_8007B5F4(void* pSelf, const u32* pKey);
void** fn_8007BB8C(void** pOut, const char* pName);

/* The ScnMdl/ScnMdlSimple material and draw-buffer helpers `g3d/g3d_scnmdl.cpp` calls. */
s32 fn_8007B424(void* pSelf);  /* 0x8007B424 - the material count */
s32 fn_8007B734(void* pSelf);  /* 0x8007B734 - a name-record reader */
s32 fn_8007B764(void* pSelf);  /* 0x8007B764 - a name-record reader */
s32 fn_8007BAF0(void* pSelf, u32* pKey); /* 0x8007BAF0 - the chain's insertion step */
void fn_8007B564(void* pSelf, u32 mask, void* pArg2, void* pArg3);
void fn_8007B8E4(void* pSelf, u32 mask, void* pArg2, void* pArg3);
void fn_8007B940(void* pSelf, u32 mask, void* pArg2, void* pArg3);
u32 fn_8007C464(void* pSelf);
s32 fn_80077E34(s32 pOut, void* pIn);  /* 0x80077E34 - builds the model view the node walks read */

s32 fn_80078904(s32 pNode);            /* 0x80078904 - the node's visibility test */
void fn_800793A4(s32* pArg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7,
                 u32 argSp0);          /* 0x800793A4 - the draw-buffer builder */

/* The `g3d_resmat_ac.h` validity predicates `g3d/g3d_resfile.cpp`'s accessors call. */
/* 0x8007B878 - the alignment-asserting offset helper, declared as the owner defines it; `g3d/g3d_resanmtexsrt.cpp`
 * casts at its two call sites. */
u32 fn_8007B878(s32 pDst, s32 offset);

/* 0x8007A510 - the softreset/return-to-title request (callers: src/mh3_pad.cpp, src/pad_connect.cpp). */
void fn_8007A510(void);

#ifdef __cplusplus
}

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

