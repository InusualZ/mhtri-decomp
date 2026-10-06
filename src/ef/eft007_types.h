/* ef/eft007_types.h - the types and macros `ef/eft007.cpp` and `ef/em_effect_ctrl.cpp` share (one definition each). */
#ifndef MHTRI_EF_EFT007_TYPES_H
#define MHTRI_EF_EFT007_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef/eft004.h"
#include "ef/eft009.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"

/* The engine's own 3-float vector.  It is NOT `nw4r::math::VEC3`: `vec_to_mh_vec3` exists to convert
 * between the two (`nw4r::math::VEC3* dst, Vec* src`), so they are distinct types that happen to share
 * a layout.  `ef.h` carries the canonical copy; this unit views its records locally, so it repeats the
 * type here (docs/plan.md 6.5 rule 1 debt, tracked in the campaign note). size: 0x0C */
typedef struct Vec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec; /* size: 0x0C */
#endif /* MHTRI_EF_EFT007_TYPES_H */
