/*
 * The `g3d/fn_80075DCC.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a
 * registered unit owns is declared once, in that owner's header, and every consumer includes it; this
 * is that header for the nw4r g3d render/dispatch cluster registered from proposal `80075DCC`
 * (`.text` 0x80075DCC-0x8007C540).
 *
 * The seven plain-`fn_XXXXXXXX` symbols below used to sit in `include/unsplit/g3d.h`, the fallback
 * band for a g3d-module symbol with no registered owner.  Registering the cluster makes them owned, so
 * the declarations move here and the consumers (`g3d/g3d_basic.cpp`, `g3d/g3d_camera.cpp`,
 * `gx/fn_8009AA78.c`, `ef/ef_drawfreestrategy.cpp`, `ef/ef_drawstrategyimpl.cpp`,
 * `g3d/fn_80063888.cpp`) include this header instead.  The transfer itself is recorded in the outbox
 * as a `shared-file` request, because it edits files (`include/unsplit/g3d.h` and the consumers) that
 * the batch applies together.
 *
 * All of them carry the map's own `fn_XXXXXXXX` stem, so they keep C linkage.
 */
#ifndef MHTRI_G3D_FN_80075DCC_H
#define MHTRI_G3D_FN_80075DCC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80075DCC - a one-line thunk: `fn_80500EF4(lbl_80795DFC * angle)`.  The callers pass a sin/cos
 * out-pair plus an angle; the shipped body reads only the angle. */
void fn_80075DCC(f32* pOutSin, f32* pOutCos, f32 angle);
/* 0x80075DD8 - sets up the camera projection for `p`. */
void fn_80075DD8(void* p);

/* 0x80077420 - the GX pipe-command writer (caller: gx/fn_8009AA78.c). */
void fn_80077420(u16 command, u8 value);

/* 0x80077DF0 - assembles an MTX34 from twelve floats (the first eight in FPRs, the last four on the
 * stack).  Callers: ef/ef_drawfreestrategy.cpp, ef/ef_drawstrategyimpl.cpp. */
void fn_80077DF0(Mtx34* dst, f32 m00, f32 m01, f32 m02, f32 m03,
                 f32 m10, f32 m11, f32 m12, f32 m13,
                 f32 m20, f32 m21, f32 m22, f32 m23);

/* 0x8007A5E4/0x8007A5A8/0x8007A724 - the 3-float setters (caller: g3d/fn_80063888.cpp).  The object
 * arrives in r3 and the three values in f1-f3. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z);
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z);
void fn_8007A724(void* self, f32 x, f32 y, f32 z);

/* 0x8007B5F4/0x8007BB8C - the ScnRoot state lookups `g3d/g3d_state.cpp` calls (rule 2, moved out of
 * include/unsplit/g3d.h when this unit registered, 2026-09-25).  `fn_8007BB8C` stores what it finds
 * through its out-parameter and returns that parameter; `fn_8007B5F4` registers `pKey` under the
 * state object. */
u32 fn_8007B5F4(void* pSelf, const u32* pKey);
void** fn_8007BB8C(void** pOut, const char* pName);

/* The ScnMdl/ScnMdlSimple material and draw-buffer helpers the ScnMdl unit
 * (g3d/g3d_scnmdl.cpp) calls.  Declared here, in the owner's header, once this unit is the owner
 * (rule 2); the consumer includes this header instead of re-declaring them.  The target object
 * references the plain `fn_XXXXXXXX` names, so they sit inside this `extern "C"` block. */
s32 fn_8007B424(void* pSelf);  /* 0x8007B424 - the material count */
s32 fn_8007B734(void* pSelf);  /* 0x8007B734 - a name-record reader */
s32 fn_8007B764(void* pSelf);  /* 0x8007B764 - a name-record reader */
s32 fn_8007BAF0(void* pSelf, u32* pKey); /* 0x8007BAF0 - the chain's insertion step */
void fn_8007B564(void* pSelf, u32 mask, void* pArg2, void* pArg3);
void fn_8007B8E4(void* pSelf, u32 mask, void* pArg2, void* pArg3);
void fn_8007B940(void* pSelf, u32 mask, void* pArg2, void* pArg3);
u32 fn_8007C464(void* pSelf);
s32 fn_80077E34(s32 pOut, void* pIn);  /* 0x80077E34 - builds the model view the node walks read */

/* 0x800768C8/0x800768DC/0x800768F0 - the three `ResMat`/`ResTex`-style handle validity tests the
 * `g3d/g3d_resmat.cpp` accessors assert through (callers: g3d_resmat.cpp).  Each reads the handle's
 * word and returns whether it is non-null; the three differ only by the type of handle they name. */
u32 fn_800768C8(void* pSelf);
u32 fn_800768DC(void* pSelf);
u32 fn_800768F0(void* pSelf);
/* 0x80076974/0x80076988 - the `ResTex`-style handle validators the `g3d_resmat` texture helpers use. */
u32 fn_80076974(void* pSelf);
u32 fn_80076988(void* pSelf);
/* 0x80077638/0x800776F4 - the `ResTlut`-style validators. */
u32 fn_80077638(void* pSelf);
u32 fn_800776F4(void* pSelf);
/* 0x800774A0/0x80077744/0x800783EC - the resolved-resource readers of the same family. */
u8* fn_800774A0(void* pSelf);
u8* fn_80077744(void* pSelf);
u8* fn_800783EC(void* pSelf);
s32 fn_80078904(s32 pNode);            /* 0x80078904 - the node's visibility test */
void fn_800793A4(s32* pArg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7,
                 u32 argSp0);          /* 0x800793A4 - the draw-buffer builder */

/* Added when `g3d/g3d_resfile.cpp` registered (rule 2): the `g3d_resmat_ac.h` handle constructors and
 * validity predicates the accessor cluster calls.  Inside this `extern "C"` block with the rest of the
 * owner's declarations (the target object references the plain `fn_XXXXXXXX` names). */
u32* fn_800766D0(u32* pDst, u32 value); /* 0x800766D0 - the 0x20-aligned handle constructor */
u32* fn_80076794(u32* pDst, u32 value); /* 0x80076794 - the 0x20-aligned handle constructor */
s32 fn_8007673C(void* p);              /* 0x8007673C - `*(u32*)p != 0` */
s32 fn_80076750(void* p);              /* 0x80076750 - `*(u32*)p != 0` */
s32 fn_80076800(void* p);              /* 0x80076800 - `*(u32*)p != 0` */
u32 fn_8007B878(s32 pDst, s32 offset); /* 0x8007B878 - the alignment-asserting offset helper */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_80075DCC_H */
