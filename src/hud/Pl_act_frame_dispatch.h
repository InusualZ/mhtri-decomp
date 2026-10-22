/* Leaf header (docs/plan.md 6.5 rule 2): `hud/pl_frame_sync.cpp`'s `Pl_act_frame_dispatch`, which `Pl/pl_act_step.cpp`'s act
 * class dispatcher tail-calls. */
#ifndef MHTRI_HUD_PL_ACT_FRAME_DISPATCH_H
#define MHTRI_HUD_PL_ACT_FRAME_DISPATCH_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80334758 - the class-8 weapon act dispatcher. */
void Pl_act_frame_dispatch(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_ACT_FRAME_DISPATCH_H */
