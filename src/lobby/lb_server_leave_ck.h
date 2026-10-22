/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans`'s `lb_server_leave_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_LOBBY_LB_SERVER_LEAVE_CK_H
#define MHTRI_LOBBY_LB_SERVER_LEAVE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C4AB4 (lobby/lb_server_sel_trans): whether the server-select state is one of the leaving ones (39, 43, 44, 47, 255). */
u32 lb_server_leave_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_LEAVE_CK_H */
