/* g3d/g3d_calcmaterial.h - the alignment-asserting `g3d_resmat_ac.h` pointer constructors `g3d/g3d_calcmaterial.cpp`
 *   owns (C linkage). */
#ifndef MHTRI_G3D_G3D_CALCMATERIAL_H
#define MHTRI_G3D_G3D_CALCMATERIAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r { namespace g3d { class ResMdl; } } /* only pointed to here: the full class is g3d/g3d_resmat.h's */

namespace nw4r {
namespace g3d {
class AnmObjTexPat;
class AnmObjTexSrt;
class AnmObjMatClr;
}  // namespace g3d
}  // namespace nw4r

/* 0x8006EE78 - applies the texture-pattern, texture-SRT and colour animations to the model's materials. */
extern "C" void g3d_calc_material_directly(const nw4r::g3d::ResMdl* pMdl, nw4r::g3d::AnmObjTexPat* pTexPat,
                                           nw4r::g3d::AnmObjTexSrt* pTexSrt, nw4r::g3d::AnmObjMatClr* pMatClr);

/* 0x8006F304 - the word copy through a reference.  C++-only: `const u32&` cannot be spelled in C, and
 * `extern "C"` keeps the plain map name the target objects reference while the reference parameter stays
 * (load-bearing for the caller's stack layout - see eft002.cpp's file header). */
extern "C" void fn_8006F304(void* dst, const u32& src);
#endif

#endif /* MHTRI_G3D_G3D_CALCMATERIAL_H */
