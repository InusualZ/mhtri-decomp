/* Leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp`'s `camera_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_CAMERA_CAMERA_UPDATE_H
#define MHTRI_CAMERA_CAMERA_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B58D4 (0x58): the camera's per-frame update. */
void camera_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_UPDATE_H */
