/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `hud/layout.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_DRAW_HELP_WINDOW_H
#define MHTRI_HUD_DRAW_HELP_WINDOW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4530 - draws the help window frame (kind 0 the plain one, else the warning one). */
void draw_help_window(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_HELP_WINDOW_H */
