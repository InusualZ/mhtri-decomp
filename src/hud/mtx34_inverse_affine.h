/* hud/mtx34_inverse_affine.h - the two paired-single matrix helpers `hud/pl_frame_sync.cpp` owns (docs/plan.md 6.5
 *   rule 2, leaf header).  Both names are GUESSES from the bodies (the 3x3 determinant, the reciprocal, the
 *   cofactor rows). */
#ifndef MHTRI_HUD_MTX34_INVERSE_AFFINE_H
#define MHTRI_HUD_MTX34_INVERSE_AFFINE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80330EF8 - writes the inverse transpose of `src`'s 3x3 part into `dst`. */
void mtx34_inverse_transpose(MTX34* dst, const MTX34* src);

/* 0x80330E14 - writes the 3x3 part of `src` into `dst`. */
void mtx34_to_mtx33(nw4r::math::MTX33* dst, const MTX34* src);

/* 0x80331000 - inverts the affine matrix `src` into `dst`; 1 on success, 0 when it is singular. */
u32 mtx34_inverse_affine(MTX34* dst, const MTX34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_MTX34_INVERSE_AFFINE_H */
