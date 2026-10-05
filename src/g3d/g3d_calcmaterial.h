/*
 * The `g3d/g3d_calcmaterial.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d/g3d_calcmaterial.cpp` (`.text` 0x8006EE78-0x8006F738) owns the alignment-asserting resource
 * pointer constructors of the `g3d_resmat_ac.h` inlines.  `g3d/g3d_resfile.cpp`'s accessor cluster calls
 * two of them, so they are declared once here (the owner's header) and that consumer includes it.
 *
 * Both keep C linkage (their map names are plain `fn_XXXXXXXX` stems).
 */
#ifndef MHTRI_G3D_G3D_CALCMATERIAL_H
#define MHTRI_G3D_G3D_CALCMATERIAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32* fn_8006F158(u32* pDst, u32 value); /* 0x8006F158 - the 0x20-aligned handle constructor */
u32* fn_8006F298(u32* pDst, u32 value); /* 0x8006F298 - the 0x20-aligned handle constructor */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCMATERIAL_H */
