/* font/flfntGetSizeX.h - leaf header (docs/plan.md 6.5 rule 2): `font/flfnt.cpp`'s font size reads and debug-draw init, for `menu/movie.cpp`.  C++ scope (the map rows are `flfntGetSizeX__Fv`, `flfntGetSizeY__Fv`, `dbg_drawgraph_init__Fv`). */
#ifndef MHTRI_FONT_FLFNTGETSIZEX_H
#define MHTRI_FONT_FLFNTGETSIZEX_H

#include "types.h"

/* 0x8005B6D4 / 0x8005B6F8 - the font's character width / height. */
s16 flfntGetSizeX(void);
s16 flfntGetSizeY(void);
/* 0x8005AF1C - resets the debug draw state. */
void dbg_drawgraph_init(void);

#endif /* MHTRI_FONT_FLFNTGETSIZEX_H */
