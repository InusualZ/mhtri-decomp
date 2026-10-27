/* drawshape_set_tex_translate.h - leaf header for `drawshape_set_tex_translate`, which `src/draw_shape.cpp` defines
 * with C linkage (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it. */
#ifndef MHTRI_DRAWSHAPE_SET_TEX_TRANSLATE_H
#define MHTRI_DRAWSHAPE_SET_TEX_TRANSLATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800542C8 - translates the shape's texture coordinates by the two floats at `pos`. */
void drawshape_set_tex_translate(const f32* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAWSHAPE_SET_TEX_TRANSLATE_H */
