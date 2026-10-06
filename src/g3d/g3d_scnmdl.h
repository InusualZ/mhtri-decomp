/* g3d/g3d_scnmdl.h - the cross-unit declarations of `g3d/g3d_scnmdl.cpp`. */
#ifndef MHTRI_G3D_G3D_SCNMDL_H
#define MHTRI_G3D_G3D_SCNMDL_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/pRoot.h"

/* The unit's cross-unit declarations (rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

void g3d_root_model_bind(s32 root, u32 id);
/* 0x8007D404 - the `ResMdlInfo` handle's block (asserting the handle is valid). */
u32 res_mdl_info_ref(const void* pInfo); /* untyped: opaque handle - the info handle */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_SCNMDL_H */
