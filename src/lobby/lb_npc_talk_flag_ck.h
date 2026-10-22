/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_npc_talk_flag_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_NPC_TALK_FLAG_CK_H
#define MHTRI_LOBBY_LB_NPC_TALK_FLAG_CK_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021D704 - 1 when NPC `self` is non-null and bit 0 of its +0x227 byte is set */
u32 lb_npc_talk_flag_ck(struct _LB_NPC* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_TALK_FLAG_CK_H */
