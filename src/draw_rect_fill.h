/* draw_rect_fill.h - leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s rectangle draw, for `menu/movie.cpp`.  GUESS name (the registry carries it); `DrawRect` is in `fn_8004CAD8.h`. */
#ifndef MHTRI_DRAW_RECT_FILL_H
#define MHTRI_DRAW_RECT_FILL_H

#include "types.h"

struct DrawRect;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005351C - draws `rect` as one coloured quad. */
void draw_rect_fill(struct DrawRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_RECT_FILL_H */
