/* Leaf header (docs/plan.md 6.5 rule 2): `res_anm_chr_copy_ctor`, which `g3d/g3d_anmchr.cpp` owns.  C linkage. */
#ifndef MHTRI_G3D_RES_ANM_CHR_COPY_CTOR_H
#define MHTRI_G3D_RES_ANM_CHR_COPY_CTOR_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class ResAnmChr;
}
}

extern "C" {
/* 0x80061B00 (0x30): copies the animation handle `pSrc` into `pDst` and returns `pDst`. */
nw4r::g3d::ResAnmChr* res_anm_chr_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc);
}
#endif

#endif /* MHTRI_G3D_RES_ANM_CHR_COPY_CTOR_H */
