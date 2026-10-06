/* Leaf header (docs/plan.md 6.5 rule 2): `hud/layout.cpp`'s indexed font draw, for a consumer that cannot include
 * `hud/layout.h` (its `pl.h` and `_mh_ivec2_` meet another view).  C++ linkage (the map row is
 * `draw_font_idx__FUsPScUlPC10_mh_ivec2_`). */
#ifndef MHTRI_HUD_DRAW_FONT_IDX_H
#define MHTRI_HUD_DRAW_FONT_IDX_H

#include "types.h"

struct _mh_ivec2_;

/* 0x802E22FC - draws string `str` with font sprite `id` at `pos`. */
void draw_font_idx(u16 id, s8* str, u32 flags, const struct _mh_ivec2_* pos);

#endif /* MHTRI_HUD_DRAW_FONT_IDX_H */
