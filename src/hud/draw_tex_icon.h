/* `draw_tex_icon`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_DRAW_TEX_ICON_H
#define MHTRI_HUD_DRAW_TEX_ICON_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws a copy of `rec` with atlas texture `tex_idx` and its colour multiplied by `color` at `pos`. */
void draw_tex_icon(_SPR_DATA_* rec, u8 tex_idx, u32 color, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_TEX_ICON_H */
