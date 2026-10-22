/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans`'s `lb_server_ready_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_LOBBY_LB_SERVER_READY_CK_H
#define MHTRI_LOBBY_LB_SERVER_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C4B14 (lobby/lb_server_sel_trans): whether the server select runs and its state is a ready one (47 or 255). */
u32 lb_server_ready_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_READY_CK_H */
