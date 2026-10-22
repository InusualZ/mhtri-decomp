/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em020_prog`'s `lobby_net_busy_ck`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_ENEMY_LOBBY_NET_BUSY_CK_H
#define MHTRI_ENEMY_LOBBY_NET_BUSY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803768F8 (enemy/em020_prog): whether the lobby network is busy. */
u32 lobby_net_busy_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_NET_BUSY_CK_H */
