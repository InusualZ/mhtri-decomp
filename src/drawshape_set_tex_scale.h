/* drawshape_set_tex_scale.h - leaf header for `drawshape_set_tex_scale`, which `src/draw_shape.cpp` defines with
 * C linkage (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it. */
#ifndef MHTRI_DRAWSHAPE_SET_TEX_SCALE_H
#define MHTRI_DRAWSHAPE_SET_TEX_SCALE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80054370 - scales the shape's texture coordinates by the two floats at `scale`, then moves them by `offset`. */
void drawshape_set_tex_scale(const f32* scale, const f32* offset);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAWSHAPE_SET_TEX_SCALE_H */
