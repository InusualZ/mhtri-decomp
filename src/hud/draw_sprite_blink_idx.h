/* hud/draw_sprite_blink_idx.h - leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s blinking arrow pair, for `menu/movie.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_DRAW_SPRITE_BLINK_IDX_H
#define MHTRI_HUD_DRAW_SPRITE_BLINK_IDX_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DAE24 - draws the sprite pair `left`/`right` (0xFFFF: none) in `color`, the one `moved` names blinking. */
void draw_sprite_blink_idx(u16 left, u16 right, u32 color, u16 moved, const struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_SPRITE_BLINK_IDX_H */
