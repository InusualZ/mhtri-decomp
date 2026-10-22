/* draw_rect_set.h - leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s rectangle setter, for `menu/movie.cpp`.  GUESS name (the registry carries it); `DrawRect` is in `fn_8004CAD8.h`. */
#ifndef MHTRI_DRAW_RECT_SET_H
#define MHTRI_DRAW_RECT_SET_H

#include "types.h"

struct DrawRect;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800525F0 - fills `rect` with the corners (x, y), (x + w, y + h) and `color`. */
void draw_rect_set(struct DrawRect* rect, s16 x, s16 y, u16 w, u16 h, u32 color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_RECT_SET_H */
