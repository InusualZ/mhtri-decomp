/*
 * The two 2D rectangle records `src/fn_8004CAD8.cpp` fills (`draw_rect_set`, `draw_rect_2d_tex_init`), split out of
 * `fn_8004CAD8.h` so the owner's source can include them without that header's consumer-side declarations.
 */
#ifndef MHTRI_DRAW_RECT_H
#define MHTRI_DRAW_RECT_H

#include "types.h"

/* One 2D textured rectangle for `draw_rect_2d_tex_by_id` (the map's `_DRAW_RECT_2D_TEX`): the corner, the
 * opposite corner, the colour word and the two texture coordinate pairs.  size: 0x14 */
typedef struct _DRAW_RECT_2D_TEX {
    /* +0x00 */ s16 x;       /* the top-left corner */
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 end_x;   /* the opposite corner (corner + size) */
    /* +0x06 */ s16 end_y;
    /* +0x08 */ u32 color;
    /* +0x0C */ s16 u0;      /* the first texture coordinate pair */
    /* +0x0E */ s16 v0;
    /* +0x10 */ s16 u1;      /* the second pair */
    /* +0x12 */ s16 v1;
} _DRAW_RECT_2D_TEX;

/* The screen rectangle `draw_rect_set` (0x800525F0) fills and `draw_shape.cpp`'s `draw_rect_fill` (0x8005351C)
 * draws: two corners and the fill colour.  size: 0xC */
typedef struct DrawRect {
    /* +0x0 */ s16 x0;
    /* +0x2 */ s16 y0;
    /* +0x4 */ s16 x1;
    /* +0x6 */ s16 y1;
    /* +0x8 */ u32 color;
} DrawRect;

#endif /* MHTRI_DRAW_RECT_H */
