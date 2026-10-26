/* `lb_npc_talk_select`, defined by `lobby/fn_80219260.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_NPC_TALK_SELECT_H
#define MHTRI_LOBBY_LB_NPC_TALK_SELECT_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021C738 - lets lobby NPC `npc` pick its talk again. */
void lb_npc_talk_select(struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_TALK_SELECT_H */
