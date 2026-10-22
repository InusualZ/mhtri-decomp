/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `lb_pl_se_frame_callback`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_PL_SE_FRAME_CALLBACK_H
#define MHTRI_LOBBY_LB_PL_SE_FRAME_CALLBACK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C4BA0 - the lobby player's SE frame callback (se_req_frame_set by motion) */
void lb_pl_se_frame_callback(struct _PLW* plw, s32 event);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PL_SE_FRAME_CALLBACK_H */
