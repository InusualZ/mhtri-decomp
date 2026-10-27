/* drawshape_set_rot_scale_ivec2.h - leaf header for `drawshape_set_rot_scale_ivec2`, which `src/draw_shape.cpp`
 * defines with C linkage (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it. */
#ifndef MHTRI_DRAWSHAPE_SET_ROT_SCALE_IVEC2_H
#define MHTRI_DRAWSHAPE_SET_ROT_SCALE_IVEC2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80054058 - rotates the shape by `angle` and scales it uniformly by `scale` about the s16 position `pos`. */
void drawshape_set_rot_scale_ivec2(const s16* pos, u16 angle, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAWSHAPE_SET_ROT_SCALE_IVEC2_H */
