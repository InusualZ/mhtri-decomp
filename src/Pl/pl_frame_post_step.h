/* Pl/pl_frame_post_step.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s `pl_frame_post_step`, for
 * `Pl/player_control.cpp`. */
#ifndef MHTRI_PL_PL_FRAME_POST_STEP_H
#define MHTRI_PL_PL_FRAME_POST_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80266F30 (0x340): the player's post-frame step the draw pass runs. */
void pl_frame_post_step(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_FRAME_POST_STEP_H */
