/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_camera_lock_clear`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_CAMERA_LOCK_CLEAR_H
#define MHTRI_PL_PL_CAMERA_LOCK_CLEAR_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027A17C - clears the actor's camera lock bytes +0x5E5/+0x5E6/+0x5E8 (registry name). */
void pl_camera_lock_clear(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_CAMERA_LOCK_CLEAR_H */
