/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_aim_motion_set`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_AIM_MOTION_SET_H
#define MHTRI_PL_PL_AIM_MOTION_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80279EBC - sets the aim blend from the up/down motion pair by the sign of the aim tilt (registry name). */
void pl_aim_motion_set(struct _PLW* self, u16 up_motion, u16 down_motion);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_AIM_MOTION_SET_H */
