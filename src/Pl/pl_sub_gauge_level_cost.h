/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s `pl_sub_gauge_level_cost`, which `Pl/pl_act_class3.cpp`
 * calls. */
#ifndef MHTRI_PL_PL_SUB_GAUGE_LEVEL_COST_H
#define MHTRI_PL_PL_SUB_GAUGE_LEVEL_COST_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80264A28 - charges the actor's sub gauge the cost its skill-adjusted level picks (registry name). */
void pl_sub_gauge_level_cost(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SUB_GAUGE_LEVEL_COST_H */
