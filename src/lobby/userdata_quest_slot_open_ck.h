/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `userdata_quest_slot_open_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_USERDATA_QUEST_SLOT_OPEN_CK_H
#define MHTRI_LOBBY_USERDATA_QUEST_SLOT_OPEN_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E9C20 - 0 when the 12-byte record `id` at +0x3F38 is kind 7 and in use, else 1 */
u32 userdata_quest_slot_open_ck(u8 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_USERDATA_QUEST_SLOT_OPEN_CK_H */
