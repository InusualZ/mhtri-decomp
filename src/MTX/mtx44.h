/*
 * MTX/mtx44.h - the entries `MTX/mtx44.c` owns: the 4x4 projections and `PSVECAdd`.
 */
#ifndef MHTRI_MTX_MTX44_H
#define MHTRI_MTX_MTX44_H

#include "types.h"
#include "MTX/mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

void C_MTXFrustum(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f);
void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f);
void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f);
void PSVECAdd(const Vec* a, const Vec* b, Vec* ab);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX_MTX44_H */
