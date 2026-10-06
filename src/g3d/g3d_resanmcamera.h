/* g3d/g3d_resanmcamera.h - the cross-unit declarations of `g3d/g3d_resanmcamera.cpp`, in the consumers'
 *   spellings (the wider form where only a parameter spelling differed). */
#ifndef MHTRI_G3D_G3D_RESANMCAMERA_H
#define MHTRI_G3D_G3D_RESANMCAMERA_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_8008A220(void* out, s32 arg1);
/* 0x8008A644 - the float channel reader: the inline float at +0x0 of `self` when `flag` is set, else the
 * referenced channel evaluated at `frame` (callers: g3d_resanmfog.cpp, g3d_resanmlight.cpp). */
f32 fn_8008A644(u32 *self, f32 frame, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMCAMERA_H */
