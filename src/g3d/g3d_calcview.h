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

const char* fn_8006FFBC(void);      /* 0x8006FFBC - the `ResMdlInfo` type name the assert prints */
u32 fn_8006FFC8(void* pSelf);       /* 0x8006FFC8 - the `ResMdlInfo` handle validity test */

/* The checked resource resolver `g3d/g3d_resfile.cpp` calls. */

/* 0x800710BC - `out = a * b` for two 3x4 matrices. */
void mtx34_concat(Mtx34* out, const Mtx34* a, const Mtx34* b);

/* 0x8006FDCC..0x8007100C - the `g3d_calcworld` node/resource helpers (callers: g3d_calcworld.cpp and the
 * matrix users).  `fn_8006FDCC`'s callers use the word as a matrix id or an array index. */
u32 fn_8006FDCC(const void* p);
struct G3DWorkObj* fn_8006FF50(void);
s32* fn_80070054(void* pOut, const void* pKey);
void mtx34_copy_ps(Mtx34* pDst, const Mtx34* pSrc);   /* 0x8007100C - paired-single copy of a 3x4 matrix */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVIEW_H */
