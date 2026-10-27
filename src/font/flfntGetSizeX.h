/* font/flfntGetSizeX.h - leaf header (docs/plan.md 6.5 rule 2): `font/flfnt.cpp`'s font size reads, for `menu/movie.cpp`.  C++ scope (the map rows are `flfntGetSizeX__Fv`, `flfntGetSizeY__Fv`). */
#ifndef MHTRI_FONT_FLFNTGETSIZEX_H
#define MHTRI_FONT_FLFNTGETSIZEX_H

#include "types.h"

/* 0x8005B6D4 / 0x8005B6F8 - the font's character width / height. */
s16 flfntGetSizeX(void);
s16 flfntGetSizeY(void);

#endif /* MHTRI_FONT_FLFNTGETSIZEX_H */
