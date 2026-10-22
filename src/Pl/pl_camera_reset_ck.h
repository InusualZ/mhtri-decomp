/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act`'s `pl_camera_reset_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_PL_CAMERA_RESET_CK_H
#define MHTRI_PL_PL_CAMERA_RESET_CK_H

#include "types.h"

#ifdef __cplusplus
struct _PLW;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027A2A0 (Pl/pl_act): whether the player's state asks the follow camera to turn behind it. */
u32 pl_camera_reset_ck(struct _PLW* self, s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_CAMERA_RESET_CK_H */
