/* lobby/lb_menu_kind_pair_ck.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80219260.cpp`'s
 *   `lb_menu_kind_pair_ck` (0x8021B8A8).  C linkage (the map row is a plain name); the callers compare the result
 *   unsigned and pass the two bytes unextended. */
#ifndef MHTRI_LOBBY_LB_MENU_KIND_PAIR_CK_H
#define MHTRI_LOBBY_LB_MENU_KIND_PAIR_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021B8A8 - 1 when the actor kind and area pair is one of the lobby menu's. */
u32 lb_menu_kind_pair_ck(u8 kind, u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_KIND_PAIR_CK_H */
