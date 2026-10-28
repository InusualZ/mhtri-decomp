/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resanmchr.cpp`'s `anm_chr_result_rot_mtx_get` (`ChrAnmResult::GetRotMtx`).
 * C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_ANM_CHR_RESULT_ROT_MTX_GET_H
#define MHTRI_G3D_ANM_CHR_RESULT_ROT_MTX_GET_H

#include "types.h"

struct ResAnmChrObj;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008DF8C (0x234): writes the result's 3x3 rotation (identity when the result carries none) into `out`. */
void anm_chr_result_rot_mtx_get(struct ResAnmChrObj* self, f32* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_ANM_CHR_RESULT_ROT_MTX_GET_H */
