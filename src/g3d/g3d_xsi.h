/* g3d/g3d_xsi.h - the cross-unit declarations of `g3d/g3d_xsi.cpp` (C linkage). */
#ifndef MHTRI_G3D_G3D_XSI_H
#define MHTRI_G3D_G3D_XSI_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
struct TexSrt;
}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* 0x800D74E8 - builds the XSI texture matrix of `pSrt` into `pMtx` (caller: g3d/fn_80075DCC.cpp). */
BOOL g3d_calc_tex_mtx_xsi(nw4r::math::MTX34* pMtx, BOOL set, const nw4r::g3d::TexSrt* pSrt, u32 flag);

}
#endif

#endif /* MHTRI_G3D_G3D_XSI_H */
