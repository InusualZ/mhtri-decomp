/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/layout` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_DRAW_LAYOUT_WINDOW_H
#define MHTRI_HUD_DRAW_LAYOUT_WINDOW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4110 - draws a layout window in the default style */
void draw_layout_window(s16 x, s16 y, u16 w, u16 h);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_LAYOUT_WINDOW_H */
