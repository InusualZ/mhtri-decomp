/*
 * Declarations for the symbols `src/camera/camera_main.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_CAMERA_CAMERA_MAIN_H
#define MHTRI_CAMERA_CAMERA_MAIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BE038 */
u32 get_camera_rot_y(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_MAIN_H */
