/* Pl/pl_frame_step.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act_step.cpp`'s per-frame player step, for
 * `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_PL_PL_FRAME_STEP_H
#define MHTRI_PL_PL_FRAME_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026099C (0x7C): runs one frame of a live player: the first frame, then the act layer outside event demos. */
void pl_frame_step(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_FRAME_STEP_H */
