/*
 * g3d/g3d_dcc.h - declarations of the symbols owned by `g3d/g3d_dcc.cpp` that other units call.
 */
#ifndef G3D_G3D_DCC_H
#define G3D_G3D_DCC_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {
struct MTX34;
}
namespace g3d {
struct TexSrt;
}
}

/* Builds the texture matrix of one SRT record into `pMtx` (GUESS name: the DCC sibling of g3d_calc_tex_mtx_xsi). */
/* 0x80075E9C - builds the texture matrix of one SRT record in the given texture-matrix mode. */
extern "C" void g3d_calc_tex_mtx(nw4r::math::MTX34* pMtx, BOOL bSet, const nw4r::g3d::TexSrt* pSrt, u32 flag,
                                 int mode);
extern "C" void g3d_calc_tex_mtx_dcc(nw4r::math::MTX34* pMtx, BOOL bSet, const nw4r::g3d::TexSrt* pSrt, u32 flag);
#endif

#endif
