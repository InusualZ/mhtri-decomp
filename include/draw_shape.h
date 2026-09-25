/* The declarations `src/draw_shape.cpp` owns (docs/plan.md 6.5 rule 2): the draw-shape state block's
 * setters.  Created when `enemy/fn_80147CE0.cpp` registered as a consumer - the owner had no header.
 * The signature is the owner's own definition (`extern "C" void fn_80056A54(u32 a, u32 b, u32 c)`,
 * which stores r3 and r5 as words and `(u8)r4`), so a caller with a pointer first argument casts it.
 */
#ifndef MHTRI_DRAW_SHAPE_H
#define MHTRI_DRAW_SHAPE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80056A54 - arm the draw-shape state block (`lbl_8066ACF8`). */
void fn_80056A54(u32 a, u32 b, u32 c);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_SHAPE_H */
