/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `cockpit_lobby_reset`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_HUD_COCKPIT_LOBBY_RESET_H
#define MHTRI_HUD_COCKPIT_LOBBY_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DFBE8 - resets the cockpit for the lobby (fn_802DFC1C) */
void cockpit_lobby_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_LOBBY_RESET_H */
