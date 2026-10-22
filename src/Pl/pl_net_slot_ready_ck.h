/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s `pl_net_slot_ready_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_NET_SLOT_READY_CK_H
#define MHTRI_PL_PL_NET_SLOT_READY_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80268308 - 1 when the player's net slot is in state 2 and none of its 11 flags is 1 */
u32 pl_net_slot_ready_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_NET_SLOT_READY_CK_H */
