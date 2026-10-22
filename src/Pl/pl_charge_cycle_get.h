/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/fn_802840DC.cpp`'s `pl_charge_cycle_get`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_PL_PL_CHARGE_CYCLE_GET_H
#define MHTRI_PL_PL_CHARGE_CYCLE_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802843F0 - the length of the actor's charge cycle, which the rig divides the charge gauge by (GUESS name). */
s16 pl_charge_cycle_get(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_CHARGE_CYCLE_GET_H */
