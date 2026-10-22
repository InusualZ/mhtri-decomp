/* disp_beta.h - leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s flat rectangle draw, for `menu/movie.cpp`.  C++ scope (the map row is `disp_beta__FP3VecP4Vec28_GXColor`). */
#ifndef MHTRI_DISP_BETA_H
#define MHTRI_DISP_BETA_H

#include "types.h"
#include "MTX/mtx.h"
#include "KPAD/kpad.h"
#include "gx.h"

/* 0x8004140C - fills the rectangle at `pos` of `size` with `color`. */
void disp_beta(Vec* pos, Vec2* size, GXColor color);

#endif /* MHTRI_DISP_BETA_H */
