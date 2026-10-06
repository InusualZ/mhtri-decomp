/* g3d/g3d_state.h - the texture/state helpers of `g3d/g3d_state.cpp` that `g3d/g3d_resmat.cpp` resolves through,
 *   and the resource-range store `g3d/g3d_resfile.cpp` calls (C linkage). */
#ifndef MHTRI_G3D_G3D_STATE_H
#define MHTRI_G3D_G3D_STATE_H

#include "types.h"
#include "nw4r/math.h"

/* The render-mode record `fn_80088584` returns; only used through a pointer. */
struct RenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80086390 - the `ResTexSrt` handle's non-const resource resolver (returns the slot array). */
u8* fn_80086390(void* pSelf);

/* 0x80087870 - the const twin of fn_80086390 (used by `ResTexSrt::GetEffectMtx`). */
u8* fn_80087870(void* pSelf);

/* 0x80088584 - the render-mode helper (caller: g3d_camera.cpp). */
struct RenderModeObj* fn_80088584(void);

/* 0x800868A0 - the pipe-command writer (callers: gx/fn_8009AA78.c, gx/fn_8009ACE4.c). */
void fn_800868A0(u32 value);

/* 0x8079124C - the `.sdata` word the unit hands back the address of. */
extern u32 lbl_8079124C;

#ifdef __cplusplus
}
#endif

/* More of the unit's cross-unit declarations (rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_inverse(Mtx34* out, const Mtx34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
