/* lobby/lb_player_frame.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s lobby player frame and draw,
 * for `Pl/player_control.cpp`. */
#ifndef MHTRI_LOBBY_LB_PLAYER_FRAME_H
#define MHTRI_LOBBY_LB_PLAYER_FRAME_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8020852C (0x104): the lobby player's frame. */
void lb_player_frame(struct _PLW* self);
/* 0x80208630 (0x1A4): draws the lobby player. */
void lb_player_draw(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLAYER_FRAME_H */
