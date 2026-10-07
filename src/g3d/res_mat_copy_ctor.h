/* g3d/res_mat_copy_ctor.h - the leaf header of `g3d/fn_80063888.cpp`'s material-handle copies and its colour-result
 *   applier: declared apart from `g3d/fn_80063888.h`, whose global placement `operator delete` would give a consumer's
 *   placement `new` a landing pad retail does not have. */
#ifndef MHTRI_G3D_RES_MAT_COPY_CTOR_H
#define MHTRI_G3D_RES_MAT_COPY_CTOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The material handle's copy constructor and its base-handle copy (one word each). */
struct ResHandle* res_mat_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);
void res_mat_common_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { struct ClrAnmResult; } }

/* 0x80064128 (0x5C0): applies a colour result to a material's channel and tev-colour blocks. */
extern "C" void apply_clr_anm_result(struct ResHandle chan, struct ResHandle tevColor,
                                     const nw4r::g3d::ClrAnmResult* pResult);
#endif

#endif /* MHTRI_G3D_RES_MAT_COPY_CTOR_H */
