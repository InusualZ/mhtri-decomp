/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act`'s `pl_camera_close_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_PL_CAMERA_CLOSE_CK_H
#define MHTRI_PL_PL_CAMERA_CLOSE_CK_H

#include "types.h"

#ifdef __cplusplus
struct _PLW;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027CC2C (Pl/pl_act): whether the player's +0x5A4 flag 0x40 asks for the close camera. */
u32 pl_camera_close_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_CAMERA_CLOSE_CK_H */
