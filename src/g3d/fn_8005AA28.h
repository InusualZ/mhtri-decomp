/* g3d/fn_8005AA28.h - the `ResMat` accessors `g3d/fn_8005AA28.cpp` owns (C linkage, plain map stems); the handle
 *   is passed as `void*`, its one-word layout being the owner's. */
#ifndef MHTRI_G3D_FN_8005AA28_H
#define MHTRI_G3D_FN_8005AA28_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The `g3d_resmat_ac.h` assert-then-set/clear helper: with `enable` non-zero it sets bit 0x100 of the
 * resource's mat flag word, otherwise it clears it. */
void fn_8005AA44(void* pSelf, u32 enable);

/* 0x8005AB00 - the `ResMat` handle's resource pointer; the ScnMdl unit's replacement passes read it
 * (rule 2: declared here, in its owner's header, not in the consumer). */

/* 0x8005AB08 (0x70): translates `pSrc` by `pPos` into the node matrix (`PSMTXTransApply`) and returns `pSrc`. */
/* untyped: opaque handle - the node matrix, read through its accessor */
Mtx34* mtx34_trans_apply(Mtx34* pSrc, const Vec3* pPos, void* pNodeMtx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_8005AA28_H */
