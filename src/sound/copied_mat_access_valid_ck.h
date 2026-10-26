/* sound/copied_mat_access_valid_ck.h - leaf header: `copied_mat_access_valid_ck` (0x800E2994), owned by
 * `sound/mhchar.cpp`. C linkage: the map carries the plain name.  The callers pass the material-access block they
 * built on the stack. */
#ifndef MHTRI_SOUND_COPIED_MAT_ACCESS_VALID_CK_H
#define MHTRI_SOUND_COPIED_MAT_ACCESS_VALID_CK_H

#include "types.h"
#include "nw4r/g3d/scnmdl.h"

extern "C" {
/* Returns non-zero when `access` resolved a material. */
u32 copied_mat_access_valid_ck(nw4r::g3d::ScnMdl::CopiedMatAccess* access);
}

#endif
