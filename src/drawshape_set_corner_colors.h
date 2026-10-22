/* Leaf header (docs/plan.md 6.5 rule 2): symbols `draw_shape.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_DRAWSHAPE_SET_CORNER_COLORS_H
#define MHTRI_DRAWSHAPE_SET_CORNER_COLORS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80053960: sets the shape's four corner colours. */
void drawshape_set_corner_colors(u32 c0, s32 c1, s32 c2, u32 c3);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAWSHAPE_SET_CORNER_COLORS_H */
