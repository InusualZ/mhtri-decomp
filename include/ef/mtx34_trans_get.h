/* ef/mtx34_trans_get.h - the declaration of `mtx34_trans_get`, which `ef/eft004_fx.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_MTX34_TRANS_GET_H
#define MHTRI_EF_MTX34_TRANS_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
void mtx34_trans_get(nw4r::math::MTX34* mtx, nw4r::math::VEC3* out);
#ifdef __cplusplus
}
#endif

#endif
