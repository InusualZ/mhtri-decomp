/* g3d/anm_chr_result_scale_get.h - the declarations of the four result accessors `g3d/g3d_resanmchr.cpp` owns
 * (docs/plan.md 6.5 rule 2); a leaf header because `g3d/g3d_resanmchr.h` does not see the result record. */
#ifndef MHTRI_G3D_ANM_CHR_RESULT_SCALE_GET_H
#define MHTRI_G3D_ANM_CHR_RESULT_SCALE_GET_H

#include "types.h"

struct ResAnmChrObj;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8008DA10 / 0x8008E1C0 - the checked getters of the result's scale and translation triples. */
void anm_chr_result_scale_get(struct ResAnmChrObj* self, f32* out);
void anm_chr_result_trans_get(struct ResAnmChrObj* self, f32* out);
/* 0x8008E8D0 / 0x8008F148 - the checked setters of the result's scale and translation triples. */
void anm_chr_result_scale_set(struct ResAnmChrObj* self, const f32* in);
void anm_chr_result_trans_set(struct ResAnmChrObj* self, const f32* in);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_ANM_CHR_RESULT_SCALE_GET_H */
