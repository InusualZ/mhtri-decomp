/*
 * The `g3d/g3d_anmvis.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d/g3d_anmvis.cpp` (`.text` 0x8006EAC0-0x8006EE78) owns the two node-visibility walkers that apply
 * an `AnmObjVis` over a model's node table.  They were declared in the consumer's own file while the
 * range was unclaimed; a consumer (`g3d/g3d_scnmdl.cpp`, whose fn_8007D47C is the `ScnMdl` twin of
 * fn_8006ED84) now includes this header instead.
 *
 * Both keep C linkage (their map names are plain `fn_XXXXXXXX` stems).  The second parameter of
 * fn_8006ECB4 is the polymorphic animation object the owner declares; it is only ever passed through
 * here, so it arrives as `void*`.
 */
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ANMVIS_H */
