/* g3d/g3d_resanm.h - the cross-unit declarations of the C unit `g3d/g3d_resanm.c` (C linkage). */
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
