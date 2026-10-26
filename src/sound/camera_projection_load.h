/* sound/camera_projection_load.h - leaf header (docs/plan.md 6.5 rule 2) for `camera_projection_load` (0x800E4754),
 * which `sound/fn_800E46E8.cpp` owns: it builds the perspective projection from the current camera and screen
 * setup and hands it to `GXSetProjection`.  C linkage: the owner defines it `extern "C"`. */
#ifndef MHTRI_SOUND_CAMERA_PROJECTION_LOAD_H
#define MHTRI_SOUND_CAMERA_PROJECTION_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void camera_projection_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_CAMERA_PROJECTION_LOAD_H */
