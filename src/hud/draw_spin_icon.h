/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `hud/layout.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_DRAW_SPIN_ICON_H
#define MHTRI_HUD_DRAW_SPIN_ICON_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4850 - draws the waiting spinner at (x, y). */
void draw_spin_icon(s16 x, s16 y, s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_SPIN_ICON_H */
