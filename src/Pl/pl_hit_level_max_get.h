/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_hit_level_max_get` (0x8027CBC8). */
#ifndef MHTRI_PL_PL_HIT_LEVEL_MAX_GET_H
#define MHTRI_PL_PL_HIT_LEVEL_MAX_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027CBC8: the larger level of the player's two live hit records (registry name, GUESS). */
u8 pl_hit_level_max_get(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_HIT_LEVEL_MAX_GET_H */
