/*
 * MTX/vec.h - the entries `MTX/vec.c` owns.
 */
#ifndef MHTRI_MTX_VEC_H
#define MHTRI_MTX_VEC_H

#include "types.h"
#include "MTX/mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

void PSVECNormalize(const Vec* src, Vec* unit);
f32 PSVECMag(const Vec* v);
f32 PSVECDotProduct(const Vec* a, const Vec* b);
void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* axb);
void C_VECHalfAngle(const Vec* a, const Vec* b, Vec* half);
f32 PSVECSquareDistance(const Vec* a, const Vec* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX_VEC_H */
