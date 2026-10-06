/* g3d/g3d_calcmaterial.h - the alignment-asserting `g3d_resmat_ac.h` pointer constructors `g3d/g3d_calcmaterial.cpp`
 *   owns (C linkage). */
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

#ifdef __cplusplus
/* 0x8006F304 - the word copy through a reference.  C++-only: `const u32&` cannot be spelled in C, and
 * `extern "C"` keeps the plain map name the target objects reference while the reference parameter stays
 * (load-bearing for the caller's stack layout - see eft002.cpp's file header). */
extern "C" void fn_8006F304(void* dst, const u32& src);
#endif

#endif /* MHTRI_G3D_G3D_CALCMATERIAL_H */
