/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s `pl_coll_shape_set`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_COLL_SHAPE_SET_H
#define MHTRI_PL_PL_COLL_SHAPE_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802950D8 - sets the player's collision shape for kind `b` */
void pl_coll_shape_set(struct _PLW* self, s32 a, s32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_COLL_SHAPE_SET_H */
