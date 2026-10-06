/* Leaf header: `cockpit_hud_hidden_ck` (0x802E54F8), defined in `hud/cockpit_quest.cpp`'s 0x802E4978 band, for its
 * `_PLW` callers (the band header `menu/fn_802E4978.h` redefines the cockpit records). */
#ifndef MHTRI_MENU_COCKPIT_HUD_HIDDEN_CK_H
#define MHTRI_MENU_COCKPIT_HUD_HIDDEN_CK_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E54F8 - 1 when the cockpit HUD is hidden for the player `plw` (`mode` 0xFF: the current view's rule,
 * otherwise the arena option `mode` of the VS chunk), or when the act word at +0x308 is 1. */
u32 cockpit_hud_hidden_ck(_PLW* plw, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_COCKPIT_HUD_HIDDEN_CK_H */
