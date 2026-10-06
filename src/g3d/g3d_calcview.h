/* g3d/g3d_calcview.h - the `ResMdl` handle validity accessors `g3d/g3d_calcview.cpp` owns (C linkage). */
#ifndef MHTRI_G3D_G3D_CALCVIEW_H
#define MHTRI_G3D_G3D_CALCVIEW_H

#include "types.h"
#include "nw4r/math.h"

/* The `g3d_calcworld` work record `fn_8006FF50` returns; only used through a pointer. */
struct G3DWorkObj;

#ifdef __cplusplus
extern "C" {
#endif

const char* fn_8006FFBC(void);      /* 0x8006FFBC - the `ResMdl` type name the assert prints */
u32 fn_8006FFC8(void* pSelf);       /* 0x8006FFC8 - the `ResMdl` handle validity test */

/* The checked resource resolver `g3d/g3d_resfile.cpp` calls. */
u32 fn_800700C0(void* p); /* 0x800700C0 - the checked resource resolver */

/* 0x800710BC - `out = a * b` for two 3x4 matrices. */
void fn_800710BC(Mtx34* out, const Mtx34* a, const Mtx34* b);

/* 0x8006FDCC..0x8007100C - the `g3d_calcworld` node/resource helpers (callers: g3d_calcworld.cpp and the
 * matrix users).  `fn_8006FDCC`'s callers use the word as a matrix id or an array index. */
u32 fn_8006FDCC(const void* p);
struct G3DWorkObj* fn_8006FF50(void);
s32* fn_80070054(void* pOut, const void* pKey);
void* fn_8007012C(void);
void fn_8007100C(void* pDst, const void* pSrc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVIEW_H */
