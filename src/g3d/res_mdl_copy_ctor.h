/* g3d/res_mdl_copy_ctor.h - leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_scninline.cpp`'s 0x8007BA08, for
 *   `menu/menu_placeinfo.cpp`.  C linkage (the map row is a plain name); the signature is the owner's definition.
 *   GUESS name (registry). */
#ifndef MHTRI_G3D_RES_MDL_COPY_CTOR_H
#define MHTRI_G3D_RES_MDL_COPY_CTOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ResHandle;

/* 0x8007BA08: copy-constructs a model handle through its base-handle copy and returns the destination. */
struct ResHandle* res_mdl_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_MDL_COPY_CTOR_H */
