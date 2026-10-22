/* Leaf header (docs/plan.md 6.5 rule 2): `pl_ride_down_set`, which `Pl/pl_act` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_PL_PL_RIDE_DOWN_SET_H
#define MHTRI_PL_PL_RIDE_DOWN_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80278BE4 - the act of a master player riding an enemy with no health. */
void pl_ride_down_set(struct _PLW* pl);

#ifdef __cplusplus
}
#endif

#endif
