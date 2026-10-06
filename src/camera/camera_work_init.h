/* Leaf header (docs/plan.md 6.5 rule 2): the `camera/camera_main.cpp` symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_CAMERA_CAMERA_WORK_INIT_H
#define MHTRI_CAMERA_CAMERA_WORK_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B56E0 / 0x802B8B8C - set both camera works up, and reset the camera for a new area (GUESS names). */
void camera_work_init(void);
void camera_area_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_WORK_INIT_H */
