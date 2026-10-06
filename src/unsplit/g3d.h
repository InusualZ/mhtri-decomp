/*
 * The `g3d` band header (docs/plan.md 6.5 rule 2): declarations of `g3d`-module symbols no registered unit owns.
 *
 * It declares the scene objects' type-name records, which no registered unit owns; every other declaration lives in its owner's header (`g3d/g3d_calcview.h`, `g3d/g3d_scnroot.h`, `g3d/g3d_state.h`, ...).  What is left is the include set its
 * includers still read through it: the math types, the `pRoot` scene root, `g3d/g3d_scnmdl.h` and `nw4r::db::Panic`.
 */
#ifndef MHTRI_UNSPLIT_G3D_H
#define MHTRI_UNSPLIT_G3D_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/pRoot.h"
#include "g3d/g3d_scnmdl.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */

/* The `g3d_calcworld`/`g3d_camera` resource types; only ever used through a pointer, so the incomplete type is
 * enough. */
struct G3DWorkObj;
struct RenderModeObj;

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h" /* nw4r::math::Frustum, owner nw4r/fn_805012C4.cpp (rule 2) */

extern "C" {
/* The "ScnObj", "ScnLeaf" and "ScnGroup" type-name records (`.rodata` 0x8056F6A0/0x8056F6B0/0x8056F6C0: a
 * length word, then the NUL-terminated name) the scene objects' run-time type members read; no registered range
 * covers them. */
extern u8 scn_typename_ScnObj[];
extern u8 scn_typename_ScnLeaf[];
extern u8 scn_typename_ScnGroup[];
}
#endif

#endif /* MHTRI_UNSPLIT_G3D_H */
