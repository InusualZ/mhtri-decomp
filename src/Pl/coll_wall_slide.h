/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s `coll_wall_slide`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_COLL_WALL_SLIDE_H
#define MHTRI_PL_COLL_WALL_SLIDE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8029208C - slides the move `from` -> `to` along the walls of area `area`, writing `out` */
s32 coll_wall_slide(VEC3* from, VEC3* to, VEC3* out, s32 mask, s32 a5, s32 a6, u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_COLL_WALL_SLIDE_H */
