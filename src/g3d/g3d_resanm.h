/*
 * The `g3d/g3d_resanm.c` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).  The unit itself is
 * C (`fn_800898B0` keeps its map name), so the declarations carry C linkage.  Consumers used to
 * re-declare these in their own source; they belong with the TU that defines them.
 */
#ifndef MHTRI_G3D_G3D_RESANM_H
#define MHTRI_G3D_G3D_RESANM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800898B0 - the float animation-channel evaluator (caller: g3d_resanmcamera.cpp).  Takes the key
 * data at `pData`; the caller resolves the self-relative offset first. */
f32 fn_800898B0(void *pData, f32 frame);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANM_H */
