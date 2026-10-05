/* lobby/lb_act_dispatch.h - the declarations of the two act dispatchers `lobby/lb_companion_ui.cpp` owns (docs/plan.md
 * 6.5 rule 2, a leaf header: the owner's full header redeclares `lobby_world_block` with its own type, so it cannot be
 * included beside the network units).  `LbActReq` is defined by the owner's header. */
#ifndef MHTRI_LOBBY_LB_ACT_DISPATCH_H
#define MHTRI_LOBBY_LB_ACT_DISPATCH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80338808 - dispatches one act request (message kind 10) for member `index`. */
void lb_act_dispatch(u8 index, struct LbActReq* req);
/* 0x80339F10 - the second act dispatcher (message kind 13; GUESS name: the session case-29 caller hands it the same
 * member and payload as `lb_act_dispatch`). */
void lb_act_dispatch_ex(u8 index, const struct NetMsgHeader* msg);
#ifdef __cplusplus
}
#endif

#endif
