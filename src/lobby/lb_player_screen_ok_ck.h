/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s `lb_player_screen_ok_ck`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_PLAYER_SCREEN_OK_CK_H
#define MHTRI_LOBBY_LB_PLAYER_SCREEN_OK_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802088B4 - whether the lobby player's screen is ready for its model to move. */
u32 lb_player_screen_ok_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLAYER_SCREEN_OK_CK_H */
