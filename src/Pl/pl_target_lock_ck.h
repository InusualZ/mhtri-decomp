/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act`'s `pl_target_lock_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_PL_TARGET_LOCK_CK_H
#define MHTRI_PL_PL_TARGET_LOCK_CK_H

#include "types.h"

#ifdef __cplusplus
struct _PLW;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027A340 (Pl/pl_act): whether the player holds the target lock. */
s32 pl_target_lock_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_TARGET_LOCK_CK_H */
