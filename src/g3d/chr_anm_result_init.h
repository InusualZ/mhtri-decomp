/* Leaf header (docs/plan.md 6.5 rule 2): `chr_anm_result_init` and `res_anm_chr_copy_ctor`, which `g3d/g3d_anmchr.cpp`
 * owns.  C linkage. */
#ifndef MHTRI_G3D_CHR_ANM_RESULT_INIT_H
#define MHTRI_G3D_CHR_ANM_RESULT_INIT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
struct ChrAnmResult;
class ResAnmChr;
}
}

extern "C" {
/* 0x80061068 (0x44): constructs a character animation result (scale, rotation vector and matrix). */
nw4r::g3d::ChrAnmResult* chr_anm_result_init(nw4r::g3d::ChrAnmResult* pSelf);
/* 0x80061B00 (0x30): copies the animation handle `pSrc` into `pDst` and returns `pDst`. */
nw4r::g3d::ResAnmChr* res_anm_chr_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc);
}
#endif

#endif /* MHTRI_G3D_CHR_ANM_RESULT_INIT_H */
