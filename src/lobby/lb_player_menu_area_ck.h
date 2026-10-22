/* lobby/lb_player_menu_area_ck.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s menu-area test, for `menu/movie.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_PLAYER_MENU_AREA_CK_H
#define MHTRI_LOBBY_LB_PLAYER_MENU_AREA_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802092AC - 1 while `self` stands in the lobby's menu area. */
u32 lb_player_menu_area_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLAYER_MENU_AREA_CK_H */
