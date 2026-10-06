/* g3d/g3d_calcview.h - the `ResMdl` handle validity accessors `g3d/g3d_calcview.cpp` owns (C linkage). */
#ifndef MHTRI_G3D_G3D_CALCVIEW_H
#define MHTRI_G3D_G3D_CALCVIEW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* fn_8006FFBC(void);      /* 0x8006FFBC - the `ResMdl` type name the assert prints */
u32 fn_8006FFC8(void* pSelf);       /* 0x8006FFC8 - the `ResMdl` handle validity test */

/* The checked resource resolver `g3d/g3d_resfile.cpp` calls. */
u32 fn_800700C0(void* p); /* 0x800700C0 - the checked resource resolver */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVIEW_H */
