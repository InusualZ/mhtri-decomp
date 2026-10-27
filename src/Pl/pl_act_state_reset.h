/* Pl/pl_act_state_reset.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s `pl_act_state_reset`, for
 * `Pl/player_control.cpp`. */
#ifndef MHTRI_PL_PL_ACT_STATE_RESET_H
#define MHTRI_PL_PL_ACT_STATE_RESET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80245FD8 (0x180): resets the actor's act state. */
void pl_act_state_reset(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STATE_RESET_H */
