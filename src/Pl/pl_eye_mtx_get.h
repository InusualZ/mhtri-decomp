/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act`'s `pl_eye_mtx_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_PL_EYE_MTX_GET_H
#define MHTRI_PL_PL_EYE_MTX_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
struct _PLW;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027CD0C (Pl/pl_act): builds the player's eye matrix. */
void pl_eye_mtx_get(struct _PLW* self, nw4r::math::MTX34* mtx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_EYE_MTX_GET_H */
