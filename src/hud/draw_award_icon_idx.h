/* hud/draw_award_icon_idx.h - leaf header (docs/plan.md 6.5 rule 2): `hud/layout.cpp`'s 0x802E18D4, for
 *   `menu/menu_placeinfo.cpp`.  C linkage (the map row is a plain name); the signature is the owner's definition.
 *   GUESS name (registry). */
#ifndef MHTRI_HUD_DRAW_AWARD_ICON_IDX_H
#define MHTRI_HUD_DRAW_AWARD_ICON_IDX_H

#include "types.h"
#include "hud/layout_types.h"   /* _mh_ivec2_ */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E18D4: draws sprite `id` at `pos` with the award icon uv pair of `tex_idx`. */
void draw_award_icon_idx(u16 id, u8 tex_idx, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_AWARD_ICON_IDX_H */
