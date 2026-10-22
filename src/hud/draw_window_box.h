/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/layout` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_DRAW_WINDOW_BOX_H
#define MHTRI_HUD_DRAW_WINDOW_BOX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E3D18 - a 9-slice textured window: fill then eight 24px border quads, min 48x48 */
void draw_window_box(s16 x, s16 y, u16 w, u16 h, s32 style, s32 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_WINDOW_BOX_H */
