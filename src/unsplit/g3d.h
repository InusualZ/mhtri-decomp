/*
 * The `g3d` band header (docs/plan.md 6.5 rule 2): declarations of `g3d`-module symbols no registered unit owns.
 *
 * Every declaration lives in its owner's header (`g3d/g3d_calcview.h`, `g3d/g3d_scnroot.h`, `g3d/g3d_state.h`, ...).  What is left is the include set its
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
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8056F688 - the "ScnMdlSimple" type-name record (`.rodata`: a length word, then the NUL-terminated name)
 * ScnMdlSimple's run-time type members read; no registered range covers it. */
extern u8 scn_typename_ScnMdlSimple[];
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_G3D_H */
