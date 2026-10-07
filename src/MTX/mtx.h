/*
 * MTX/mtx.h - the Revolution SDK MTX library types (vector, quaternion, 3x4 and 4x4 matrices) and the entry points of
 *    `MTX/mtx.c`.  C view; the C++ consumers (g3d, ef) reach the same symbols through their own typed declarations.
 *    The other units' entries are in `MTX/mtxvec.h`, `MTX/mtx44.h` and `MTX/vec.h`.
 */
#ifndef MHTRI_MTX_MTX_H
#define MHTRI_MTX_MTX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vec {
    f32 x; /* +0x00 */
    f32 y; /* +0x04 */
    f32 z; /* +0x08 */
} Vec; /* size: 0x0C */

typedef struct Quaternion {
    f32 x; /* +0x00 */
    f32 y; /* +0x04 */
    f32 z; /* +0x08 */
    f32 w; /* +0x0C */
} Quaternion; /* size: 0x10 */

typedef f32 Mtx[3][4];    /* size: 0x30 */
typedef f32 Mtx44[4][4];  /* size: 0x40 */

/* MTX/mtx.c */
void PSMTXIdentity(Mtx m);
void PSMTXCopy(const Mtx src, Mtx dst);
void PSMTXConcat(const Mtx a, const Mtx b, Mtx ab);
void PSMTXConcatArray(const Mtx a, const Mtx* srcBase, Mtx* dstBase, u32 count);
u32 PSMTXInverse(const Mtx src, Mtx inv);
u32 PSMTXInvXpose(const Mtx src, Mtx xPose);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
void PSMTXRotTrig(Mtx m, char axis, f32 sinA, f32 cosA);
void __PSMTXRotAxisRadInternal(Mtx m, const Vec* axis, f32 sinA, f32 cosA);
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad);
void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT);
void PSMTXTransApply(const Mtx src, Mtx dst, f32 xT, f32 yT, f32 zT);
void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS);
void PSMTXScaleApply(const Mtx src, Mtx dst, f32 xS, f32 yS, f32 zS);
void PSMTXQuat(Mtx m, const Quaternion* q);
void C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* camUp, const Vec* target);
void C_MTXLightFrustum(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 scaleS, f32 scaleT, f32 transS, f32 transT);
void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT);
void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX_MTX_H */
