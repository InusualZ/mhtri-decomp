/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_scnroot.cpp`'s `scn_root_get_current_camera`, for a consumer that
 * cannot include the owner's full header beside its own scene types.  C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_SCN_ROOT_GET_CURRENT_CAMERA_H
#define MHTRI_G3D_SCN_ROOT_GET_CURRENT_CAMERA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80082BCC - returns the scene root `model`'s current camera handle. */
s32 scn_root_get_current_camera(s32 model);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_SCN_ROOT_GET_CURRENT_CAMERA_H */
