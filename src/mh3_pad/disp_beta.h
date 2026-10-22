/* mh3_pad/disp_beta.h - leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `disp_beta`, for
 *   `menu/menu_placeinfo.cpp`.  C++ linkage (the map row is the mangling `disp_beta__FP3VecP4Vec28_GXColor`); the
 *   vector types are named by tag only, so the header can sit beside either of the two `Vec` definitions. */
#ifndef MHTRI_MH3_PAD_DISP_BETA_H
#define MHTRI_MH3_PAD_DISP_BETA_H

#include "types.h"
#include "gx.h"   /* _GXColor */

struct Vec;
struct Vec2;

/* 0x8004140C: a flat `color` rectangle at `pos` of size `size`. */
void disp_beta(struct Vec* pos, struct Vec2* size, _GXColor color);

#endif /* MHTRI_MH3_PAD_DISP_BETA_H */
