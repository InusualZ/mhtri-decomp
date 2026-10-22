/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll`'s `coll_line_check`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_COLL_LINE_CHECK_H
#define MHTRI_PL_COLL_LINE_CHECK_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80293A88 (Pl/pl_coll): tests the line from `from` to `to` against the collision; positive with the hit point in `hit` when it hits. */
s32 coll_line_check(nw4r::math::VEC3* from, nw4r::math::VEC3* to, f32 radius, nw4r::math::VEC3* hit, s32 kind, s32 mask, u8 area, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_COLL_LINE_CHECK_H */
