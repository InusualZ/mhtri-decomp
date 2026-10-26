/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_state_enter_aim`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_ACT_STATE_ENTER_AIM_H
#define MHTRI_PL_PL_ACT_STATE_ENTER_AIM_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802761DC - enters the act state, then sets the aim motion pair (registry name). */
void pl_act_state_enter_aim(struct _PLW* self, u32 a, u32 b, u32 c, u32 up_motion, u32 down_motion);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STATE_ENTER_AIM_H */
