/* g3d/g3d_anmvis.h - the two node-visibility walkers `g3d/g3d_anmvis.cpp` owns (C linkage); fn_8006ECB4's
 *   animation object is passed through as `void*`. */
#ifndef MHTRI_G3D_G3D_ANMVIS_H
#define MHTRI_G3D_G3D_ANMVIS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006ECB4 - walk the model's node table and forward each visible node's virtual result. */
void fn_8006ECB4(void* pModel, void* pSelf);
/* 0x8006ED84 - the same walk, writing one byte per node into `pByteVec`. */
void fn_8006ED84(u8* pByteVec, void* pModel, void* pSelf);

/* The checked `ResAnmVis` resolver `g3d/g3d_resfile.cpp` calls. */
u32 fn_8006EC3C(void* p); /* 0x8006EC3C - the checked `ResAnmVis` resolver */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ANMVIS_H */
