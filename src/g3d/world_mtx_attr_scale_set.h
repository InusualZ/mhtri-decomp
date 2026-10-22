/* g3d/world_mtx_attr_scale_set.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `world_mtx_attr_scale_set` (0x800736F8), which `g3d/g3d_calcworld.cpp` owns;
 * it stores a node's three scale values and picks its scale-mode flag. */
#ifndef MHTRI_G3D_WORLD_MTX_ATTR_SCALE_SET_H
#define MHTRI_G3D_WORLD_MTX_ATTR_SCALE_SET_H

#include "types.h"

struct VtxAttrRef;

#ifdef __cplusplus
extern "C" {
#endif

void world_mtx_attr_scale_set(struct VtxAttrRef* pSelf, f32 x, f32 y, f32 z);

#ifdef __cplusplus
}
#endif

#endif
