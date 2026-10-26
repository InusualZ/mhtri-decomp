/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_hold_notify`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_ACT_HOLD_NOTIFY_H
#define MHTRI_PL_PL_ACT_HOLD_NOTIFY_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802754C0 - notifies the act's hold latch. */
void pl_act_hold_notify(struct _PLW* plw, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_HOLD_NOTIFY_H */
