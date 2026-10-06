/* g3d/g3d_resanmamblight.h - the channel evaluators of `g3d/g3d_resanmamblight.c` that `g3d/g3d_resanmfog.cpp`
 *   also calls (C linkage), declared `(self, f32 frame, s32 flag)` - the order the retail fog call site schedules;
 *   the owner's `(self, s32 flag, f32 frame)` definition is ABI-identical. */
#ifndef MHTRI_G3D_G3D_RESANMAMBLIGHT_H
#define MHTRI_G3D_G3D_RESANMAMBLIGHT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008A188 - `flag` set returns the inline word, else evaluates the referenced channel. */
s32 fn_8008A188(u32 *self, f32 frame, s32 flag);

/* 0x8008A1A8 - clamps `frame` into [0, count]. */
f32 fn_8008A1A8(u16 *count, f32 frame);

/* 0x80089F94 - the `ResAnmAmbLight` value-type constructor (writes `v` to `out`); called by the
 * `g3d/g3d_resanmtexsrt.cpp` `ResFile` accessor family. */
void* fn_80089F94(void* out, u32 v);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMAMBLIGHT_H */
