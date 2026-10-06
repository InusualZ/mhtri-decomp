/*
 * camera/camera_frame_get.h - leaf header (docs/plan.md 6.5 rule 2) for `camera/camera_main.cpp`'s `camera_frame_get`
 *   (0x802BC82C); the owner defines it inside its file-wide `extern "C"` block.
 */
#ifndef MHTRI_CAMERA_CAMERA_FRAME_GET_H
#define MHTRI_CAMERA_CAMERA_FRAME_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The camera work's frame counter (+0x288), one less when `full` is set.  GUESS name. */
s16 camera_frame_get(u8 full);

/* 0x802BC468 - clears the camera work's talk bytes.  GUESS name. */
void camera_talk_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_FRAME_GET_H */
