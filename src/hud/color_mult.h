/* `color_mult`, defined by `hud/layout.cpp` (leaf header, rule 2).  C++ linkage: the map spells it
 * `color_mult__FUlUl`. */
#ifndef MHTRI_HUD_COLOR_MULT_H
#define MHTRI_HUD_COLOR_MULT_H

#include "types.h"

/* The channel-wise product of two RGBA colours. */
u32 color_mult(u32 a, u32 b);

#endif /* MHTRI_HUD_COLOR_MULT_H */
