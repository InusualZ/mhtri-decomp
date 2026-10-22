/* `draw_weaponicon_idx`, defined by `hud/layout.cpp` (leaf header, rule 2).  C++ linkage: the map spells it
 * `draw_weaponicon_idx__FUsUcUlPC10_mh_ivec2_`. */
#ifndef MHTRI_HUD_DRAW_WEAPONICON_IDX_H
#define MHTRI_HUD_DRAW_WEAPONICON_IDX_H

#include "types.h"

struct _mh_ivec2_;

/* Draws weapon icon `kind` of sprite `id` in `color` at `pos`. */
void draw_weaponicon_idx(u16 id, u8 kind, u32 color, const struct _mh_ivec2_* pos);

#endif /* MHTRI_HUD_DRAW_WEAPONICON_IDX_H */
