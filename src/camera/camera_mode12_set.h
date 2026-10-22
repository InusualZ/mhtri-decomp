/* camera/camera_mode12_set.h - the declaration of `camera_mode12_set`, which `camera/camera_main.cpp` owns
 * (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_CAMERA_CAMERA_MODE12_SET_H
#define MHTRI_CAMERA_CAMERA_MODE12_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x802BC7B0 - puts the camera into mode 12 with the caller's argument. */
void camera_mode12_set(u32 arg);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_MODE12_SET_H */
