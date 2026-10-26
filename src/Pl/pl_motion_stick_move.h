/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_motion_stick_move`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_MOTION_STICK_MOVE_H
#define MHTRI_PL_PL_MOTION_STICK_MOVE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027A57C - moves the actor by its stick input for one motion (registry name). */
void pl_motion_stick_move(struct _PLW* self, u16 motion, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MOTION_STICK_MOVE_H */
