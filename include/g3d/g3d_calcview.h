/*
 * The `g3d/g3d_calcview.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d_calcview.cpp` (`.text` 0x8006F738-0x8007270C) owns the view/billboard-matrix calculator and the
 * `ResMdl` handle validity accessors the `g3d_resmdl_ac.h` inlined assert expands to.  A consumer
 * (`g3d/g3d_scnmdl.cpp`, whose fn_8007D404 is that assert's out-of-line copy) includes this header
 * instead of declaring them itself.
 *
 * Both keep C linkage (their map names are plain `fn_XXXXXXXX` stems).
 */
#ifndef MHTRI_G3D_G3D_CALCVIEW_H
#define MHTRI_G3D_G3D_CALCVIEW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* fn_8006FFBC(void);      /* 0x8006FFBC - the `ResMdl` type name the assert prints */
u32 fn_8006FFC8(void* pSelf);       /* 0x8006FFC8 - the `ResMdl` handle validity test */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVIEW_H */
