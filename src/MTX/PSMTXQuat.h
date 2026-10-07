/* MTX/PSMTXQuat.h - `PSMTXQuat`, which `MTX/mtx.c` owns (docs/plan.md 6.5 rule 2, leaf header).
 *   PSMTXQuat is a GUESS (0x804C6430 sits between PSMTXScaleApply and the rotation builders of the SDK's mtx.c). */
#ifndef MHTRI_MTX_PSMTXQUAT_H
#define MHTRI_MTX_PSMTXQUAT_H

#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C6430 - builds the rotation matrix of the quaternion `q` into `m`. */
void PSMTXQuat(nw4r::math::MTX34* m, const nw4r::math::QUAT* q);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX_PSMTXQUAT_H */
