/* Leaf header (docs/plan.md 6.5 rule 2): `pl_coll_tri_hit_ck`, which `Pl/pl_coll` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_PL_PL_COLL_TRI_HIT_CK_H
#define MHTRI_PL_PL_COLL_TRI_HIT_CK_H

#include "types.h"
#include "nw4r/math.h" /* VEC3 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80291114 - tests a point and radius against a triangle; returns 1 and the hit height on a hit. */
u32 pl_coll_tri_hit_ck(VEC3* pos, f32 radius, VEC3* a, VEC3* b, VEC3* c, f32* height);

#ifdef __cplusplus
}
#endif

#endif
