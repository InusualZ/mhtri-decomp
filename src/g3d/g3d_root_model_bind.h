/* g3d/g3d_root_model_bind.h - leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_scnmdl.cpp`'s `g3d_root_model_bind`,
 *   for a consumer that cannot include the owner's full header beside `sound/se.h` (the two declare
 *   `scn_mdl_clean_mat_buffer` with different parameters).  C linkage; the signature is the owner header's. */
#ifndef MHTRI_G3D_G3D_ROOT_MODEL_BIND_H
#define MHTRI_G3D_G3D_ROOT_MODEL_BIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007F0CC: binds model `id` under the scene root `root`. */
void g3d_root_model_bind(s32 root, u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ROOT_MODEL_BIND_H */
