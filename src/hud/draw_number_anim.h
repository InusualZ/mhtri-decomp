/* `draw_number_anim`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_DRAW_NUMBER_ANIM_H
#define MHTRI_HUD_DRAW_NUMBER_ANIM_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the number sprite `id` of animation part `part` with frame `frame` in `color` at `pos`. */
u32 draw_number_anim(u16 id, u16 part, u8 frame, u32 color, const struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_NUMBER_ANIM_H */
