/* camera/camera_mode_req.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `camera_mode_req` (0x802BC4AC), which `camera/camera_main.cpp` owns;
 * it requests camera mode `kind` with its argument. */
#ifndef MHTRI_CAMERA_CAMERA_MODE_REQ_H
#define MHTRI_CAMERA_CAMERA_MODE_REQ_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void camera_mode_req(u32 kind, u32 arg);

#ifdef __cplusplus
}
#endif

#endif
