/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_act_step_tail.cpp`'s `eft_frame_pl_create`. */
#ifndef MHTRI_ENEMY_EFT_FRAME_PL_CREATE_H
#define MHTRI_ENEMY_EFT_FRAME_PL_CREATE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void eft_frame_pl_create(struct _PLW* source, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EFT_FRAME_PL_CREATE_H */
