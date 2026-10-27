/* font/dbg_drawgraph.h - the cross-unit declarations of `font/dbg_drawgraph.cpp`, the screen-space debug draw.
 *   Its `nw4r::ut::Color` weak copies are called through the class (`nw4r/ut_Color.h`). */
#ifndef MHTRI_FONT_DBG_DRAWGRAPH_H
#define MHTRI_FONT_DBG_DRAWGRAPH_H

#include "types.h"

#ifdef __cplusplus
/* 0x8005AF1C `dbg_drawgraph_init__Fv` (C++ scope) - sets GX up for flat untextured screen-space drawing. */
void dbg_drawgraph_init(void);
#endif

#endif /* MHTRI_FONT_DBG_DRAWGRAPH_H */
