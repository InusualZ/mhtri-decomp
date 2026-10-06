/*
 * lobby/LbStr.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/lb_npc.cpp`'s `LbStr` (0x80211E1C), a C++ free
 *   function; kept apart from `lobby/lb_cmd_pressed_ck.h` because the owner includes that header inside one of its
 *   namespaces, where a C++ declaration would take the namespace into its mangling.
 */
#ifndef MHTRI_LOBBY_LBSTR_H
#define MHTRI_LOBBY_LBSTR_H

#include "types.h"

#ifdef __cplusplus
/* The lobby string table's entry `idx` of group `kind` (`LbStr__FUcUs`); the owner's own spelling. */
/* untyped: opaque handle passed through - every caller hands the entry straight to the font layer as its own string type */
void* LbStr(u8 kind, u16 idx);
#endif

#endif /* MHTRI_LOBBY_LBSTR_H */
