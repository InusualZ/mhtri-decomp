/* `lb_npc_mark_state_get`, defined by `ef/eft050.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EF_LB_NPC_MARK_STATE_GET_H
#define MHTRI_EF_LB_NPC_MARK_STATE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The NPC mark state from the lobby block: 0 while +0x5228 is set, else -1 while +0x5208 is clear, else 1. */
s16 lb_npc_mark_state_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LB_NPC_MARK_STATE_GET_H */
