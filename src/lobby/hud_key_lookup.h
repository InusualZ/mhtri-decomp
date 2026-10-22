/* lobby/hud_key_lookup.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp`'s `hud_key_lookup`, for
 *   `menu/menu_plsearch.cpp`, which cannot include the owner's full header beside its own lobby types.  C linkage. */
#ifndef MHTRI_LOBBY_HUD_KEY_LOOKUP_H
#define MHTRI_LOBBY_HUD_KEY_LOOKUP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033A0BC (0x80): the index of the hunter card whose id text starts with `key` (its first six bytes), 255 when
 * none of the ten does. */
u8 hud_key_lookup(u8* key);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_HUD_KEY_LOOKUP_H */
