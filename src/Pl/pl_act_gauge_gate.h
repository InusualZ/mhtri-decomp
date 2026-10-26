/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_gauge_gate`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_ACT_GAUGE_GATE_H
#define MHTRI_PL_PL_ACT_GAUGE_GATE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802768F8 - gates the act on the actor's gauge by a signed amount. */
void pl_act_gauge_gate(struct _PLW* plw, s16 amount);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_GAUGE_GATE_H */
