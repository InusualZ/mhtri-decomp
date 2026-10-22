/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui`'s `lb_sub1a_send`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_LOBBY_LB_SUB1A_SEND_H
#define MHTRI_LOBBY_LB_SUB1A_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339D94 (lobby/lb_companion_ui): sends the lobby sub-command 0x1A for `player` with `kind`. */
void lb_sub1a_send(u8 player, s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB1A_SEND_H */
