/* g3d/g3d_resvtx.h - the cross-unit declarations of `g3d/g3d_resvtx.cpp` (C linkage). */
#ifndef MHTRI_G3D_G3D_RESVTX_H
#define MHTRI_G3D_G3D_RESVTX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80088E84..0x8008918C - the three `GetBaseVtx` shapes the 0x8007270C blend starts from (callers:
 * g3d_calcvtx.cpp, g3d_resshp.cpp).  Each fills the base-vertex pointer and the vertex stride for one resource. */
void fn_80088E84(const void* p, const void** ppBaseVtx, u8* pStride);
void fn_80088FFC(const void* p, const void** ppBaseVtx, u8* pStride);
void fn_8008918C(const void* p, const void** ppBaseVtx, u8* pStride);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESVTX_H */
