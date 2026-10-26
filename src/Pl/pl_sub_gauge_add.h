/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_sub_gauge_add` (0x80276868). */
#ifndef MHTRI_PL_PL_SUB_GAUGE_ADD_H
#define MHTRI_PL_PL_SUB_GAUGE_ADD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80276868: adds `delta` to the master player's +0x378 gauge, clamped to 0..+0x37A (registry name, GUESS). */
void pl_sub_gauge_add(struct _PLW* plw, s16 delta);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SUB_GAUGE_ADD_H */
