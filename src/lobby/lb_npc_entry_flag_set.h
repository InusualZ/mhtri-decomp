/* `lb_npc_entry_flag_set`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2).  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_LOBBY_LB_NPC_ENTRY_FLAG_SET_H
#define MHTRI_LOBBY_LB_NPC_ENTRY_FLAG_SET_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FDFA4 (0x1C): stores the 0x80 bit of `entry` into the NPC's +0x264 byte and resets its +0x258 vector.  GUESS
 * name. */
void lb_npc_entry_flag_set(struct _LB_NPC* npc, u8 entry);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_ENTRY_FLAG_SET_H */
