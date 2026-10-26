/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s `pl_act_guard_gauge_adjust`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_ACT_GUARD_GAUGE_ADJUST_H
#define MHTRI_PL_PL_ACT_GUARD_GAUGE_ADJUST_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80252AEC - steps the guard gauge by the two part flags and clamps it to the two limits. */
void pl_act_guard_gauge_adjust(struct _PLW* self, u16 neg_limit, u16 pos_limit, s32 flag_a, s32 flag_b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_GUARD_GAUGE_ADJUST_H */
